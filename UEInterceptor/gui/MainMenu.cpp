#include "MainMenu.h"

#include "UnrealInternal.h"
#include "UObject/Class.h"
#include "UObject/EnumProperty.h"
#include "UObject/UObjectArray.h"
#include "UObject/UnrealType.h"

#include "imgui.h"
#include "ImGuiAddon.h"
#include "../dllmain.h"

#include "../mem/String.hpp"

#include <tuple>
#include <vector>
#include <algorithm>
#include <map>
#include <assert.h>

class InfoUObjBase
{
public:
    InfoUObjBase(UObjectBase* p) : ptr(p) {
        auto type_name = get_full_name(p);
        type = type_name.first;
        name = type_name.second;
    }

    UObjectBase* ptr;
    std::string type;
    std::string name;
};


namespace SearchContext {
    static std::map<std::string, bool> all_types_map;
    static std::vector<std::string> all_types;
    static std::vector<InfoUObjBase> all_uobjs;
    static std::vector<InfoUObjBase> filtered_uobjs;
    int selected_idx = -1;
    char select_type[1024]{ 0 };
    bool type_exact = false;
    bool case_sensitive = false;
    char search_keyword[1024]{};

    InfoUObjBase* Selected() {
        if (selected_idx < 0 || selected_idx >= filtered_uobjs.size()) {
            // maybe outdated index. skip this time
            return nullptr;
        }
        return &filtered_uobjs[selected_idx];
    }
};

bool ReloadUObjects()
{
    SearchContext::all_uobjs.clear();
    SearchContext::all_types_map.clear();
    SearchContext::all_types.clear();
    SearchContext::selected_idx = -1;
    for (auto i = 0; i < get_GUObjectArray()->GetObjectArrayNum(); ++i) {
        auto obj_item = get_GUObjectArray()->IndexToObject(i);

        if (obj_item == nullptr) {
            continue;
        }

        auto obj = obj_item->Object;
        if (obj) {
            InfoUObjBase iobj{ obj };
            SearchContext::all_uobjs.push_back(iobj);
            if (SearchContext::all_types_map.find(iobj.type) == SearchContext::all_types_map.end()) {
                SearchContext::all_types_map[iobj.type] = true;
                SearchContext::all_types.push_back(iobj.type);
            }
            
        }
    }
    std::sort(SearchContext::all_types.begin(), SearchContext::all_types.end());
    SearchContext::all_types.insert(SearchContext::all_types.begin(), "*");
    return true;
}

int UpdateSearchResult(ImGuiInputTextCallbackData* data = nullptr)
{
    SearchContext::filtered_uobjs.clear();
    std::string keyword = SearchContext::search_keyword;
    if (data) {
        // callback, SearchContext::search_keyword has not been updated, use data->Buf
        keyword = data->Buf;
    }
    if (!SearchContext::case_sensitive) {
        std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
    }
    for (size_t i = 0; i < SearchContext::all_uobjs.size(); i++) {
        InfoUObjBase& uobj = SearchContext::all_uobjs[i];
        if (keyword.size()) {
            std::string objname = uobj.name;
            if (!SearchContext::case_sensitive) {
                std::transform(objname.begin(), objname.end(), objname.begin(), ::tolower);
            }
            if (objname.find(keyword) == std::string::npos) {
                continue;
            }
        }

        if (SearchContext::select_type != nullptr && *SearchContext::select_type != 0 && *SearchContext::select_type != '*') {
            // not empty, use it as type filter
            if (SearchContext::type_exact) {
                if (uobj.type != SearchContext::select_type) {
                    continue;
                }
            }
            else {
                if (uobj.type.find(SearchContext::select_type) == uobj.type.npos) {
                    continue;
                }
            }
        }

        SearchContext::filtered_uobjs.push_back(uobj);
    }
    return 0;
}

void DrawLeftPanel()
{
    ImGui::TableNextColumn();

    float col_width = ImGui::GetColumnWidth();

    ImGui::BeginGroup();
    ImGui::SetNextItemWidth(col_width * 0.4);
    static bool first_shown = false;
    bool search_condition_changed = false;
    // level 2 (base 0) headers are too complicated to include. Checking by ClassName

    static ImGui::ComboFilterState filter_stat{ 0, false };

    if (ImGui::ComboFilter("##type filter", SearchContext::select_type, sizeof(SearchContext::select_type),
        SearchContext::all_types, filter_stat, ImGuiComboFlags_None)) {
        search_condition_changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Exact", &SearchContext::type_exact)) {
        search_condition_changed = true;
    }

    ImGui::SameLine();

    ImGui::SetNextItemWidth(col_width * 0.6 - ImGui::CalcTextSize("[ x ] Exact  [ x ] CaseSensitive ").x);
    if (ImGui::InputTextWithHint("##search keyword", "Search keyword ...", SearchContext::search_keyword, sizeof(SearchContext::search_keyword),
        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackEdit, UpdateSearchResult, 0)) {
        search_condition_changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("CaseSensitive", &SearchContext::case_sensitive)) {
        search_condition_changed = true;
    }


    if (!first_shown || search_condition_changed) {
        first_shown = true;
        ReloadUObjects();
        UpdateSearchResult(nullptr);
    }

    ImVec2 winsize = ImGui::GetWindowSize();

    ImGui::BeginChild("Filtered Objects", { col_width, winsize.y - 80 }, false);
    ImGui::LabelText("##Filtered Objects Count", "Found: %d", SearchContext::filtered_uobjs.size());
    // , { col_width - 20, winsize.y - 120 }
    if (ImGui::BeginTable("##Filtered Objects Table", 2, ImGuiTableFlags_BordersInner | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable)) {
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Full Name", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableHeadersRow();
        for (size_t i = 0; i < SearchContext::filtered_uobjs.size(); i++) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text(SearchContext::filtered_uobjs[i].type.c_str());
            ImGui::TableNextColumn();
            if (ImGui::Selectable(SearchContext::filtered_uobjs[i].name.c_str(), SearchContext::selected_idx == i))
                SearchContext::selected_idx = i;
            if (SearchContext::selected_idx == i) {
                ImGui::SetItemDefaultFocus();
            }
            if (ImGui::IsItemActive() || ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", SearchContext::filtered_uobjs[i].name.c_str());
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
    ImGui::EndGroup();
}

void DrawUEnumInfo(UEnum* uenum) {
    static bool show_hex = false;
    ImGui::Checkbox("HEX", &show_hex);

    std::string enum_cpp_form = "";
    switch (uenum->GetCppForm()) {
    case UEnum::ECppForm::EnumClass:
        enum_cpp_form = "enum class";
        break;
    case UEnum::ECppForm::Namespaced:
        enum_cpp_form = "namespaced";
        break;
    case UEnum::ECppForm::Regular:
        enum_cpp_form = "c";
        break;
    }
    ImGui::Text("Type: %s", enum_cpp_form.c_str());

    float col_width = ImGui::GetColumnWidth();
    ImGui::BeginChild("UEnum Definition", ImVec2(col_width - 20, 0), true);
    for(auto& [fname, value] : uenum->Names) {
        ImGui::TextWrapped(show_hex ? "%s = 0x%llx" : "%s = %lld", narrow(fname).c_str(), value);
    }
    ImGui::EndChild();
}


bool DrawUFunction(UStruct* obj, UFunction* ufunc)
{
    std::string return_type = "void";
    std::vector<std::pair<std::string, std::string>> args;
    for (auto* field = ufunc->Children; field != nullptr; field = field->Next) {
        if (!field->IsA<UProperty>()) {
            assert(false);
            continue;
        }
        auto prop = (UProperty*)field;
        if (!prop->GetSize()) {
            assert(false);
            continue;
        }
        auto param_flags = prop->PropertyFlags;
        auto param_name = narrow(prop->GetName());
        auto param_type = uprop_type_to_str(prop);
        if (param_type == "") {
            param_type = "?";
        }
        if (param_flags & CPF_ReturnParm) {
            return_type = param_type;
        }
        else {
            if (param_flags & CPF_ReferenceParm) {
                param_type += "&";
            } else if (param_flags & CPF_OutParm) {
                param_type += "*";
            }
            args.push_back({ param_type, param_name });
        }
    }
    std::string args_str;
    for (auto& [t, n] : args) {
        args_str += t;
        args_str += " ";
        args_str += n;
        args_str += ", ";
    }
    if (args_str.size() >= 2) {
        if (args_str.back() == ' ') {
            args_str.pop_back();
        }
        if (args_str.back() == ',') {
            args_str.pop_back();
        }
    }
    std::string signature = return_type.c_str();
    signature += " ";
    signature += narrow(ufunc->GetFName()).c_str();
    signature += "(";
    signature += args_str.c_str();
    signature += ")";

    if (ufunc->FunctionFlags & FUNC_Static) {
        signature = "static " + signature;
    }

    bool selected = false;
    return ImGui::Selectable(signature.c_str(), &selected);
}

void DrawCallUFunction(UStruct* uobj, UFunction* ufunc)
{
    if (ufunc->FunctionFlags & FUNC_Static) {
        uobj = nullptr;
    }
    ImGui::Text("%s->%s", uobj ? narrow(uobj->GetFName()).c_str() : "", narrow(ufunc->GetFName()).c_str());
    ImGui::NewLine();
    ImGui::Text("%p->%p", uobj ? uobj : 0, ufunc);
    ImGui::NewLine();
    ImGui::NewLine();

    UFunctionInvoker invoker{ uobj, ufunc };
    if (!invoker) {
        ImGui::Text("Failed to parse function");
    }
}

void DrawUPropertyInfoAsField(UProperty* uprop)
{
    if (!uprop->GetSize()) {
        return;
    }

    auto prop_name = narrow(uprop->GetName());
    ImGui::Text("%s", uprop_to_str(uprop).c_str());
    TMap<FString, FString> values;
    std::vector<std::string> input_datas;
    uprop->GetNativePropertyValues(values);
    for (auto& [s1, s2] : values) {
        ImGui::Text("%s = %s", narrow(s1).c_str(), narrow(s1).c_str());
    }
}


void DrawUStructInfo(UStruct* st) {
    ImGui::TextColored({0.2f, 0.2f, 0.8f, 1.f}, "size: 0x%x, align:0x%x", st->GetStructureSize(), st->GetMinAlignment());

    // properties
    for (auto* field = st->Children; field != nullptr; field = field->Next) {
        if (auto* uprop = DCast<UProperty*>(field)) {
            DrawUPropertyInfoAsField(uprop);
        }
    }
    static UStruct* calling_obj = nullptr;
    static UFunction* calling_function = nullptr;
    if (auto* ufunc = DCast<UFunction*>(st)) {
        // static functions
        if (DrawUFunction(st, ufunc)) {
            calling_obj = nullptr;
            calling_function = ufunc;
        }
    } else {
        // class methods
        for (auto* field = st->Children; field != nullptr; field = field->Next) {
            if (UFunction* ufunc = DCast<UFunction*>(field)) {
                if (DrawUFunction(st, ufunc)) {
                    calling_obj = st;
                    calling_function = ufunc;
                }
            }
        }
    }

    static bool s_function_caller_opening = true;
    if (calling_function) {
        ImGui::SetNextWindowSize({ 800, 600 });
        if (ImGui::Begin("Function Invoker", &s_function_caller_opening)) {
            DrawCallUFunction(calling_obj, calling_function);
            ImGui::End();
        }
        if (!s_function_caller_opening) {
            calling_obj = nullptr;
            calling_function = nullptr;
        }
    }
}


void DrawRightPanel()
{
    auto* selected = SearchContext::Selected();
    if (!selected) {
        return;
    }

    ImGui::TableNextColumn();

    float col_width = ImGui::GetColumnWidth();
    ImVec2 winsize = ImGui::GetWindowSize();

    ImGui::BeginGroup();
    ImGui::BeginChild("##View Detail", ImVec2(col_width, winsize.y - 60), false);

    ImGui::TextColored({0.f, 1.f, 0.f, 1.f}, "%s", selected->type.c_str());
    ImGui::TextWrapped("%s", selected->name.c_str());
    ImGui::NewLine();
    if (auto* uenum = DCast<UEnum*>(selected->ptr)) {
        DrawUEnumInfo(uenum);
    }
    else if (auto* uprop = DCast<UProperty*>(selected->ptr)) {
        DrawUPropertyInfoAsField(uprop);
    }
    else if (auto* ustruct = DCast<UStruct*>(selected->ptr)) {
        DrawUStructInfo(ustruct);
    }
    else if (kanan::strEndsWith(selected->type, "Actor")
        || kanan::strEndsWith(selected->type, "Character")
        || kanan::strEndsWith(selected->type, "Pawn")
    ) {
         // AActor and child classes
         auto* obj = DCast<UObject*>(selected->ptr);
         // uintptr_t process_event = GetVirtualFunctionAt(obj, UOBJECT_PROCESSEVENT_INDEX);
         //obj->ProcessEvent(nullptr, nullptr);
    }
    else {
        // assert(false);
    }

    ImGui::EndChild();
    ImGui::EndGroup();
}


bool DrawMainMenu()
{
    bool window_opening = true;

    // demo
    /*
    ImGui::ShowDemoWindow(&window_opening);
    if (!window_opening) {
        SelfUnload();
    }
    return window_opening;
    */

    // real
    ImGui::SetNextWindowSize({ 1280, 720 }, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("World Viewer", &window_opening, 0))
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
        if (ImGui::TreeNode("Object Viewer")) {
            if (ImGui::BeginTable("##WorldViewerTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) {
                ImGui::TableNextRow();
                DrawLeftPanel();    // column left
                DrawRightPanel();   // column right
                ImGui::EndTable();
            }
            ImGui::TreePop();
        }
        ImGui::End();
    }

    if (!window_opening) {
        SelfUnload();
    }

	return window_opening;
}
