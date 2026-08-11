//
// Created by Byakuya on 2026/8/6.
//

#include "NetFirewallPolicyExecuter.h"

#include <windows.h>
#include <netfw.h>     // Windows 防火墙 API
#include <comutil.h>   // 用于 _bstr_t 等字符串转换
#include <string>
#include <algorithm>

#include "../../usr/utils/WindowsLogHelper.h"
#include "../../usr/log/LogFilename.h"
#include "../../usr/log/LogToFile.h"

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "comsuppw.lib")

#include "../../Generic_Configuration.h"

using namespace std;

// 辅助函数：宽字符串转小写（用于不区分大小写比较）
static std::wstring toLower(const std::wstring& s) {
    std::wstring result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}

bool ruleExists(const std::wstring& ipAddress) {
    HRESULT hr;
    INetFwPolicy2* pNetFwPolicy2 = nullptr;
    INetFwRules* pFwRules = nullptr;
    IEnumVARIANT* pEnumerator = nullptr;
    bool isBlocked = false;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    // 注意：如果主程序已初始化COM，这里可能会返回S_FALSE，不算错误
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return false;

    hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER,
                          __uuidof(INetFwPolicy2), (void**)&pNetFwPolicy2);

    if (SUCCEEDED(hr) && pNetFwPolicy2) {
        hr = pNetFwPolicy2->get_Rules(&pFwRules);
        if (SUCCEEDED(hr) && pFwRules) {
            IUnknown* pUnk = nullptr;
            hr = pFwRules->get__NewEnum(&pUnk);
            if (SUCCEEDED(hr) && pUnk) {
                hr = pUnk->QueryInterface(__uuidof(IEnumVARIANT), (void**)&pEnumerator);
                pUnk->Release();
            }

            if (SUCCEEDED(hr) && pEnumerator) {
                VARIANT variant;
                VariantInit(&variant);
                ULONG fetched = 0;

                // 遍历所有规则
                while (pEnumerator->Next(1, &variant, &fetched) == S_OK && fetched > 0) {
                    if (variant.vt == VT_DISPATCH) {
                        INetFwRule* pRule = nullptr;
                        hr = variant.pdispVal->QueryInterface(__uuidof(INetFwRule), (void**)&pRule);
                        if (SUCCEEDED(hr) && pRule) {
                            // 1. 获取 RemoteAddresses
                            BSTR bstrRemoteAddr = nullptr;
                            hr = pRule->get_RemoteAddresses(&bstrRemoteAddr);
                            if (SUCCEEDED(hr) && bstrRemoteAddr) {
                                std::wstring currentAddr(bstrRemoteAddr);
                                SysFreeString(bstrRemoteAddr);

                                // 2. 比较 IP (简单包含匹配，因为可能是单个IP或列表)
                                // 注意：这里只是简单示例，严谨起见应分割字符串比对
                                if (currentAddr.find(ipAddress) != std::wstring::npos) {
                                    // 进一步确认动作是 BLOCK 且启用状态
                                    NET_FW_ACTION action;
                                    VARIANT_BOOL enabled;
                                    if (SUCCEEDED(pRule->get_Action(&action)) && action == NET_FW_ACTION_BLOCK &&
                                        SUCCEEDED(pRule->get_Enabled(&enabled)) && enabled == VARIANT_TRUE) {
                                        isBlocked = true;
                                        pRule->Release();
                                        break;
                                    }
                                }
                            }
                            pRule->Release();
                        }
                    }
                    VariantClear(&variant);
                    if (isBlocked) break;
                }
                pEnumerator->Release();
            }
        }
        if(pFwRules) pFwRules->Release();
        pNetFwPolicy2->Release();
    }
    CoUninitialize();
    return isBlocked;
}

bool blockIPInFirewall(const std::wstring& ipAddress, const std::wstring& ruleName) {
    if (!ruleExists(ipAddress)) {
        std::string logMsg = "[INFO] 没有检测到 " + WStringToString(ipAddress) + " 的入站规则，开始新建......";
        log(mainProgram, "NetFirewallPolicyExecuter::blockIPInFirewall", logMsg.c_str());

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

        pFwRule->put_Protocol((NET_FW_IP_PROTOCOL)256);

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
    } else {
        std::string logMsg = "[INFO] 检测到 " + WStringToString(ipAddress) + " 的入站规则，放弃。";
        log(mainProgram, "NetFirewallPolicyExecuter::blockIPInFirewall", logMsg.c_str());
    }
}