//
// Created by Byakuya on 2026/8/4.
//

#include "UserSecurityHandler.h"
#include "../smtp/MailService.h"
#include "../Generic_Configuration.h"
#include "utils/WindowsLogHelper.h"
#include "log/LogToFile.h"
#include "log/LogFilename.h"

#include <windows.h>
#include <winevt.h>
#include <string>

#pragma comment(lib, "wevtapi.lib")

void UserSecurityHandler::ProcessSuccessEvent(EVT_HANDLE hEvent) {
    DWORD dwBufferSize = 0;
    DWORD dwBufferUsed = 0;
    DWORD dwPropertyCount = 0;
    LPWSTR pBuffer = NULL;

    // 获取渲染后的 XML 所需大小
    if (!EvtRender(NULL, hEvent, EvtRenderEventXml, dwBufferSize, pBuffer, &dwBufferUsed, &dwPropertyCount)) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

            pBuffer = (LPWSTR)malloc(dwBufferUsed);
            if (!pBuffer) {
                log(mainProgram, "UserSecurityHandler::ProcessSuccessEvent", "[ERROR] 内存分配失败！");
                return;
            }

            if (EvtRender(NULL, hEvent, EvtRenderEventXml, dwBufferUsed, pBuffer, &dwBufferUsed, &dwPropertyCount)) {
                std::wstring xmlStr(pBuffer);

                // 提取 LogonType (登录类型)
                std::wstring logonType = ExtractXmlValue(xmlStr, L"LogonType");

                // 提取 TargetUserName (登录用户名)
                std::wstring userName = ExtractXmlValue(xmlStr, L"TargetUserName");

                std::string sLoginTypeDesc; // 【修复3】直接使用窄字符串，避免宽窄转换的麻烦

                // C. 过滤系统内部噪音
                // LogonType 2 = 交互式登录 (本地键盘)
                // LogonType 10 = 远程桌面 (RDP)
                if ((logonType == L"2" || logonType == L"10") &&
                    userName != L"SYSTEM" &&
                    userName != L"DWM-1" &&
                    userName != L"DWM-15" &&      // 新增：桌面窗口管理器
                    userName != L"UMFD-15" &&     // 新增：用户模式驱动框架
                    userName != L"NETWORK SERVICE" && // 新增：网络服务
                    userName != L"LOCAL SERVICE" &&   // 新增：本地服务
                    !userName.empty()) {

                    if (logonType == L"2") {
                        sLoginTypeDesc = "服务器本地交互";
                    } else if (logonType == L"10") {
                        sLoginTypeDesc = "RDP 登录";
                    }

                    // 【修复2】彻底抛弃 cout，使用安全的 log() 函数
                    log(mainProgram, "UserSecurityHandler::ProcessSuccessEvent", "[SUCCESS] 用户登录成功: %s | 登录类型: %s (%s)",
                        WStringToString(userName).c_str(),
                        WStringToString(logonType).c_str(),
                        sLoginTypeDesc.c_str());

                    if (SMTP_FEATURE_ENABLED and EVERY_LOGIN_PROCEED_AN_EMAIL) {
                        send_email(1, WStringToString(userName), WStringToString(ExtractIpAddress(xmlStr)));
                    }
                }
            } else {
                log(mainProgram, "UserSecurityHandler::ProcessSuccessEvent","[ERROR] EvtRender 第二次调用失败，错误码: %lu", GetLastError());
            }

            // 【修复4】安全释放内存
            free(pBuffer);
        }
    }
}

// EVT API 要求的回调函数
DWORD WINAPI SuccessEvtCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent) {
    UNREFERENCED_PARAMETER(pContext);
    if (action == EvtSubscribeActionDeliver) {
        UserSecurityHandler::ProcessSuccessEvent(hEvent);
    }
    return ERROR_SUCCESS;
}

