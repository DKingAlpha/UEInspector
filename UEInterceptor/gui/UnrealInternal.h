#pragma once
#include "UObject/UnrealType.h"
#include <string>


template <typename T, typename T2 = UObject*> T DCast(T2 In) {
    if (In && ((UObject*)In)->IsA<std::remove_pointer_t<T>>()) {
        return (T)In;
    }

    return nullptr;
}


FUObjectArray* get_GUObjectArray();
std::string get_full_name(UObjectBase* obj);
std::string narrow(const FString& fstr);
std::string narrow(const FName& fname);
