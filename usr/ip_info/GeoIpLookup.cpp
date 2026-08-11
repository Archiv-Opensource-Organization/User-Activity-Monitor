//
// Created by Work on 2026/8/4.
//

#include "GeoIpLookup.h"

#include <iostream>
#include <ostream>
#include <string>
#include <curl/curl.h>

// curl接收回调
static size_t c_curl_write_callback(void *contents, size_t size, size_t nmemb, std::string *s)
{
    size_t newLength = size * nmemb;
    try
    {
        s->append((char*)contents, newLength);
    }
    catch (std::bad_alloc &e)
    {
        return 0;
    }
    return newLength;
}

std::string getGeoLocation(const std::string& ip)
{
    if(ip.empty())
    {
        return "未知位置";
    }

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        return "未知位置";
    }

    std::string response;
    // 使用公开免费接口，返回简单json；注意：不要高频大量调用，适合告警少量查询
    std::string url = "http://ip-api.com/json/" + ip + "?fields=status,country,regionName,city,isp";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);        //5秒超时，防止卡住告警邮件
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, c_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if(res != CURLE_OK || response.empty())
    {
        return "未知位置";
    }

    // 简易解析json（不引入完整json库，轻量截取字符串）
    auto get_val = [&](const std::string& key)->std::string
    {
        size_t pos = response.find("\"" + key + "\":");
        if(pos == std::string::npos) return "";
        pos += key.length() + 3;
        if(response[pos] == '"') pos++;
        size_t end = response.find('"', pos);
        if(end == std::string::npos) return "";
        return response.substr(pos, end - pos);
    };

    std::string status = get_val("status");
    if(status != "success")
    {
        return "未知位置";
    }

    std::string country  = get_val("country");
    std::string province = get_val("regionName");
    std::string city     = get_val("city");
    std::string isp      = get_val("isp");

    std::string loc;
    if(!country.empty()) loc += country;
    if(!province.empty()) loc += " " + province;
    if(!city.empty()) loc += " " + city;
    if(!isp.empty()) loc += " | " + isp;

    std::cout << loc << std::endl;

    if(loc.empty())
    {
        loc = "未知位置";
    }
    return loc;
}

std::string getCountry(std::string ip)
{
    if(ip.empty())
    {
        return "未知位置";
    }

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        return "未知位置";
    }

    std::string response;
    // 使用公开免费接口，返回简单json；注意：不要高频大量调用，适合告警少量查询
    std::string url = "http://ip-api.com/json/" + ip + "?fields=status,country,regionName,city,isp";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);        //5秒超时，防止卡住告警邮件
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, c_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if(res != CURLE_OK || response.empty())
    {
        return "未知位置";
    }

    // 简易解析json（不引入完整json库，轻量截取字符串）
    auto get_val = [&](const std::string& key)->std::string
    {
        size_t pos = response.find("\"" + key + "\":");
        if(pos == std::string::npos) return "";
        pos += key.length() + 3;
        if(response[pos] == '"') pos++;
        size_t end = response.find('"', pos);
        if(end == std::string::npos) return "";
        return response.substr(pos, end - pos);
    };

    std::string status = get_val("status");
    if(status != "success")
    {
        return "未知位置";
    }

    std::string country  = get_val("country");

    std::cout << ip + " 的所属国家为 " + country + "。" << std::endl;

    return country;
}