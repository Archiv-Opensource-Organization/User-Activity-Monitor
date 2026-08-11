//
// Created by Byakuya on 2026/8/5.
//

#include "WindowsLogHelper.h"

#include <string>

#include "curl/multi.h"

using namespace std;

std::wstring ExtractXmlValue(const std::wstring& xml, const std::wstring& tagName) {
    // 1. 准备两种可能的格式：双引号和单引号
    std::wstring searchTagDouble = L"<Data Name=\"" + tagName + L"\">";
    std::wstring searchTagSingle = L"<Data Name='" + tagName + L"'>";

    size_t startPos = std::wstring::npos;
    size_t tagLength = 0;

    // 2. 先尝试找双引号格式
    startPos = xml.find(searchTagDouble);
    if (startPos != std::wstring::npos) {
        tagLength = searchTagDouble.length();
    } else {
        // 3. 如果没找到，再尝试找单引号格式
        startPos = xml.find(searchTagSingle);
        if (startPos != std::wstring::npos) {
            tagLength = searchTagSingle.length();
        }
    }

    // 4. 如果两种都没找到，说明该字段不存在，返回空
    if (startPos == std::wstring::npos) {
        return L"";
    }

    // 5. 跳过开始标签，寻找结束标签
    startPos += tagLength;
    size_t endPos = xml.find(L"</Data>", startPos);
    if (endPos == std::wstring::npos) return L"";

    // 6. 截取并返回中间的值
    return xml.substr(startPos, endPos - startPos);
}

// 专门用于提取 <System> 节点下的基础信息（如 EventID, Provider 等）
std::wstring ExtractSystemValue(const std::wstring& xml, const std::wstring& tagName) {
    std::wstring startTag = L"<" + tagName + L">";
    std::wstring endTag = L"</" + tagName + L">";

    size_t startPos = xml.find(startTag);
    if (startPos == std::wstring::npos) return L"";

    startPos += startTag.length();
    size_t endPos = xml.find(endTag, startPos);
    if (endPos == std::wstring::npos) return L"";

    return xml.substr(startPos, endPos - startPos);
}

std::wstring ExtractIpAddress(const std::wstring& xml) {
    std::wstring ip = ExtractXmlValue(xml, L"IpAddress");

    if (ip == L"-" || ip == L"::1" || ip == L"127.0.0.1") {
        return L"";
    }
    return ip;
}

std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

std::wstring stringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();

    // 1. 计算转换后需要的宽字符缓冲区大小
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    if (size_needed <= 0) return std::wstring();

    // 2. 分配空间并执行转换
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);

    return wstr;
}
