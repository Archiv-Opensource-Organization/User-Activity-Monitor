//
// Created by Byakuya on 2026/8/8.
//

#include "Time.h"

#include <ctime>
#include <string>

const char* GetLocalTimeStr() {
    // 1. 获取当前时间戳
    std::time_t now = std::time(nullptr);

    // 2. 转换为本地时间结构体
    std::tm* local_tm = std::localtime(&now);

    // 3. 【关键】使用 static，让这个字符串在函数结束后依然存活！
    static std::string time_buffer;

    // 4. 格式化时间 (例如: "2026-08-08 16:19:52")
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local_tm);

    // 5. 赋值给静态字符串并返回 C 风格指针
    time_buffer = buffer;
    return time_buffer.c_str();
}
