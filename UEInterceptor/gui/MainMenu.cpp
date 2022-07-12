#include "MainMenu.h"

#include "UnrealInternal.h"
#include "UObject/Class.h"
#include "UObject/EnumProperty.h"
#include "UObject/UObjectArray.h"
#include "UObject/UnrealType.h"

#include "imgui.h"
#include "../dllmain.h"

#include <tuple>
#include <vector>
#include <algorithm>

// ===========================================================

void DrawUEnum(UEnum* uenum)
{
}

void DrawUStruct(UStruct* ustruct)
{
    if (ustruct->IsA<UFunction>()) {
        // function
    }
    else if (ustruct->IsA<UClass>()) {
        // class
    }
    else {
        // struct
    }
}

class InfoUObjBase
{
public:
    InfoUObjBase(UObjectBase* p) : ptr(p), name(get_full_name(p)) {

    }

    UObjectBase* ptr;
    std::string name;
};

namespace SearchContext {
    static std::vector<InfoUObjBase> all_uobjs;
    static std::vector<InfoUObjBase> filtered_uobjs;
    int select_type = 0;
    bool case_sensitive = true;
    char search_keyword[1024]{};


    int selected_idx = -1;
    InfoUObjBase* Selected() {
        if (selected_idx < 0) {
            return nullptr;
        }
        return &filtered_uobjs[selected_idx];
    }
};

bool ReloadUObjects()
{
    SearchContext::all_uobjs.clear();
    for (auto i = 0; i < get_GUObjectArray()->GetObjectArrayNum(); ++i) {
        auto obj_item = get_GUObjectArray()->IndexToObject(i);

        if (obj_item == nullptr) {
            continue;
        }

        auto obj = obj_item->Object;
        if (DCast<UEnum*>(obj) || DCast<UStruct*>(obj)) {
            SearchContext::all_uobjs.push_back({obj});
        }
    }
    return true;
}

int UpdateSearchResult(ImGuiInputTextCallbackData* data = nullptr)
{
    SearchContext::filtered_uobjs.clear();
    std::string keyword = SearchContext::search_keyword;
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
        if (SearchContext::select_type == 1 && !DCast<UEnum*>(uobj.ptr)) {
            continue;
        }
        if (SearchContext::select_type == 2 && !DCast<UStruct*>(uobj.ptr)) {
            continue;
        }
        if (SearchContext::select_type == 3 && !DCast<UFunction*>(uobj.ptr)) {
            continue;
        }
        if (SearchContext::select_type == 4 && !DCast<UClass*>(uobj.ptr)) {
            continue;
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
    ImGui::SetNextItemWidth(col_width * 0.3);
    static bool first_shown = false;
    bool search_condition_changed = false;
    if (ImGui::Combo("##Type", &SearchContext::select_type,
        "ANY TYPES\0"
        "UEnum\0"
        "UStruct\0"
        "- UFunction\0"
        "- UClass\0")) {
        search_condition_changed = true;
    }

    ImGui::SameLine();

    if (ImGui::Checkbox("CaseSensitive", &SearchContext::case_sensitive)) {
        search_condition_changed = true;
    }

    ImGui::SameLine();

    ImGui::SetNextItemWidth(col_width * 0.7 - ImGui::CalcTextSize("[x] CaseSensitive  ").x);
    if (ImGui::InputTextWithHint("##search keyword", "Search keyword ...", SearchContext::search_keyword, sizeof(SearchContext::search_keyword),
        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackEdit, UpdateSearchResult, 0)) {
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
    if (ImGui::BeginListBox("##Filtered Objects", { col_width - 20, winsize.y - 120 })) {
        for (size_t i = 0; i < SearchContext::filtered_uobjs.size(); i++) {
            if (ImGui::Selectable(SearchContext::filtered_uobjs[i].name.c_str(), SearchContext::selected_idx == i))
                SearchContext::selected_idx = i;
            if (SearchContext::selected_idx == i) {
                ImGui::SetItemDefaultFocus();
            }
            if (ImGui::IsItemActive() || ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", SearchContext::filtered_uobjs[i].name.c_str());
        }
        ImGui::EndListBox();
    }
    ImGui::EndChild();
    ImGui::EndGroup();
}

void DrawUEnumInfo(UEnum* obj) {
    static bool show_hex = false;
    ImGui::Checkbox("HEX", &show_hex);

    std::string enum_cpp_form = "";
    switch (obj->GetCppForm()) {
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
    for(auto& [fname, value] : obj->Names) {
        ImGui::TextWrapped(show_hex ? "%s = 0x%llx" : "%s = %lld", narrow(fname).c_str(), value);
    }
    ImGui::EndChild();
}

void DrawUFunctionInfo(UFunction* obj) {

}

void DrawUClassInfo(UClass* obj) {

}

void DrawUStructInfo(UStruct* obj) {

}

void DrawRightPanel()
{
    auto* selected = SearchContext::Selected();
    if (!selected) {
        return;
    }

    ImGui::TableNextColumn();

    float col_width = ImGui::GetColumnWidth();

    ImGui::BeginGroup();
    ImGui::BeginChild("##View Detail", ImVec2(col_width, 0), true);

    ImGui::TextWrapped("Name: %s", selected->name.c_str());
    if (auto* uenum = DCast<UEnum*>(selected->ptr)) {
        DrawUEnumInfo(uenum);
    }
    else if (auto* ufunction = DCast<UFunction*>(selected->ptr)) {
        DrawUFunctionInfo(ufunction);
    }
    else if (auto* uclass = DCast<UClass*>(selected->ptr)) {
        DrawUClassInfo(uclass);
    }
    else if (auto* ustruct = DCast<UStruct*>(selected->ptr)) {
        DrawUStructInfo(ustruct);
    }
    else {
        assert(false);
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
            if (ImGui::BeginTable("##WorldViewerTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable), 16.0f) {
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
