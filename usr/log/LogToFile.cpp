//
// Created by Byakuya on 2026/8/6.
//
#include "LogToFile.h"

#include <windows.h>
#include <iostream>
#include <string>
#include <cstdarg>

#include "../time/Time.h"

using namespace std;

/**
 * 神秘注释
 * @param fileName 文件名
 * @param implementer 来源cpp文件
 * @param msg 消息
 * @param ...
 */
void log(const char* fileName, const char* implementer, const char* msg, ...) {
    FILE* fp = fopen(fileName, "a");
    if (!fp) return;

    // 直接调用，获取带有时间的 const char*
    const char* currentTime = GetLocalTimeStr();

    // 写入时间
    fprintf(fp, "[%s] ", currentTime);

    // 写入实现者
    if (implementer) {
        fprintf(fp, "[%s] ", implementer);
    }

    // 写入日志内容
    va_list args;
    va_start(args, msg);
    vfprintf(fp, msg, args);
    va_end(args);

    fprintf(fp, "\n");
    fflush(fp);
    fclose(fp);
}