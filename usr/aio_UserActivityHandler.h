//
// Created by Byakuya on 2026/8/5.
//

#ifndef LIBUSERACTIVITYMONITOR_USER_ACTIVITY_HANDLER_H
#define LIBUSERACTIVITYMONITOR_USER_ACTIVITY_HANDLER_H

#include <windows.h>
#include <winevt.h>

class aio_UserActivityHandler {

public:
    void StartMonitoring(DWORD eventId);
    void StopMonitoring();
    static void handle_event(EVT_HANDLE hEvent);

private:
    EVT_HANDLE m_hSubscription = NULL;

    // 核心订阅方法
    void SubscribeToEvent(DWORD eventId, EVT_SUBSCRIBE_CALLBACK callback);

    // Windows API 要求的回调函数（必须是静态的）
    static DWORD WINAPI EvtEventCallback(
        EVT_SUBSCRIBE_NOTIFY_ACTION action,
        PVOID pContext,
        EVT_HANDLE hEvent
    );

};


#endif //LIBUSERACTIVITYMONITOR_USER_ACTIVITY_HANDLER_H
