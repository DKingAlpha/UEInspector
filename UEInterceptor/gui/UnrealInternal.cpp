#include "UnrealInternal.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/EnumProperty.h"

#include "../mem/Scan.hpp"
#include "../mem/String.hpp"
#include "../mem/Utility.hpp"

#include <unordered_map>
#include <assert.h>


// shipping
constexpr auto GUOBJECTARRAY_PAT = "48 8D 0D ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 33 FF 48 8B C8";  // UE4Game (test)
/// constexpr auto GUOBJECTARRAY_PAT = "48 8D 0D ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 48 8B D6"; // pavlov
constexpr auto FNAME_TOSTRING_PAT = "48 89 5C 24 08 57 48 83 EC ? 83 79 04 00";

constexpr auto GMALLOC_PAT = "4C 8B D1 48 8B 0D ? ? ? ? 48 85 C9 75 ? 49 8B CA E9"; //  UE4Game (test)
// constexpr auto GMALLOC_PAT = "41 8B D8 48 8B 0D ? ? ? ? 48 8B FA 48 85 C9 75 ? E8"; //  pavlov

constexpr int UOBJECT_PROCESSEVENT_INDEX = 64;

std::string narrow(const FString& fstr) {
    auto& char_array = fstr.GetCharArray();
    return kanan::narrow(char_array.Num() ? char_array.GetData() : L"");
}

std::string narrow(const FName& fname) {
    return narrow(fname.ToString());
}

FMalloc** p_GMalloc = nullptr;

FMalloc* get_GMalloc()
{
    if (!p_GMalloc) {
        auto p = kanan::scan(GMALLOC_PAT);
        if (!p) {
            assert(false);
            return nullptr;
        }
        p_GMalloc = (FMalloc**)kanan::rel_to_abs(*p + 6);
    }
    return *p_GMalloc;
}

FUObjectArray* get_GUObjectArray() {
    static FUObjectArray* obj_array{};

    if (obj_array == nullptr) {
        auto lea = kanan::scan(GUOBJECTARRAY_PAT);

        if (!lea) {
            assert(false);
            return nullptr;
        }

        obj_array = (FUObjectArray*)kanan::rel_to_abs(*lea + 3);
    }
    assert(obj_array);
    return obj_array;
}


std::string get_full_name(UObjectBase* obj) {
    auto c = obj->GetClass();

    if (c == nullptr) {
        return "";
    }

    auto obj_name = narrow(obj->GetFName());

    for (auto outer = obj->GetOuter(); outer != nullptr; outer = outer->GetOuter()) {
        obj_name = narrow(outer->GetFName()) + '.' + obj_name;
    }

    return narrow(c->GetFName()) + ' ' + obj_name;
}

UObjectBase* find_object(const char* obj_full_name) {
    static std::unordered_map<std::string, UObjectBase*> obj_map{};

    if (auto search = obj_map.find(obj_full_name); search != obj_map.end()) {
        return search->second;
    }

    auto obj_array = get_GUObjectArray();

    for (auto i = 0; i < obj_array->GetObjectArrayNum(); ++i) {
        if (auto obj_item = obj_array->IndexToObject(i)) {
            if (auto obj_base = obj_item->Object) {
                if (get_full_name(obj_base) == obj_full_name) {
                    obj_map[obj_full_name] = obj_base;
                    return obj_base;
                }
            }
        }
    }

    return nullptr;
}


void FMemory::Free(void* mem) {
    return get_GMalloc()->Free(mem);
}

void* FMemory::Realloc(void* mem, size_t count, uint32_t alignment) {
    return get_GMalloc()->Realloc(mem, count, alignment);
}

size_t FMemory::QuantizeSize(size_t size, uint32_t alignment) {
    return get_GMalloc()->QuantizeSize(size, alignment);
}

UClass* UStruct::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.Struct");
}

UClass* UClass::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.Class");
}

UClass* UScriptStruct::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.ScriptStruct");
}

UClass* UEnum::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.Enum");
}

UClass* UFunction::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.Function");
}

UClass* UProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.Property");
}

UClass* UByteProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.ByteProperty");
}

UClass* UInt8Property::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.Int8Property");
}

UClass* UInt16Property::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.Int16Property");
}

UClass* UIntProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.IntProperty");
}

UClass* UInt64Property::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.Int64Property");
}

UClass* UUInt16Property::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.UInt16Property");
}

UClass* UUInt32Property::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.UInt32Property");
}

UClass* UUInt64Property::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.UInt64Property");
}

UClass* UFloatProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.FloatProperty");
}

UClass* UDoubleProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.DoubleProperty");
}

UClass* UBoolProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.BoolProperty");
}

UClass* UObjectProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.ObjectProperty");
}

UClass* UNameProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.NameProperty");
}

UClass* UStrProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.StrProperty");
}

UClass* UArrayProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.ArrayProperty");
}

UClass* UStructProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.StructProperty");
}

UClass* UEnumProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class /Script/CoreUObject.EnumProperty");
}

FString FName::ToString() const {
    FString out;

    static void (*toString)(const FName*, FString&) {};

    if (toString == nullptr) {

        toString = (decltype(toString))kanan::scan(FNAME_TOSTRING_PAT).value_or(0);
    }

    assert(toString);
    toString(this, out);
    return out;
}