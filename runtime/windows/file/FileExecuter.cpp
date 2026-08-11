//
// Created by Byakuya on 2026/8/8.
//

#include "FileExecuter.h"

#include <cstdio>   // 用于 remove 和 rename
#include <fstream>
#include <windows.h>
#include <iostream>
#include <cstdarg>

using namespace std;

/**
 * 神秘注释
 * @param fileName 文件名
 * @param msg 消息
 * @param ...
 */
void add(const char* fileName, const char* msg, ...) {
    FILE* fp = fopen(fileName, "a");
    if (!fp) return;

    // 写入日志内容
    va_list args;
    va_start(args, msg);
    vfprintf(fp, msg, args);
    va_end(args);

    fprintf(fp, "\n");
    fflush(fp);
    fclose(fp);
}

void remove(const char* fileName, const char* targetLine) {
    const char* tempName = "temp_for_rw.dat"; // 临时文件路径

    std::ifstream inFile(fileName);
    if (!inFile.is_open()) return; // 文件不存在或打不开，直接返回

    std::ofstream outFile(tempName);
    if (!outFile.is_open()) {
        inFile.close();
        return;
    }

    std::string line;
    // 逐行读取原文件
    while (std::getline(inFile, line)) {
        if (line != targetLine) {
            outFile << line << std::endl;
        }
    }

    inFile.close();
    outFile.close();

    // 删除原文件，并用临时文件替换原文件
    std::remove(fileName);
    std::rename(tempName, fileName);
}

// 检查文件中是否包含指定的行/字符串
bool contains(const char* fileName, const char* targetLine) {
    std::ifstream inFile(fileName);
    if (!inFile.is_open()) return false; // 文件不存在或打不开，直接返回 false

    std::string line;
    // 逐行读取文件
    while (std::getline(inFile, line)) {
        // 只要找到一行完全匹配的，立刻返回 true，不再往下读（性能优化）
        if (line == targetLine) {
            inFile.close();
            return true;
        }
    }

    inFile.close();
    return false; // 读到文件末尾都没找到，返回 false
}