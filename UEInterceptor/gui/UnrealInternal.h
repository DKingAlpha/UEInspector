#pragma once
#include "UObject/UnrealType.h"
#include <string>
#include <vector>
#include <functional>
#include <assert.h>
#include "../mem/String.hpp"

// 4.20
#define UOBJECT_PROCESSEVENT_INDEX 0x41

// 4.23?
// #define UOBJECT_PROCESSEVENT_INDEX 0x40


template <typename T, typename T2> T DCast(T2 In) {
    if (In && ((UObjectBaseUtility*)In)->IsA<std::remove_pointer_t<T>>()) {
        return (T)In;
    }

    return nullptr;
}


FUObjectArray* get_GUObjectArray();
std::tuple<std::string, std::string, std::string> get_full_name(UObjectBase* obj);
UObjectBase* find_object(const char* type_name, const char* obj_full_name, bool cache=true);
std::string uprop_to_str(UProperty* uprop);
std::string uprop_type_to_str(UProperty* fprop);
std::string narrow(const FString& fstr);
std::string narrow(const FName& fname);

inline uintptr_t GetVirtualFunctionAt(void* pObj, int index) {
    uintptr_t* pAddr = reinterpret_cast<uintptr_t*>(pObj);
    pAddr = (uintptr_t*)*pAddr;  // Gets a pointer to the virtual function table 
    return pAddr[index];
}


struct UFuncParamInfo {
    UProperty* param;
    bool is_ptr_or_ref;
    std::string type;
    std::string name;
};

class UFunctionInvoker
{
public:
    UFunctionInvoker(UFunction* ufunc);

    operator bool() const { return m_is_valid; }

    UFunction* m_f;

    bool m_is_static;
    bool m_has_returntype;
    UFuncParamInfo m_return_type;
    std::vector<UFuncParamInfo> m_arguments;

    bool m_is_valid;
};


class DynProperty {
public:
    std::vector<unsigned char> data;

    EClassCastFlags cast_flags = CASTCLASS_None;
    bool is_numeric = false;
    bool is_signed = false;
    int max_bytelen = 0;

    UProperty* m_prop = nullptr;

    static inline uint64_t s_to_u64(std::string s)
    {
        assert(s.size());
        if (s[0] == '0' && s[1] == 'x') {
            return static_cast<uint64_t>(std::stoull(s, nullptr, 16));
        }
        else {
            return static_cast<uint64_t>(std::stoull(s, nullptr, 10));
        }
    }

    static inline int64_t s_to_s64(std::string s)
    {
        assert(s.size());
        if (s[0] == '0' && s[1] == 'x') {
            return static_cast<uint64_t>(std::stoll(s, nullptr, 16));
        }
        else {
            return static_cast<uint64_t>(std::stoll(s, nullptr, 10));
        }
    }

    bool Parse(std::string s) {
        if (!s.size()) {
            return false;
        }
        if (!m_prop) {
            return false;
        }
        if (auto* p = DCast<UNumericProperty*>(m_prop)) {
            union {
                float f;
                double d;
                uint64_t u64;
                int64_t s64;
                // little endian. u64 is enough
            } v;
            v.u64 = 0;
            const unsigned char* pv = reinterpret_cast<const unsigned char*>(&v);
            if (p->IsInteger()) {
                if (DCast<UInt8Property*>(p) || DCast<UInt16Property*>(p) || DCast<UIntProperty*>(p)
                    || DCast<UInt64Property*>(p)) {
                    v.s64 = s_to_s64(s);
                }
                else {
                    v.u64 = s_to_u64(s);
                }
            }
            else {
                if (auto p1 = DCast<UDoubleProperty*>(p)) {
                    v.d = std::stod(s);
                }
                else if (auto p1 = DCast<UFloatProperty*>(p)) {
                    v.f = std::stof(s);
                }
                else {
                    assert(false);
                }
            }
            if (m_prop->GetSize() < sizeof(v) && pv[m_prop->GetSize()] != 0) {
                // quick&dirty check of out-of-range
                return false;
            }
            data.resize(m_prop->GetSize());
            memset(data.data(), 0, data.size());
            memcpy(data.data(), pv, sizeof(data.size()));
            return true;
        }
        else if (auto* p = DCast<UBoolProperty*>(m_prop)) {
            if (s[0] == '0' || s[0] == 'f' || s[0] == 'F') {
                data.push_back(0);
            }
            else {
                data.push_back(1);
            }
            return true;
        }
        else if (IsPtrOrRef()) {
            uintptr_t ptr = static_cast<uintptr_t>(std::stoull(s));
            data.resize(sizeof(ptr));
            memset(data.data(), 0, data.size());
            memcpy(data.data(), &ptr, sizeof(data.size()));
            return true;
        }
        return false;
    }

    std::string ToString() {
        if (!data.size()) {
            return "";
        }
        if (!m_prop) {
            return "";
        }
        char buf[64]{ 0 };
        if (auto* p = DCast<UNumericProperty*>(m_prop)) {

            if (p->IsInteger()) {
                uint64_t sbuf = 0;
                memcpy(&sbuf, data.data(), sizeof(data.size()));
                if (DCast<UInt8Property*>(p) || DCast<UInt16Property*>(p) || DCast<UIntProperty*>(p)
                    || DCast<UInt64Property*>(p)) {
                    snprintf(buf, sizeof(buf), "%lld", *(int64_t*)&sbuf);
                }
                else {
                    snprintf(buf, sizeof(buf), "%llu", *(uint64_t*)&sbuf);
                }
            }
            else {

                if (auto p1 = DCast<UDoubleProperty*>(p)) {
                    snprintf(buf, sizeof(buf), "%lf", *(double*)data.data());
                }
                else if (auto p1 = DCast<UFloatProperty*>(p)) {
                    snprintf(buf, sizeof(buf), "%f", *(float*)data.data());
                }
                else {
                    assert(false);
                }
            }
        }
        else if (auto* p = DCast<UBoolProperty*>(m_prop)) {
            return data[0] ? "true" : "false";
        }
        else if (IsPtrOrRef() || IsRetval()) {
            return kanan::stringFormat("%p", data.data());
        }
        return buf;
    }

    explicit DynProperty(UProperty* uprop)
    {
        Load(uprop);
    }

    explicit DynProperty()
    {
        Load(nullptr);
    }

    void Load(UProperty* uprop)
    {
        m_prop = uprop;
    }

    bool IsPtrOrRef()
    {
        if (!m_prop) {
            return false;
        }
        auto param_flags = m_prop->PropertyFlags;
        auto param_type = uprop_type_to_str(m_prop);
        auto param_name = narrow(m_prop->GetName());
        if ((param_flags & CPF_ReferenceParm) || (param_flags & CPF_OutParm)) {
            param_type += "&";
            return true;
        }
        if (param_type.rfind("*") != param_type.npos || param_name.rfind("*") != param_name.npos) {
            return true;
        }
        return false;
    }

    bool IsRetval()
    {
        return bool(m_prop->PropertyFlags & CPF_ReturnParm);
    }
};