//
// Created by Work on 2026/8/4.
//

#ifndef LIBUSERACTIVITYMONITOR_IP_LOCATION_LOOKUP_H
#define LIBUSERACTIVITYMONITOR_IP_LOCATION_LOOKUP_H
#include <string>

static size_t c_curl_write_callback(void *contents, size_t size, size_t nmemb, std::string *s);

std::string getGeoLocation(const std::string& ip);

std::string getCountry(std::string ip);

class GeoIpLookup {
};


#endif //LIBUSERACTIVITYMONITOR_IP_LOCATION_LOOKUP_H
