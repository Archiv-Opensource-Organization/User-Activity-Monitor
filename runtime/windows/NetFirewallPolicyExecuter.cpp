//
// Created by Byakuya on 2026/8/6.
//

#include "NetFirewallPolicyExecuter.h"

#include <windows.h>
#include <netfw.h>     // Windows 防火墙 API
#include <comutil.h>   // 用于 _bstr_t 等字符串转换
#include <string>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "comsuppw.lib")

#include "../../Generic_Configuration.h"

using namespace std;

bool blockIPInFirewall(const std::wstring& ipAddress, const std::wstring& ruleName) {
    HRESULT hr;
    INetFwPolicy2* pNetFwPolicy2 = nullptr;
    INetFwRule* pFwRule = nullptr;

    hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return false;

    hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pNetFwPolicy2));
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    hr = CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFwRule));
    if (FAILED(hr)) {
        pNetFwPolicy2->Release();
        CoUninitialize();
        return false;
    }

    pFwRule->put_Name(_bstr_t(ruleName.c_str()));
    pFwRule->put_Description(_bstr_t(L"Auto-blocked by Archiv Security Monitoring System"));

    pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_TCP);
    pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_UDP);

    pFwRule->put_RemoteAddresses(_bstr_t(ipAddress.c_str())); // 指定远程 IP
    pFwRule->put_Direction(NET_FW_RULE_DIR_IN);    // 入站规则
    pFwRule->put_Action(NET_FW_ACTION_BLOCK);      // 【核心】动作为阻止
    pFwRule->put_Enabled(VARIANT_TRUE);            // 启用规则
    pFwRule->put_Profiles(NET_FW_PROFILE2_ALL);    // 应用于所有网络配置文件

    // 5. 将规则添加到防火墙策略中
    // 1. 先通过 get_Rules 获取规则集合对象
    INetFwRules* pFwRules = nullptr;
    HRESULT hrRules = pNetFwPolicy2->get_Rules(&pFwRules);

    if (SUCCEEDED(hrRules) && pFwRules != nullptr) {
        hr = pFwRules->Add(pFwRule);
    } else {
        hr = hrRules; // 如果获取规则集合失败，把错误码赋给 hr
    }

    // 6. 释放 COM 资源
    pFwRule->Release();
    pNetFwPolicy2->Release();
    CoUninitialize();

    return SUCCEEDED(hr);
}