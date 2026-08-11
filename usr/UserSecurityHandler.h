//
// Created by Work on 2026/8/4.
//

#ifndef LIBUSERACTIVITYMONITOR_USERSECURITYMONITOR_H
#define LIBUSERACTIVITYMONITOR_USERSECURITYMONITOR_H

#include <windows.h>
#include <winevt.h> // 必须包含 EVT API 头文件
#include <string>

class UserSecurityHandler {

public:
    static void ProcessSuccessEvent(EVT_HANDLE hEvent);

private:
    // Windows EVT 订阅句柄
    EVT_HANDLE m_hSubscription = NULL;

    // 静态回调函数 (Windows API 要求必须是静态的)
    static DWORD WINAPI EvtEventCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent);

};


#endif //LIBUSERACTIVITYMONITOR_USERSECURITYMONITOR_H
