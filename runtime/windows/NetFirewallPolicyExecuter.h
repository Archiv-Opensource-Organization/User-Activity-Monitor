//
// Created by Byakuya on 2026/8/6.
//

#ifndef LIBUSERACTIVITYMONITOR_NET_FIREWALL_EXECUTER_H
#define LIBUSERACTIVITYMONITOR_NET_FIREWALL_EXECUTER_H

#include <string>

bool blockIPInFirewall(const std::wstring& ipAddress, const std::wstring& ruleName);

class NetFirewallPolicyExecuter {
};


#endif //LIBUSERACTIVITYMONITOR_NET_FIREWALL_EXECUTER_H
