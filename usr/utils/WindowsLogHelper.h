//
// Created by Byakuya on 2026/8/5.
//

#ifndef LIBUSERACTIVITYMONITOR_WINDOWS_LOG_HELPER_H
#define LIBUSERACTIVITYMONITOR_WINDOWS_LOG_HELPER_H

#include <string>

std::wstring stringToWString(const std::string& str);
std::string WStringToString(const std::wstring& wstr);
std::wstring ExtractIpAddress(const std::wstring& xml);
std::wstring ExtractXmlValue(const std::wstring& xml, const std::wstring& tagName);
std::wstring ExtractSystemValue(const std::wstring& xml, const std::wstring& tagName);

class WindowsLogHelper {
};


#endif //LIBUSERACTIVITYMONITOR_WINDOWS_LOG_HELPER_H
