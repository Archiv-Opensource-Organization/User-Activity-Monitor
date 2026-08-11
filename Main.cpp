#include <iostream>
#include <curl/curl.h>
#include <string>
#include <filesystem>

#include "Generic_Configuration.h"
#include "usr/aio_UserActivityHandler.h"
#include "runtime/windows/AdminRuntimeChecker.h"
#include "runtime/windows/WindowsScheduledTaskHandler.h"
#include "usr/utils/WindowsLogHelper.h"
#include "usr/log/LogToFile.h"
#include "usr/log/LogFilename.h"
#include "usr/time/Time.h"

#include "ProductInfo.h"

namespace fs = std::filesystem;

std::wstring getSiblingExePath(const std::wstring& targetExeName) {
    // 1. 获取当前 exe 的完整路径
    wchar_t buffer[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, buffer, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        throw std::runtime_error("获取当前模块路径失败");
    }

    fs::path currentExePath(buffer);
    fs::path parentDir = currentExePath.parent_path();

    // 3. 拼接目标 exe 名称
    fs::path targetPath = parentDir / targetExeName;

    return targetPath.wstring();
}

const char* ACTIVATE_ALL_USER_SCHEDULED_TASKNAME = "Activate All User Scheduled Task";

int main() {
    std::cout << "Running software version: " << VERSION << std::endl;

    std::string titleText = std::string(PRODUCT) + " " + VERSION;
    std::string shellCmd = "title " + titleText;
    system(shellCmd.c_str());

    // 1. 设置控制台编码，防止乱码或地址泄露
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    // 1. 检查权限
    if (!IsRunAsAdmin()) {
        system("color c0"); // 嘿嘿
        std::cerr << "[!] 警告：未以管理员身份运行，无法读取 Windows Security 日志！\n";
        std::cerr << "[!]                          连接 Windows 计划程序！\n";
        std::cerr << "请右键点击程序 -> 以管理员身份运行。\n";

        std::cout << "按回车键退出..." << std::endl;
        std::cin.get();
        return 1;
    }

    if (!WindowsScheduledTaskHandler::initialize()) {
        system("color c0"); // 嘿嘿
        std::cout << "按回车键退出..." << std::endl;
        std::cin.get();
        return 1;
    }

    log(mainProgram, "main", "System started at %s", GetLocalTimeStr());

    // 自动走一个激活所有账户
    // 1. 获取两个同级程序的路径
    std::wstring elevatorPath = getSiblingExePath(L"ArchivElevator.exe");
    std::wstring activatorPath = getSiblingExePath(L"UserActivator.exe");

    // 2. 拼接 ArchivElevator 需要的参数（记得给路径加双引号，防止路径中有空格）
    std::wstring activatorArgs = L"\"" + activatorPath + L"\"";

    std::wstring taskName = stringToWString(ACTIVATE_ALL_USER_SCHEDULED_TASKNAME);

    // 3. 检查并创建计划任务
    // 注意：这里把 ArchivElevator 作为执行目标，把 UserActivator 的路径作为参数传入
    if (!WindowsScheduledTaskHandler::taskExists(taskName)) {
        bool created = WindowsScheduledTaskHandler::createTask(
            taskName,
            elevatorPath,       // 执行 ArchivElevator.exe
            activatorArgs,      // 传入 UserActivator.exe 的路径作为参数
            30                  // 每30天执行一次
        );

        if (!created) {
            std::cerr << "[ERROR] 计划任务创建失败！请确保以管理员身份运行！" << std::endl;
        } else {
            std::cout << "[SUCCESS] 计划任务创建成功！" << std::endl;
        }
    }

    std::cout << "[系统] 启动附属组件：通过 ArchivElevator.exe UAC 运行 UserActivator.exe......" << std::endl;
    WindowsScheduledTaskHandler::runTaskNow(taskName);

    CURLcode global_ret = curl_global_init(CURL_GLOBAL_ALL);
    if (global_ret != CURLE_OK)
    {
        std::wcerr << "curl_global_init 失败\n";
        return 1;
    }

    std::cout << "[第三方库] Curl 初始化成功。" << std::endl;
    std::cout << "[系统] 正在初始化监控..." << std::endl;

    aio_UserActivityHandler handler;

    // 2. 开启监控（假设 StartMonitoring 内部已经处理了线程或异步逻辑）
    // 如果 StartMonitoring 本身是阻塞的，那就不需要这一步，直接看方案二
    handler.StartMonitoring(4625);
    handler.StartMonitoring(4624);

    std::cerr << "[系统] 本窗口因等待参数无法使用，请移步 ens_main_program.txt" << std::endl;

    // 2. 等待用户输入 (让程序在这里“活着”)
    std::cout << "[系统] 监控已启动！按回车键退出..." << std::endl;
    std::cin.get();

    // 3. 用户按回车后，再停止监控
    handler.StopMonitoring();

    // 全局清理
    curl_global_cleanup();
    return 0;
}

void WindowsScheduledTaskHandler::cleanup() {
    if (m_pService) {
        m_pService->Release();
        m_pService = nullptr;
    }
    if (m_initialized) {
        CoUninitialize();
        m_initialized = false;
    }
}