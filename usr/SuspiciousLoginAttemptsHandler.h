//
// Created by Work on 2026/8/4.
//

#ifndef LIBUSERACTIVITYMONITOR_SUSPICIOUSLOGINATTEMPTSMONITOR_H
#define LIBUSERACTIVITYMONITOR_SUSPICIOUSLOGINATTEMPTSMONITOR_H

#pragma once

#include <windows.h>
#include <winevt.h> // 必须包含 EVT API 头文件
#include <string>

class SuspiciousLoginAttemptsHandler {

public:
    // 核心监听接口
    static void ProcessEventXml(EVT_HANDLE hEvent);

private:
    // Windows EVT 订阅句柄
    EVT_HANDLE m_hSubscription = NULL;
    static DWORD WINAPI SusEvtEventCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent);
};


#endif //LIBUSERACTIVITYMONITOR_SUSPICIOUSLOGINATTEMPTSMONITOR_H
