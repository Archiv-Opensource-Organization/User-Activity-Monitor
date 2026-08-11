//
// Created by Byakuya on 2026/8/6.
//

#include "UserExpireHandler.h"

#include <windows.h>
#include <lm.h>
#include <string>
#include <iostream>

#pragma comment(lib, "netapi32.lib")

using namespace std;

/**
 * @brief 设置指定本地用户的密码为“永不过期”
 * @param username 目标用户名 (宽字符串)
 * @return true 设置成功, false 设置失败
 */
bool SetPasswordNeverExpires(const std::wstring& username) {
    NET_API_STATUS nStatus;
    USER_INFO_1008 ui1008 = {0};

    // UF_DONT_EXPIRE_PASSWD 是密码永不过期的标志位
    ui1008.usri1008_flags = UF_DONT_EXPIRE_PASSWD;

    // 调用 NetUserSetInfo 修改用户属性
    // 参数1: 服务器名 (NULL 表示本地计算机)
    // 参数2: 用户名
    // 参数3: 信息级别 (1008 专门用于修改用户标志)
    // 参数4: 指向 USER_INFO_1008 的指针
    // 参数5: 错误参数指针 (可选)
    nStatus = NetUserSetInfo(NULL, username.c_str(), 1008, (LPBYTE)&ui1008, NULL);

    if (nStatus == NERR_Success) {
        return true;
    } else {
        return false;
    }
}