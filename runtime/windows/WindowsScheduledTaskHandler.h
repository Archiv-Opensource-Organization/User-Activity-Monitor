//
// Created by Byakuya on 2026/8/8.
//

#ifndef LIBUSERACTIVITYMONITOR_WINDOWSSCHEDULEDTASKHANDLER_H
#define LIBUSERACTIVITYMONITOR_WINDOWSSCHEDULEDTASKHANDLER_H
#include <string>
#include <taskschd.h>


class WindowsScheduledTaskHandler {

public:
    WindowsScheduledTaskHandler() = default;
    ~WindowsScheduledTaskHandler() = default;

    // 初始化 COM 环境（必须在主线程或线程初始化时调用）
    static bool initialize();

    static void cleanup();

    // 1. 创建计划任务
    // taskName: 任务名称 (如 "MySecurityCleaner")
    // exePath: 要执行的程序绝对路径
    // args: 命令行参数 (可选)
    // intervalDays: 执行间隔天数 (1表示每天)
    static bool createTask(const std::wstring& taskName, const std::wstring& exePath,
                    const std::wstring& args = L"", int intervalDays = 1);

    // 2. 查询计划任务是否存在
    static bool taskExists(const std::wstring& taskName);

    // 3. 删除计划任务
    static bool deleteTask(const std::wstring& taskName);

    // 4. 手动触发一次任务（可选功能）
    static bool runTaskNow(const std::wstring& taskName);

private:
    static ITaskService* m_pService;
    static bool m_initialized;
};


#endif //LIBUSERACTIVITYMONITOR_WINDOWSSCHEDULEDTASKHANDLER_H
