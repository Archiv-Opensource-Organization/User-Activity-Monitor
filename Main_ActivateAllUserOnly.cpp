//
// Created by Byakuya on 2026/8/6.
//

#include <windows.h>
#include <lm.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "netapi32.lib")

#include "ProductInfo.h"

#include "usr/UserExpireHandler.h"
#include "usr/log/LogToFile.h"
#include "usr/log/LogFilename.h"

#include "usr/utils/WindowsLogHelper.h"

#include "runtime/windows/AdminRuntimeChecker.h"

int main() {
    std::cout << "Running software version: " << VERSION << std::endl;

    std::string titleText = std::string(CO_PRODUCT_AAUO) + " " + VERSION;
    std::string shellCmd = "title " + titleText;
    system(shellCmd.c_str());

    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    // 1. 检查权限
    if (!IsRunAsAdmin()) {
        system("color c0");
        std::cout << "[!] 警告：未以管理员身份运行，无法使用 NET USER！\n";
        std::cout << "请右键点击程序 -> 以管理员身份运行。\n";

        std::cout << "按回车键退出..." << std::endl;
        std::cin.get();
        return 1;
    }

    LPUSER_INFO_0 pBuf = NULL;
    LPUSER_INFO_0 pTmpBuf;
    DWORD dwLevel = 0;
    DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = 0;
    NET_API_STATUS nStatus;

    // 定义一个系统账户黑名单，这些账户不需要也不应该被修改
    std::vector<std::wstring> blacklist = {
        L"Guest",
        L"DefaultAccount",
        L"SYSTEM",
        L"WDAGUtilityAccount"
    };

    std::cout << "[INFO] 开始批量设置本地用户密码永不过期..." << std::endl;
    log(activateAllUserOnly, "Main_ActivateAllUserOnly::main", "[INFO] 开始批量设置本地用户密码永不过期...");

    do {
        // 枚举本地用户
        nStatus = NetUserEnum(
            NULL,           // 本地计算机
            dwLevel,        // 级别 0，只获取用户名
            FILTER_NORMAL_ACCOUNT, // 只获取普通用户账户
            (LPBYTE*)&pBuf,
            dwPrefMaxLen,
            &dwEntriesRead,
            &dwTotalEntries,
            &dwResumeHandle
        );

        if ((nStatus == NERR_Success) || (nStatus == ERROR_MORE_DATA)) {
            pTmpBuf = pBuf;
            for (DWORD i = 0; i < dwEntriesRead; i++) {
                if (pTmpBuf == NULL) break;

                std::wstring currentUserName = pTmpBuf->usri0_name;

                // 检查是否在黑名单中
                bool isBlacklisted = false;
                for (const auto& blacklistedName : blacklist) {
                    if (_wcsicmp(currentUserName.c_str(), blacklistedName.c_str()) == 0) {
                        isBlacklisted = true;
                        break;
                    }
                }

                // 如果不在黑名单中，则执行设置
                if (!isBlacklisted) {
                    if (SetPasswordNeverExpires(currentUserName)) {

                        std::wcout << L"[SUCCESS] 用户 [" << currentUserName << L"] 密码已设置为永不过期。" << std::endl;

                        log(activateAllUserOnly, "Main_ActivateAllUserOnly::main()",
                            WStringToString(L"[SUCCESS] 用户 [" + currentUserName + L"] 密码已设置为永不过期。").c_str() );
                    } else {

                        std::wcout << L"[FAILED] 用户 [" << currentUserName << L"] 设置失败！" << std::endl;

                        log(activateAllUserOnly, "Main_ActivateAllUserOnly::main()",
                            WStringToString(L"[FAILED] 用户 [" + currentUserName + L"] 设置失败！").c_str() );

                    }
                } else {
                    std::wcout << L"[SKIP] 跳过系统/保留账户: [" << currentUserName << L"]" << std::endl;

                    log(activateAllUserOnly, "Main_ActivateAllUserOnly::main()",
                            WStringToString(L"[SKIP] 根据算法设计，跳过用户 [" + currentUserName + L"]。").c_str() );
                }

                pTmpBuf++;
            }
        } else {
            std::wcerr << L"[ERROR] 枚举用户失败，错误代码: " << nStatus << std::endl;

            log(activateAllUserOnly, "Main_ActivateAllUserOnly::main()",
                            WStringToString(L"[ERROR] 枚举失败！").c_str() );
        }

        // 释放当前批次的内存
        if (pBuf != NULL) {
            NetApiBufferFree(pBuf);
            pBuf = NULL;
        }
    } while (nStatus == ERROR_MORE_DATA);

    return 0;
}