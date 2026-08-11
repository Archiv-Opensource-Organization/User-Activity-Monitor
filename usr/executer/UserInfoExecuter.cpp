#define RPC_NO_WINDOWS_H

// 1. 第一步：引入 cstddef，把 std::byte 引进来
#include <cstddef>

// 2. 第二步：【核心操作】把 std::byte 藏进一个安全的命名空间里
// 这样它就不会和 Windows SDK 冲突了，但你依然可以用 MyStd::byte
namespace MyStd {
    using byte = std::byte;
}

// 3. 第三步：现在可以放心地包含 Windows 头文件了！
// 因为全局的 std::byte 已经被藏起来了，Windows 的 unsigned char byte 可以顺利定义
#include <windows.h>
#include <lm.h>

#include "UserInfoExecuter.h"
#include <iostream>
#include <string>

#include "../log/LogToFile.h"
#include "../log/LogFilename.h"

#pragma comment(lib, "netapi32.lib")

using namespace std;

/**
 * 修改用户
 * @param username 用户名
 * @param newPassword 新密码，可为空
 * @param operationType 操作方式
 * 0 为锁定用户，1为解锁，2为更改密码，此时 newPassword 不为空
 */
void UserInfoExecuter::modifyUserAccount(const string& username, const string& newPassword, int operationType) {
    if (username.empty()) return;

    // Unicode：转换用户名为宽字符
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, username.c_str(), -1, NULL, 0);
    wstring wUserName(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, username.c_str(), -1, &wUserName[0], size_needed);

    USER_INFO_1 ui = { 0 };
    ui.usri1_name = (LPWSTR)wUserName.c_str();
    ui.usri1_priv = USER_PRIV_USER;

    // 根据操作类型配置参数
    switch (operationType) {
        case 0:
            // Lock
            ui.usri1_flags = UF_ACCOUNTDISABLE;
            log(mainProgram, "UserInfoExecuter::modifyUserAccount", ("[SUCCESS] 成功锁定用户 " + username).c_str());
            break;
        case 1:
            // Unlock
            log(mainProgram, "UserInfoExecuter::modifyUserAccount", ("[SUCCESS] 成功解锁用户 " + username).c_str());
            ui.usri1_flags = UF_NORMAL_ACCOUNT | UF_PASSWD_NOTREQD; // 启用并清除禁用标志
            break;
        case 2:
            // Change Password
            //
            // 转换新密码为宽字符
            log(mainProgram, "UserInfoExecuter::modifyUserAccount", ("[SUCCESS] 将用户 " + username + " 的密码更改为 " + newPassword).c_str());
            int pwd_size = MultiByteToWideChar(CP_UTF8, 0, newPassword.c_str(), -1, NULL, 0);
            wstring wPassword(pwd_size, 0);
            MultiByteToWideChar(CP_UTF8, 0, newPassword.c_str(), -1, &wPassword[0], pwd_size);
            ui.usri1_password = (LPWSTR)wPassword.c_str();
            break;
    }

    // 2. 调用 API
    DWORD dwError = 0;
    NET_API_STATUS nStatus = NetUserSetInfo(NULL, wUserName.c_str(), 1, (LPBYTE)&ui, &dwError);

    // 3. 处理结果
    if (nStatus == NERR_Success) {
        cout << "[+] 操作成功: " << username << endl;
        log(mainProgram, "UserInfoExecuter::modifyUserAccount", ("[+] 操作成功：" + username).c_str());

    } else {
        cerr << "[-] 操作失败: " << username << " | 错误码: " << nStatus << endl;

        log(mainProgram, "UserInfoExecuter::modifyUserAccount", ("[+] 操作失败：" + username + " | 错误码：" + to_string(nStatus)).c_str());
    }
}

// --- 对外接口 ---
void UserInfoExecuter::lockUser(string userName) {
    modifyUserAccount(userName, "", 0);
}

void UserInfoExecuter::unlockUser(string username) {
    modifyUserAccount(username, "", 1);
}

void UserInfoExecuter::changeUserPassword(string username, string password) {
    modifyUserAccount(username, password, 2);
}
