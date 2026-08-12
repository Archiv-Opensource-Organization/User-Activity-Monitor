#include "SuspiciousLoginAttemptsHandler.h"

#include <algorithm>
#include <windows.h>
#include <winevt.h>
#include <iostream>
#include <string>
#include <map>
#include <set>
#include <mutex>          // 【新增】用于多线程并发安全
#include <vector>         // 【新增】用于时间窗口记录
#include <chrono>         // 【新增】用于获取当前时间戳

#include "../Generic_Configuration.h"
#include "../smtp/MailService.h"
#include "../usr/executer/UserInfoExecuter.h"
#include "utils/WindowsLogHelper.h"
#include "log/LogToFile.h"
#include "log/LogFilename.h"
#include "Configuration.h"
#include "../runtime/windows/file/Configuration.h"
#include "../runtime/windows/file/FileExecuter.h"
#include "../runtime/windows/NetFirewallPolicyExecuter.h"

#pragma comment(lib, "wevtapi.lib")
#pragma comment(lib, "netapi32.lib")

static std::mutex g_mutex;

// 记录每个用户名对应的所有攻击 IP
static std::map<std::wstring, std::set<std::wstring>> g_userAttackIps;
// 记录每个用户名在时间窗口内的失败次数
static std::map<std::wstring, int> g_loginFailCounter;
// 记录每个 IP 在时间窗口内的失败次数
// static std::map<std::wstring, int> g_ipCounter;

static std::map<std::wstring, std::vector<std::chrono::steady_clock::time_point>> g_userFailTimestamps;
static std::map<std::wstring, std::vector<std::chrono::steady_clock::time_point>> g_ipFailTimestamps;

// 系统账号白名单
static const std::set<std::wstring> g_systemUserWhitelist = {
    L"SYSTEM", L"ANONYMOUS LOGON", L"IUSR", L"IWAM", L"LOCAL SERVICE", L"NETWORK SERVICE"
};

//清理过期时间戳的辅助函数（例如只保留最近 5 分钟内的记录）
void CleanExpiredTimestamps(std::vector<std::chrono::steady_clock::time_point>& timestamps, int windowSeconds) {
    auto now = std::chrono::steady_clock::now();
    auto window = std::chrono::seconds(windowSeconds);
    timestamps.erase(
        std::remove_if(timestamps.begin(), timestamps.end(),
            [&](const std::chrono::steady_clock::time_point& tp) { return (now - tp) > window; }),
        timestamps.end()
    );
}

void SuspiciousLoginAttemptsHandler::ProcessEventXml(EVT_HANDLE hEvent) {
    DWORD dwBufferSize = 0;
    DWORD dwBufferUsed = 0;
    DWORD dwPropertyCount = 0;
    LPWSTR pBuffer = NULL;

    if (!EvtRender(NULL, hEvent, EvtRenderEventXml, dwBufferSize, pBuffer, &dwBufferUsed, &dwPropertyCount)) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            pBuffer = (LPWSTR)malloc(dwBufferUsed);
            if (!pBuffer) {
                log(mainProgram, "SuspiciousLoginAttemptsHandler::ProcessEventXml", "[ERROR] 内存分配失败！");
                return;
            }

            if (EvtRender(NULL, hEvent, EvtRenderEventXml, dwBufferUsed, pBuffer, &dwBufferUsed, &dwPropertyCount)) {
                std::wstring xmlStr(pBuffer);

                if (xmlStr.find(L"<EventID>4625</EventID>") != std::wstring::npos) {
                    std::wstring userName = ExtractXmlValue(xmlStr, L"TargetUserName");
                    std::wstring ipAddress = ExtractIpAddress(xmlStr);

                    if (!userName.empty() && !ipAddress.empty()) {
                        // 【新增】1. 过滤系统账号和机器账号（以$结尾）
                        if (g_systemUserWhitelist.find(userName) != g_systemUserWhitelist.end() ||
                            (!userName.empty() && userName.back() == L'$')) {
                            free(pBuffer);
                            return;
                        }

                        bool lockedUserExists = false;
                        bool blockedIPExists = false;

                        // 2. 检查是否已被锁定
                        if (contains(LOCKED_USERS, WStringToString(userName).c_str())) {

                            log(mainProgram, "SuspiciousLoginAttemptsHandler::ProcessEventXml",
                                "[INFO] Username detected in LOCKED_USERS: %s", WStringToString(userName).c_str());

                            lockedUserExists = true;
                        }

                        if (contains(BLOCKED_IPS, WStringToString(ipAddress).c_str())) {

                            log(mainProgram, "SuspiciousLoginAttemptsHandler::ProcessEventXml",
                                "[INFO] Username detected in BLOCKED_IPS: %s", WStringToString(ipAddress).c_str());

                            if (!lockedUserExists) {
                                add(LOCKED_USERS, WStringToString(userName).c_str()); // 神秘补救
                            }

                            blockedIPExists = true;
                        }

                        if (lockedUserExists && blockedIPExists) {
                            log(mainProgram, "SuspiciousLoginAttemptsHandler::ProcessEventXml", "布尔值 lockedUserExists 或者 blockedIPExists 为 true，不对本事件操作。");

                            free(pBuffer);
                            return;
                        }

                        // 【新增】3. 加锁保护全局状态
                        std::lock_guard<std::mutex> lock(g_mutex);

                        // 记录时间戳
                        auto now = std::chrono::steady_clock::now();
                        g_userFailTimestamps[userName].push_back(now);
                        g_ipFailTimestamps[ipAddress].push_back(now);

                        // 清理 5 分钟（300秒）之前的旧记录
                        CleanExpiredTimestamps(g_userFailTimestamps[userName], 300);
                        CleanExpiredTimestamps(g_ipFailTimestamps[ipAddress], 300);

                        // 更新当前时间窗口内的真实计数
                        int userCount = (int)g_userFailTimestamps[userName].size();
                        int ipCount = (int)g_ipFailTimestamps[ipAddress].size();

                        // 记录 IP 到用户的映射
                        g_userAttackIps[userName].insert(ipAddress);

                        log(mainProgram, "SuspiciousLoginAttemptsHandler::ProcessEventXml",
                            "[!] 登录失败: User=%s | IP=%s | UserWinCount=%d/%d | IPWinCount=%d/%d",
                            WStringToString(userName).c_str(), WStringToString(ipAddress).c_str(),
                            userCount, LOGIN_FAIL_THRESHOLD, ipCount, LOGIN_FAIL_THRESHOLD);

                        bool triggerUserAlert = (userCount >= LOGIN_FAIL_THRESHOLD);
                        bool triggerIpAlert = (ipCount >= LOGIN_FAIL_THRESHOLD);

                        // 4. 触发警报（用户维度 或 IP维度）
                        if (triggerUserAlert || triggerIpAlert) {
                            std::string alertReason = triggerUserAlert ? "User Brute Force" : "IP Scanning/Attack";
                            log(mainProgram, "SuspiciousLoginAttemptsHandler::ProcessEventXml",
                                "*** ALERT: %s detected for user: %s ***",
                                alertReason.c_str(), WStringToString(userName).c_str());

                            // 组装 IP 列表
                            std::string allIpsStr = "";
                            const auto& ipSet = g_userAttackIps[userName];
                            for (auto it = ipSet.begin(); it != ipSet.end(); ++it) {
                                allIpsStr += WStringToString(*it);
                                if (std::next(it) != ipSet.end()) allIpsStr += ", ";
                            }

                            // 封禁相关 IP
                            for (const auto& ip : ipSet) {
                                std::string ipStr = WStringToString(ip);
                                log(mainProgram, "SuspiciousLoginAttemptsHandler::ProcessEventXml",
                                    "[FIREWALL] 正在封禁恶意 IP: %s", ipStr.c_str());
                                blockIPInFirewall(ip, L"Blocked");
                                if (!contains(BLOCKED_IPS, ipStr.c_str())) {
                                    add(BLOCKED_IPS, ipStr.c_str());
                                }
                            }

                            // 发送邮件
                            if (SMTP_FEATURE_ENABLED) {
                                send_email(0, WStringToString(userName), allIpsStr);
                            }

                            // 锁定用户
                            UserInfoExecuter user_info_executer;
                            user_info_executer.lockUser(WStringToString(userName));
                            if (!contains(LOCKED_USERS, WStringToString(userName).c_str())) {
                                add(LOCKED_USERS, WStringToString(userName).c_str());
                            }

                            // 重置该用户的攻击记录
                            g_loginFailCounter[userName] = 0;
                            g_userAttackIps[userName].clear();
                            g_userFailTimestamps[userName].clear();
                        }
                    }
                }
            } else {
                log(mainProgram, "SuspiciousLoginAttemptsHandler::ProcessEventXml",
                    "[ERROR] EvtRender 第二次调用失败。错误码: %lu", GetLastError());
            }
            free(pBuffer);
        }
    }
}

DWORD WINAPI SuspiciousLoginAttemptsHandler::SusEvtEventCallback(
    EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent)
{
    UNREFERENCED_PARAMETER(pContext);
    if (action == EvtSubscribeActionDeliver) {
        ProcessEventXml(hEvent);
    }
    return ERROR_SUCCESS;
}
