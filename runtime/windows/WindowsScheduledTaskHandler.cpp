#include "WindowsScheduledTaskHandler.h"
#include "../../usr/log/LogToFile.h"
#include "../../usr/log/LogFilename.h"
#include "../../usr/utils/WindowsLogHelper.h"
#include <windows.h>
#include <comdef.h>
#include <comip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

ITaskService* WindowsScheduledTaskHandler::m_pService = nullptr;
bool WindowsScheduledTaskHandler::m_initialized = false;

bool WindowsScheduledTaskHandler::initialize() {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        std::cerr << "[Windows] COM 初始化失败。" << std::endl;
        log(mainProgram, "WindowsScheduledTaskHandler::Initialize", "[ERROR] COM 初始化失败: 0x%X", hr);
        return false;
    }
    m_initialized = true;

    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&m_pService);
    if (FAILED(hr)) {
        std::cerr << "[Windows] COM - 计划事件组件初始化失败，因为无法创建任务计划实例。" << std::endl;
        log(mainProgram, "WindowsScheduledTaskHandler::Initialize", "[ERROR] 创建 TaskScheduler 实例失败: 0x%X", hr);
        return false;
    }

    // 连接到本地任务计划服务
    hr = m_pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        std::cerr << "[Windows] COM - 计划事件组件初始化失败，因为无法连接到任务计划服务。" << std::endl;
        log(mainProgram, "WindowsScheduledTaskHandler::Initialize", "[ERROR] 连接到任务计划服务失败: 0x%X", hr);
        return false;
    }

    std::cout << "[Windows] COM - 计划事件组件初始化成功。" << std::endl;
    return true;
}

bool WindowsScheduledTaskHandler::createTask(const std::wstring& taskName, const std::wstring& exePath, const std::wstring& args, int intervalDays) {
    if (!m_pService) return false;

    HRESULT hr;
    ITaskDefinition* pTask = NULL;
    hr = m_pService->NewTask(0, &pTask);
    if (FAILED(hr)) {
        log(mainProgram, "WindowsScheduledTaskHandler::CreateTask", "[ERROR] 创建新任务定义失败: 0x%X", hr);
        return false;
    }

    // 1. 先获取任务的 Principal 对象
    IPrincipal* pPrincipal = NULL;
    hr = pTask->get_Principal(&pPrincipal);
    if (SUCCEEDED(hr) && pPrincipal != NULL) {
        // 2. 设置运行级别为最高权限
        pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);

        pPrincipal->Release();
    }

    // 1. 设置注册信息
    IRegistrationInfo* pRegInfo = NULL;
    hr = pTask->get_RegistrationInfo(&pRegInfo);
    if (FAILED(hr) || pRegInfo == NULL) {
        log(mainProgram, "WindowsScheduledTaskHandler::CreateTask", "[ERROR] 获取 RegistrationInfo 失败: 0x%X", hr);
        pTask->Release();
        return false;
    }
    hr = pRegInfo->put_Description(_bstr_t(L"Archiv Security Monitoring System Auto-Scheduled Task"));
    pRegInfo->Release();
    if (FAILED(hr)) {
        log(mainProgram, "WindowsScheduledTaskHandler::CreateTask", "[ERROR] 设置任务描述失败: 0x%X", hr);
        pTask->Release();
        return false;
    }

    ITaskSettings* pSettings = NULL;
    pTask->get_Settings(&pSettings);

    // 👈 关键：允许按需启动
    pSettings->put_AllowDemandStart(VARIANT_TRUE);

    // 顺便加上：如果任务错过了时间，立刻执行（防止排队）
    pSettings->put_StartWhenAvailable(VARIANT_TRUE);

    // 2. 设置触发器（每天执行）
    ITriggerCollection* pTriggerCollection = NULL;
    hr = pTask->get_Triggers(&pTriggerCollection);
    if (FAILED(hr) || pTriggerCollection == NULL) {
        log(mainProgram, "WindowsScheduledTaskHandler::CreateTask", "[ERROR] 获取 Triggers 集合失败: 0x%X", hr);
        pTask->Release();
        return false;
    }

    ITrigger* pTrigger = NULL;
    hr = pTriggerCollection->Create(TASK_TRIGGER_DAILY, &pTrigger);
    if (FAILED(hr) || pTrigger == NULL) {
        log(mainProgram, "WindowsScheduledTaskHandler::CreateTask", "[ERROR] 创建 Trigger 失败: 0x%X", hr);
        pTriggerCollection->Release();
        pTask->Release();
        return false;
    }

    IDailyTrigger* pDailyTrigger = NULL;
    hr = pTrigger->QueryInterface(IID_IDailyTrigger, (void**)&pDailyTrigger);
    if (FAILED(hr) || pDailyTrigger == NULL) {
        log(mainProgram, "WindowsScheduledTaskHandler::CreateTask", "[ERROR] QueryInterface for IDailyTrigger 失败: 0x%X", hr);
        pTrigger->Release();
        pTriggerCollection->Release();
        pTask->Release();
        return false;
    }

    std::wstring authorName = L"Archiv Technology Co,.Ltd";
    hr = pRegInfo->put_Author(_bstr_t(authorName.c_str()));
    if (FAILED(hr)) {
        log(mainProgram, "WindowsScheduledTaskHandler::CreateTask", "[ERROR] 设置任务创建者失败: 0x%X", hr);
        // 创建者不是核心配置，失败可以不阻断流程，但建议记录日志
    }

    {
        SYSTEMTIME sysTime;
        GetLocalTime(&sysTime);

        wchar_t timeStr[64];
        swprintf_s(timeStr, L"%04d-%02d-%02dT%02d:%02d:%02d",
                   sysTime.wYear, sysTime.wMonth, sysTime.wDay,
                   sysTime.wHour, sysTime.wMinute, sysTime.wSecond);

        // 设置 StartBoundary
        hr = pDailyTrigger->put_StartBoundary(_bstr_t(timeStr));
        if (FAILED(hr)) {
            log(mainProgram, "WindowsScheduledTaskHandler::CreateTask", "[ERROR] 设置触发器开始时间失败: 0x%X", hr);
            pDailyTrigger->Release();
            pTrigger->Release();
            pTriggerCollection->Release();
            pTask->Release();
            return false;
        }
    }

    pDailyTrigger->put_DaysInterval((SHORT)intervalDays);
    pDailyTrigger->Release();
    pTrigger->Release();
    pTriggerCollection->Release();

    // 3. 设置执行动作
    IActionCollection* pActionCollection = NULL;
    hr = pTask->get_Actions(&pActionCollection);
    if (FAILED(hr) || pActionCollection == NULL) {
        log(mainProgram, "WindowsScheduledTaskHandler::CreateTask", "[ERROR] 获取 Actions 集合失败: 0x%X", hr);
        pTask->Release();
        return false;
    }

    IAction* pAction = NULL;
    hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
    if (FAILED(hr) || pAction == NULL) {
        log(mainProgram, "WindowsScheduledTaskHandler::CreateTask", "[ERROR] 创建 Action 失败: 0x%X", hr);
        pActionCollection->Release();
        pTask->Release();
        return false;
    }

    IExecAction* pExecAction = NULL;
    hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
    if (FAILED(hr) || pExecAction == NULL) {
        log(mainProgram, "WindowsScheduledTaskHandler::CreateTask", "[ERROR] QueryInterface for IExecAction 失败: 0x%X", hr);
        pAction->Release();
        pActionCollection->Release();
        pTask->Release();
        return false;
    }
    pExecAction->put_Path(_bstr_t(exePath.c_str()));
    if (!args.empty()) {
        pExecAction->put_Arguments(_bstr_t(args.c_str()));
    }
    pExecAction->Release();
    pAction->Release();
    pActionCollection->Release();

    // 4. 注册任务（使用最高权限 SYSTEM 账户运行）
    ITaskFolder* pRootFolder = NULL;
    hr = m_pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr) || pRootFolder == NULL) {
        log(mainProgram, "WindowsScheduledTaskHandler::CreateTask", "[ERROR] 获取根文件夹失败: 0x%X", hr);
        pTask->Release();
        return false;
    }

    IRegisteredTask* pRegisteredTask = NULL;
    hr = pRootFolder->RegisterTaskDefinition(
        _bstr_t(taskName.c_str()),
        pTask,
        TASK_CREATE_OR_UPDATE,
        _variant_t(L"SYSTEM"), // 以 SYSTEM 身份运行
        _variant_t(),          // 无密码
        TASK_LOGON_SERVICE_ACCOUNT,
        _variant_t(L""),
        &pRegisteredTask
    );

    // 释放局部资源
    if (pRegisteredTask) pRegisteredTask->Release();
    pRootFolder->Release();
    pTask->Release();

    // 检查最终结果
    if (FAILED(hr)) {
        log(mainProgram, "WindowsScheduledTaskHandler::CreateTask", "[ERROR] 注册任务失败! 错误码: 0x%X", hr);
        return false;
    }

    log(mainProgram, "WindowsScheduledTaskHandler::CreateTask", "[INFO] 计划任务 [%s] 创建成功", WStringToString(taskName).c_str());
    return true;
}

bool WindowsScheduledTaskHandler::taskExists(const std::wstring& taskName) {
    if (!m_pService) return false;
    ITaskFolder* pRootFolder = NULL;
    HRESULT hr = m_pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) return false;

    IRegisteredTask* pRegisteredTask = NULL;
    hr = pRootFolder->GetTask(_bstr_t(taskName.c_str()), &pRegisteredTask);
    pRootFolder->Release();

    if (SUCCEEDED(hr)) {
        if (pRegisteredTask) pRegisteredTask->Release();
        return true;
    }
    return false;
}

bool WindowsScheduledTaskHandler::deleteTask(const std::wstring& taskName) {
    if (!m_pService) return false;
    ITaskFolder* pRootFolder = NULL;
    HRESULT hr = m_pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) return false;

    hr = pRootFolder->DeleteTask(_bstr_t(taskName.c_str()), 0);
    pRootFolder->Release();

    if (SUCCEEDED(hr)) {
        log(mainProgram, "WindowsScheduledTaskHandler::DeleteTask", "[INFO] 计划任务 [%s] 已删除", WStringToString(taskName).c_str());
        return true;
    }
    return false;
}

bool WindowsScheduledTaskHandler::runTaskNow(const std::wstring& taskName) {
    if (!m_pService) return false;
    ITaskFolder* pRootFolder = NULL;
    HRESULT hr = m_pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) return false;

    IRegisteredTask* pRegisteredTask = NULL;
    hr = pRootFolder->GetTask(_bstr_t(taskName.c_str()), &pRegisteredTask);

    // ✅ 修改：必须先检查 GetTask 是否成功
    if (SUCCEEDED(hr) && pRegisteredTask != NULL) {
        IRunningTask* pRunningTask = NULL;
        hr = pRegisteredTask->Run(_variant_t(), &pRunningTask);
        if (pRunningTask) pRunningTask->Release();
        pRegisteredTask->Release();
        pRootFolder->Release();

        if (SUCCEEDED(hr)) return true;
        else {
            log(mainProgram, "WindowsScheduledTaskHandler::RunTaskNow", "[ERROR] 运行任务失败: 0x%X", hr);
            return false;
        }
    } else {
        // ✅ 修改：如果任务不存在，记录日志而不是直接崩溃
        log(mainProgram, "WindowsScheduledTaskHandler::RunTaskNow", "[ERROR] 找不到任务，无法运行: %s", WStringToString(taskName).c_str());
        pRootFolder->Release();
        return false;
    }
}