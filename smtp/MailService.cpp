//
// Created by Work on 2026/8/4.
//

#include "MailService.h"

#include <iostream>
#include <string>
#include <curl/curl.h>

#include "../usr/ip_info/GeoIpLookup.h"
#include "MailStringAssistant.h"
#include "SMTPConfiguration.h"

using namespace std;

string build_alert_login_attempted_email(const string& username, const string& ip) {
    string subject = "[Archiv Server Security] 检测到可疑登录活动";

    string html_body = openFiletoString("smtp/templates/zh_cn_alert_suspicious_login_template.html");

    replaceAll(html_body, "##USERNAME##", username);
    replaceAll(html_body, "##IPADDR##", ip);
    // replaceAll(html_body, "##GEOLOCATION##", getGeoLocation(ip));

    // 组装HTML邮件报文
    std::string mail_payload =
"From: Archiv Security Monitoring System (no-reply) <" + SMTP_FROM_EMAIL + ">\r\n"
"To: Server Administrator <" + SMTP_TO_EMAIL + ">\r\n"
"Subject: " + subject + "\r\n"
"Content-Type: text/html; charset=utf-8\r\n"
"X-Mailer: ASSC\r\n"
"\r\n" + html_body + "\r\n";

    return mail_payload;
}

string build_alert_login_proceeded_email(const string& username, const string& ip) {
    string subject = "[Archiv Server Security] 检测到登录活动";

    string html_body = openFiletoString("smtp/templates/zh_cn_alert_login_proceeded_template.html");

    replaceAll(html_body, "##USERNAME##", username);
    replaceAll(html_body, "##IPADDR##", ip);
    replaceAll(html_body, "##GEOLOCATION##", getGeoLocation(ip));

    // 组装HTML邮件报文
    std::string mail_payload =
"From: Archiv Security Monitoring System (no-reply) <" + SMTP_FROM_EMAIL + ">\r\n"
"To: Server Administrator <" + SMTP_TO_EMAIL + ">\r\n"
"Subject: " + subject + "\r\n"
"Content-Type: text/html; charset=utf-8\r\n"
"X-Mailer: ASSC\r\n"
"\r\n" + html_body + "\r\n";

    return mail_payload;
}

size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp) {
    std::string* data = static_cast<std::string*>(userp);
    if (data->empty()) return 0;

    // size_t copy_len = std::min(size * nmemb, data->size());
    size_t max_copy = size * nmemb;
    size_t str_len = data->size();
    size_t copy_len = (max_copy < str_len) ? max_copy : str_len;

    memcpy(ptr, data->data(), copy_len);
    data->erase(0, copy_len);
    return copy_len;
}

/**
 * 发送邮件
 * @param email_type 邮件类型
 * 0为尝试登录，1为登录成功
 * @param username
 * @param ip
 * @return
 */
bool send_email(int email_type, const std::string& username, const std::string& ip) {
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "[错误] CURL 初始化失败\n";
        return false;
    }

    std::cout << "Loaded curl: " << curl_version() << std::endl;

    std::string mail_payload;

    switch (email_type) {
        case 0:
            mail_payload = build_alert_login_attempted_email(username, ip);
            break;
        case 1:
            mail_payload = build_alert_login_proceeded_email(username, ip);
            break;
    }

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(curl, CURLOPT_URL, SMTP_SERVER.c_str());

    curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN");

    curl_easy_setopt(curl, CURLOPT_USERNAME, SMTP_AUTH_USER.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, SMTP_AUTH_PASS.c_str());

    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, SMTP_FROM_EMAIL.c_str());

    struct curl_slist* recipients = nullptr;
    recipients = curl_slist_append(recipients, SMTP_TO_EMAIL.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_READDATA, &mail_payload);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

    // 调试临时关闭校验，生产务必删掉，替换为CURLOPT_CAINFO
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    bool ok = (res == CURLE_OK);

    if (!ok)
        std::cerr << "[错误] 邮件发送失败：" << curl_easy_strerror(res) << "\n";
    else
        std::cout << "[成功] 邮件发送完成！\n";

    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
    return ok;
}
