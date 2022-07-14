#pragma once
#include "UObject/UnrealType.h"
#include <string>
#include <vector>

#define UOBJECT_PROCESSEVENT_INDEX 0x40

template <typename T, typename T2 = UObject*> T DCast(T2 In) {
    if (In && ((UObjectBaseUtility*)In)->IsA<std::remove_pointer_t<T>>()) {
        return (T)In;
    }

    return nullptr;
}


FUObjectArray* get_GUObjectArray();
std::pair<std::string, std::string> get_full_name(UObjectBase* obj);
UObjectBase* find_object(const char* type_name, const char* obj_full_name, bool cache=true);
std::string uprop_to_str(UProperty* uprop);
std::string uprop_type_to_str(UProperty* fprop);
std::string narrow(const FString& fstr);
std::string narrow(const FName& fname);

/*
inline uintptr_t GetVirtualFunctionAt(void* pObj, int index) {
    uintptr_t* pAddr = reinterpret_cast<uintptr_t*>(pObj);
    pAddr = (uintptr_t*)*pAddr;  // Gets a pointer to the virtual function table 
    return pAddr[index];
}
*/

struct UFuncParamInfo {
    UProperty* param;
    bool is_ptr_or_ref;
    std::string type;
    std::string name;
};

class UFunctionInvoker
{
public:
    UFunctionInvoker(UStruct* uobj, UFunction* ufunc);

    operator bool() const { return m_is_valid; }

    UStruct* m_this;
    UFunction* m_f;

    bool m_is_static;
    bool m_has_returntype;
    UFuncParamInfo m_return_type;
    std::vector<UFuncParamInfo> m_arguments;

    bool m_is_valid;
};