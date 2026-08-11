//
// Created by Byakuya on 2026/8/5.
//

#include "aio_UserActivityHandler.h"

#include <windows.h>
#include <winevt.h>
#include <iostream>
#include <string>

#include "utils/WindowsLogHelper.h"
#include "SuspiciousLoginAttemptsHandler.h"
#include "UserSecurityHandler.h"
#include "log/LogToFile.h"
#include "log/LogFilename.h"

using namespace std;

void aio_UserActivityHandler::handle_event(EVT_HANDLE hEvent) {

    log(mainProgram, "aio_UserActivityHandler::handle_event", "解析器开始。");

    // 2. 定义 EvtRender 所需的变量
    DWORD dwBufferSize = 0;      // 【核心】既作为输入(提供的大小)，也作为输出(需要的大小)
    DWORD dwPropertyCount = 0;   // 接收属性数量
    LPWSTR pBuffer = NULL;       // 接收 XML 字符串的缓冲区

    // 3. 第一次调用 EvtRender：故意传 0，逼 API 返回所需大小
    // 注意：最后一个参数是 &dwPropertyCount，不要传错！
    if (!EvtRender(NULL, hEvent, EvtRenderEventXml, dwBufferSize, pBuffer, &dwBufferSize, &dwPropertyCount)) {
        DWORD lastError = GetLastError();

        if (lastError == ERROR_INSUFFICIENT_BUFFER) {
            // 【关键】此时 dwBufferSize 已经被 API 偷偷修改为实际需要的字节数了！
            log(mainProgram, "aio_UserActivityHandler::handle_event", "[INFO] 缓冲区不足，API 提示需要 %lu 字节，正在分配...", dwBufferSize);

            // 4. 分配内存
            pBuffer = (LPWSTR)malloc(dwBufferSize);
            if (pBuffer) {
                // 5. 第二次调用 EvtRender：传入分配好的内存和正确的大小
                if (!EvtRender(NULL, hEvent, EvtRenderEventXml, dwBufferSize, pBuffer, &dwBufferSize, &dwPropertyCount)) {
                    log(mainProgram, "aio_UserActivityHandler::handle_event", "[ERROR] EvtRender 重试失败，错误码: %lu", GetLastError());
                    free(pBuffer);
                    pBuffer = NULL; // 失败后必须置空，防止后续误用
                } else {
                    log(mainProgram, "aio_UserActivityHandler::handle_event", "[SUCCESS] EvtRender 成功获取 XML");
                }
            } else {
                log(mainProgram, "aio_UserActivityHandler::handle_event", "[ERROR] malloc 内存分配失败！");
            }
        } else {
            // 如果不是缓冲区不足，说明是其他严重错误（如句柄无效）
            log(mainProgram, "aio_UserActivityHandler::handle_event", "[ERROR] EvtRender 首次调用失败，非缓冲区问题，错误码: %lu", lastError);
        }
    } else {
        // 极少情况：如果初始 dwBufferSize 足够大，第一次就会成功
        log(mainProgram, "aio_UserActivityHandler::handle_event", "[SUCCESS] EvtRender 首次调用即成功");
    }

    // 6. 如果成功拿到了 pBuffer，开始解析 XML
    if (pBuffer) {
        try {
            std::wstring xml(pBuffer);
            std::wstring eventIDStr = ExtractSystemValue(xml, L"EventID");
            DWORD eventID = _wtoi(eventIDStr.c_str());

            log(mainProgram, "aio_UserActivityHandler::handle_event", "[PARSE] 成功解析到 EventID: %lu", eventID);

            // 7. 根据 EventID 分发任务
            switch (eventID) {
                case 4625:
                    log(mainProgram, "aio_UserActivityHandler::handle_event","[ROUTE] 匹配到 4625 (登录失败)，转接 SuspiciousLoginAttemptsHandler");
                    {
                        SuspiciousLoginAttemptsHandler SLAH;
                        SLAH.ProcessEventXml(hEvent);
                    }
                    break;

                case 4624:
                    log(mainProgram, "aio_UserActivityHandler::handle_event", "[ROUTE] 匹配到 4624 (登录成功)，转接 UserSecurityHandler");
                    {
                        UserSecurityHandler USH;
                        USH.ProcessSuccessEvent(hEvent);
                    }
                    break;

                default:
                    // 忽略其他无关事件，避免日志刷屏
                    // log_msg("[IGNORE] 忽略事件 ID: %lu", eventID);
                    break;
            }
        } catch (const std::exception& e) {
            log(mainProgram, "aio_UserActivityHandler::handle_event", "[CRASH] 解析过程发生异常: %s", e.what());
        } catch (...) {
            log(mainProgram, "aio_UserActivityHandler::handle_event", "[CRASH] 解析过程发生未知异常");
        }

        // 8. 释放内存
        free(pBuffer);
        pBuffer = NULL;
    } else {
        log(mainProgram, "aio_UserActivityHandler::handle_event", "[WARN] pBuffer 为空，跳过解析逻辑");
    }
}

DWORD WINAPI aio_UserActivityHandler::EvtEventCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent) {

    if (action == EvtSubscribeActionDeliver) {
        aio_UserActivityHandler* pThis = static_cast<aio_UserActivityHandler*>(pContext);

        pThis->handle_event(hEvent);
    }
    return ERROR_SUCCESS;
}


void aio_UserActivityHandler::SubscribeToEvent(DWORD eventId, EVT_SUBSCRIBE_CALLBACK callback) {
    // 动态拼接 XPath 查询语句 (注意：必须使用宽字符 wstring)
    wstring xpath = L"Event/System[EventID=" + to_wstring(eventId) + L"]";

    m_hSubscription = EvtSubscribe(
        NULL,                               // Session
        NULL,                               // SignalEvent
        L"Security",                        // ChannelPath
        xpath.c_str(),                      // Query (动态生成的查询)
        NULL,                               // Bookmark
        this,                               // Context
        callback,                           // Callback
        EvtSubscribeToFutureEvents      // StartMode
    );

    if (m_hSubscription == NULL) {
        DWORD err = GetLastError();
        cerr << "[!] EvtSubscribe 无法为 EventID = " << eventId << " 的事件注册订阅。 | 错误码: " << err << endl;

        if (err == ERROR_ACCESS_DENIED) {
            cerr << "[!] 请以 管理员身份 运行" << endl;
        }

    } else {
        cout << "[+] 成功订阅 EventID: " << eventId << endl;
    }
}

/**
 * 开始监听
 * @param eventId 4625，4624
 */
void aio_UserActivityHandler::StartMonitoring(DWORD eventId) {
    cout << "[*] 监听器启动，正在订阅 EventID: " << eventId << " ..." << endl;
    SubscribeToEvent(eventId, EvtEventCallback);
}

// 3. 停止监听
void aio_UserActivityHandler::StopMonitoring() {
    if (m_hSubscription) {
        EvtClose(m_hSubscription);
        m_hSubscription = NULL;
        cout << "[*] 监听器已停止。" << endl;
    }
}