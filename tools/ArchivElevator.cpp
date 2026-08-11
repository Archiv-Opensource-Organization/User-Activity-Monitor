//
// Created by Byakuya on 2026/8/10.
//
#include <windows.h>
#include <shlwapi.h>
#include <string>

#pragma comment(lib, "shlwapi.lib")

// 使用最标准的 main 入口，彻底告别 WinMain 和 wmain 的烦恼
int main(int argc, char* argv[]) {
    std::wstring targetPath;

    // 1. 使用 Windows API 获取完整的宽字符命令行
    // 因为 main 接收的是窄字符，而我们需要支持中文路径，所以必须用这个
    int wArgc;
    LPWSTR* wArgv = CommandLineToArgvW(GetCommandLineW(), &wArgc);

    // 2. 提取第一个参数（即传入的目标程序路径）
    // wArgv[0] 是 ArchivElevator.exe 本身，wArgv[1] 才是我们需要的参数
    if (wArgc > 1 && wArgv != NULL) {
        targetPath = wArgv[1];
    }

    // 3. 释放 CommandLineToArgvW 分配的内存
    if (wArgv != NULL) {
        LocalFree(wArgv);
    }

    // 4. 去除可能存在的首尾引号（防止路径带空格）
    if (targetPath.size() >= 2 && targetPath.front() == L'"' && targetPath.back() == L'"') {
        targetPath = targetPath.substr(1, targetPath.size() - 2);
    }

    // 5. 没传路径，给出提示并退出
    if (targetPath.empty()) {
        MessageBoxW(NULL, L"用法: ArchivElevator.exe \"<目标程序的绝对路径>\"", L"ArchivElevator", MB_ICONINFORMATION);
        return 1;
    }

    // 6. 提取目标程序所在的父目录，作为工作目录
    std::wstring workDir = targetPath;
    PathRemoveFileSpecW(&workDir[0]);
    workDir.resize(wcslen(workDir.c_str()));

    // 7. 使用 "runas" 动词强制触发 UAC 提权运行
    HINSTANCE hRet = ShellExecuteW(
        NULL,
        L"runas",               // 强制请求管理员权限
        targetPath.c_str(),     // 目标程序路径
        NULL,                   // 无额外参数
        workDir.c_str(),        // 工作目录
        SW_SHOWNORMAL           // 正常显示窗口
    );

    // 8. 检查启动是否成功
    if ((intptr_t)hRet <= 32) {
        std::wstring errMsg = L"无法以管理员身份启动目标程序！\n\n";

        if ((intptr_t)hRet == 5) {
            errMsg += L"原因：您在 UAC 弹窗中点击了“否”。";
        } else if ((intptr_t)hRet == 2) {
            errMsg += L"原因：找不到指定的文件。\n路径: " + targetPath;
        } else {
            errMsg += L"未知错误码: " + std::to_wstring((intptr_t)hRet);
        }

        MessageBoxW(NULL, errMsg.c_str(), L"ArchivElevator Error", MB_ICONERROR);
        return 1;
    }

    return 0;
}