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
constexpr auto GUOBJECTARRAY_PAT = "48 8D 05 ? ? ? ? 48 89 71 10 45 84 C0 48 89 01"; // Tower Unite
// constexpr auto GUOBJECTARRAY_PAT = "48 8D 0D ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 33 FF 48 8B C8";  // UE4Game (test)
/// constexpr auto GUOBJECTARRAY_PAT = "48 8D 0D ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 48 8B D6"; // pavlov

constexpr auto FNAME_TOSTRING_PAT = "48 89 5C 24 08 57 48 83 EC ? 83 79 04 00";

constexpr auto GMALLOC_PAT = "41 8B D8 48 8B 0D ? ? ? ? 48 8B FA 48 85 C9"; // Tower Unite
// constexpr auto GMALLOC_PAT = "4C 8B D1 48 8B 0D ? ? ? ? 48 85 C9 75 ? 49 8B CA E9"; //  UE4Game (test)
// constexpr auto GMALLOC_PAT = "41 8B D8 48 8B 0D ? ? ? ? 48 8B FA 48 85 C9 75 ? E8"; //  pavlov


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


std::tuple<std::string, std::string, std::string> get_full_name(UObjectBase* obj) {
    if (obj == nullptr) {
        return {};
    }

    auto c = obj->GetClass();

    if (c == nullptr) {
        return {};
    }

    auto obj_name = narrow(obj->GetFName());
    std::string outer_name = "";

    for (auto outer = obj->GetOuter(); outer != nullptr; outer = outer->GetOuter()) {
        outer_name = narrow(outer->GetFName()) + "." + outer_name;
    }

    return {outer_name, narrow(c->GetFName()),obj_name };
}

UObjectBase* find_object(const char* type_name, const char* obj_full_name, bool cache) {
    static std::unordered_map<std::string, UObjectBase*> obj_map{};

    std::string concated_name = type_name;
    concated_name += " ";
    concated_name += obj_full_name;
    if (cache) {
        if (auto search = obj_map.find(concated_name); search != obj_map.end()) {
            return search->second;
        }
    }
    auto obj_array = get_GUObjectArray();

    for (auto i = 0; i < obj_array->GetObjectArrayNum(); ++i) {
        if (auto obj_item = obj_array->IndexToObject(i)) {
            if (auto obj_base = obj_item->Object) {
                auto [_outer, _type, _name] = get_full_name(obj_base);
                if (_type == type_name && (_outer + _name) == obj_full_name) {
                    obj_map[concated_name] = obj_base;
                    return obj_base;
                }
            }
        }
    }

    if (cache) {
        // retry without cache
        obj_map.clear();
        return find_object(type_name, obj_full_name, false);
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

UClass* UObject::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.Object");
}

UClass* UStruct::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.Struct");
}

UClass* UClass::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.Class");
}

UClass* UScriptStruct::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.ScriptStruct");
}

UClass* UEnum::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.Enum");
}

UClass* UFunction::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.Function");
}

UClass* UProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.Property");
}

UClass* UByteProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.ByteProperty");
}

UClass* UInt8Property::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.Int8Property");
}

UClass* UInt16Property::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.Int16Property");
}

UClass* UIntProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.IntProperty");
}

UClass* UInt64Property::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.Int64Property");
}

UClass* UUInt16Property::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.UInt16Property");
}

UClass* UUInt32Property::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.UInt32Property");
}

UClass* UUInt64Property::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.UInt64Property");
}

UClass* UFloatProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.FloatProperty");
}

UClass* UDoubleProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.DoubleProperty");
}

UClass* UBoolProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.BoolProperty");
}

UClass* UObjectProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.ObjectProperty");
}

UClass* UNameProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.NameProperty");
}

UClass* UStrProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.StrProperty");
}

UClass* UArrayProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.ArrayProperty");
}

UClass* UStructProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.StructProperty");
}

UClass* UEnumProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.EnumProperty");
}

UClass* UNumericProperty::GetPrivateStaticClass() {
    return (UClass*)find_object("Class", "/Script/CoreUObject.NumericProperty");
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

std::string uprop_to_str(UProperty* uprop) {
    DynProperty dynprop(uprop);
    std::string value = dynprop.ToString();
    if (value.size()) {
        value = " = " + value;
    }
    return  kanan::stringFormat("%s %s%s; // off:0x%x, size:0x%x, align:0x%x",
        uprop_type_to_str(uprop).c_str(),
        narrow(uprop->GetName()).c_str(),
        value.c_str(),
        uprop->GetOffset_ForUFunction(),
        uprop->GetSize(),
        uprop->GetMinAlignment()
    );
}

std::string uprop_type_to_str(UProperty* fprop) {
    if (fprop->IsA<UByteProperty>()) {
        auto fbyte = (UByteProperty*)fprop;

        // TODO: Handle FByteProperty's that are actual enums.
        if (fbyte->Enum != nullptr) {
            auto uenum = fbyte->Enum;
            auto enum_name = narrow(uenum->GetFName());
            enum_name = enum_name.substr(enum_name.find_last_of(':') + 1);

            if (uenum->GetCppForm() == UEnum::ECppForm::Namespaced) {
                auto ns_name = enum_name;
                enum_name = narrow(uenum->CppType);
                enum_name = enum_name.substr(enum_name.find_last_of(':') + 1);

                if (enum_name.empty()) {
                    enum_name = "Type";
                }

                return ns_name + "::" + enum_name;
            }
            else {
                return enum_name;
            }
        }
        else {
            return "uint8_t";
        }
    }
    else if (fprop->IsA<UInt8Property>()) {
        return "int8_t";
    }
    else if (fprop->IsA<UInt16Property>()) {
        return "int16_t";
    }
    else if (fprop->IsA<UIntProperty>()) {
        return "int32_t";
    }
    else if (fprop->IsA<UInt64Property>()) {
        return "int64_t";
    }
    else if (fprop->IsA<UUInt16Property>()) {
        return "uint16_t";
    }
    else if (fprop->IsA<UUInt32Property>()) {
        return "uint32_t";
    }
    else if (fprop->IsA<UUInt64Property>()) {
        return "uint64_t";
    }
    else if (fprop->IsA<UFloatProperty>()) {
        return "float";
    }
    else if (fprop->IsA<UDoubleProperty>()) {
        return "double";
    }
    else if (fprop->IsA<UBoolProperty>()) {
        auto fbool = (UBoolProperty*)fprop;

        // A FieldMask of 0xFF indicates a native bool, otherwise its part of a bitfield.
        // NOTE: Must make FieldMask public.
        if (fbool->FieldMask == 0xFF) {
            return "bool";
        }
        else {
            return "uint8_t";
        }
    }
    else if (fprop->IsA<UObjectProperty>()) {
        auto fobj = (UObjectProperty*)fprop;
        if (auto uclass = fobj->PropertyClass) {
            return kanan::narrow(uclass->GetPrefixCPP()) + narrow(uclass->GetFName()) + '*';
        }
    }
    else if (fprop->IsA<UStructProperty>()) {
        auto fstruct = (UStructProperty*)fprop;
        auto ustruct = fstruct->Struct;
        return kanan::narrow(ustruct->GetPrefixCPP()) + narrow(ustruct->GetFName());
    }
    else if (fprop->IsA<UArrayProperty>()) {
        auto farray = (UArrayProperty*)fprop;
        auto inner = farray->Inner;
        auto inner_typename = uprop_type_to_str(inner);

        if (inner_typename.empty()) {
            inner_typename = "void*";
        }

        return "TArray<" + inner_typename + '>';
    }
    else if (fprop->IsA<UNameProperty>()) {
        return "FName";
    }
    else if (fprop->IsA<UStrProperty>()) {
        return "FString";
    }
    else if (fprop->IsA<UEnumProperty>()) {
        auto fenum = (UEnumProperty*)fprop;
        auto uenum = fenum->GetEnum();
        auto enum_name = narrow(uenum->GetFName());
        enum_name = enum_name.substr(enum_name.find_last_of(':') + 1);

        if (uenum->GetCppForm() == UEnum::ECppForm::Namespaced) {
            auto ns_name = enum_name;
            enum_name = narrow(uenum->CppType);
            enum_name = enum_name.substr(enum_name.find_last_of(':') + 1);

            if (enum_name.empty()) {
                enum_name = "Type";
            }

            return ns_name + "::" + enum_name;
        }
        else {
            return enum_name;
        }
    }

    return narrow(fprop->GetClass()->GetName());
}

UFunctionInvoker::UFunctionInvoker(UFunction* ufunc) :
   m_f(ufunc), m_is_static(false), m_has_returntype(false),
    m_return_type({}), m_arguments({}), m_is_valid(false)
{
    for (auto* field = ufunc->Children; field != nullptr; field = field->Next) {
        if (!field->IsA<UProperty>()) {
            assert(false);
            continue;
        }
        auto prop = (UProperty*)field;
        auto param_flags = prop->PropertyFlags;
        auto param_type = uprop_type_to_str(prop);
        auto param_name = narrow(prop->GetName());
        if (param_type == "") {
            param_type = "?";
            return;
        }

        bool is_ptr_or_ref = false;
        if ((param_flags & CPF_ReferenceParm) || (param_flags & CPF_OutParm)) {
            param_type += "&";
            is_ptr_or_ref = true;
        }
        if (param_type.rfind("*") != param_type.npos || param_name.rfind("*") != param_name.npos) {
            is_ptr_or_ref = true;
        }
        UFuncParamInfo info{ prop, is_ptr_or_ref, param_type, param_name };
        if (param_flags & CPF_ReturnParm) {
            m_has_returntype = true;
            m_return_type = info;
        }
        else {
            m_arguments.push_back(info);
        }
    }
    m_is_valid = true;
}
