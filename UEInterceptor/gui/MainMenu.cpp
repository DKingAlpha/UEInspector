#include "MainMenu.h"

#include "UnrealInternal.h"
#include "UObject/Class.h"
// #include "UObject/EnumProperty.h"
#include "UObject/UObjectArray.h"
#include "UObject/UnrealType.h"

#include "imgui.h"
#include "ImGuiAddon.h"
#include "../dllmain.h"

#include "../mem/String.hpp"
#include "../mem/Module.hpp"

#include "MinHook.h"
#include "NoUEUtil.h"

#include <tuple>
#include <vector>
#include <algorithm>
#include <map>
#include <assert.h>

class InfoUObjBase
{
public:
    InfoUObjBase(UObjectBase* uobj) : ptr(uobj) {
        auto [_outer, _type, _name] = get_full_name(uobj);
        outer = _outer;
        type = _type;
        name = _name;

        if (UStruct* p = DCast<UStruct*>(ptr)) {
            do {
                full_types.insert(full_types.begin(), narrow(p->GetFName()));
                p = p->GetSuperStruct();
            } while (p != nullptr);
        }
    }

    UObjectBase* ptr;
    std::vector<std::string> full_types;
    std::string outer;
    std::string type;
    std::string name;
};


namespace SearchContext {
    std::map<std::string, bool> all_types_map;
    std::vector<std::string> all_types;
    std::vector<InfoUObjBase> all_uobjs;
    std::vector<InfoUObjBase> filtered_uobjs;
    UFunction* calling_function;
    int current_page = 0;
    int selected_idx = -1;
    UObjectBase* selected_caller = nullptr;   // context of ProcessEvent
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

bool ReloadUObjects(bool keep_index = false)
{
    SearchContext::all_uobjs.clear();
    SearchContext::all_types_map.clear();
    SearchContext::all_types.clear();
    SearchContext::calling_function = nullptr;
    if (!keep_index) {
        SearchContext::selected_idx = -1;
    }
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
    SearchContext::current_page = 0;
    std::string keyword = SearchContext::search_keyword;
    if (data) {
        // callback, SearchContext::search_keyword has not been updated, use data->Buf
        keyword = data->Buf;
    }
    if (!SearchContext::case_sensitive) {
        std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
    }
    std::string lower_select_type = SearchContext::select_type;
    std::transform(lower_select_type.begin(), lower_select_type.end(), lower_select_type.begin(), ::tolower);
    for (size_t i = 0; i < SearchContext::all_uobjs.size(); i++) {
        InfoUObjBase& uobj = SearchContext::all_uobjs[i];
        if (keyword.size()) {
            std::string objname = uobj.outer + uobj.name;
            if (!SearchContext::case_sensitive) {
                std::transform(objname.begin(), objname.end(), objname.begin(), ::tolower);
            }
            if (objname.find(keyword) == std::string::npos) {
                continue;
            }
        }

        if (lower_select_type.size() > 0 && lower_select_type.front() != '*') {
            // not empty, use it as type filter
            std::string lower_item_type = uobj.type;
            std::transform(lower_item_type.begin(), lower_item_type.end(), lower_item_type.begin(), ::tolower);
            if (SearchContext::type_exact) {
                if (lower_item_type != lower_select_type) {
                    continue;
                }
            }
            else {
                if (lower_item_type.find(lower_select_type) == lower_item_type.npos) {
                    continue;
                }
            }
        }

        SearchContext::filtered_uobjs.push_back(uobj);
    }
    return 0;
}

std::vector<InfoUObjBase*> FindExistingUObjectInstances(UClass* ucls)
{
    std::vector<InfoUObjBase*> result;
    ReloadUObjects(true);
    for (auto& info : SearchContext::all_uobjs) {
        auto* obj_base = info.ptr;
        if (!obj_base) {
            continue;
        }
        UClass* cls = obj_base->GetClass();

        if (cls == ucls /* || cls->IsChildOf(ucls) */) {
            result.push_back(&info);
        }
    }
    return result;
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
    ImGui::SameLine();
    const int page_limit = 10000;
    if (ImGui::Button("<")) {
        // prev page
        SearchContext::current_page--;
    }
    ImGui::SameLine();
    const int show_range_start = SearchContext::current_page * page_limit;
    int show_range_end = (SearchContext::current_page + 1) * page_limit;
    if (show_range_end > SearchContext::filtered_uobjs.size()) {
        show_range_end = SearchContext::filtered_uobjs.size();
    }

    ImGui::Text("%d-%d", show_range_start + 1, show_range_end);
    ImGui::SameLine();
    if (ImGui::Button(">")) {
        // next page
        SearchContext::current_page++;
    }
    int max_page = ceilf(SearchContext::filtered_uobjs.size() / page_limit);
    if (SearchContext::current_page >= max_page ) {
        SearchContext::current_page = max_page - 1;
    }
    if (SearchContext::current_page < 0) {
        SearchContext::current_page = 0;
    }
    // , { col_width - 20, winsize.y - 120 }
    if (ImGui::BeginTable("##Filtered Objects Table", 2, ImGuiTableFlags_BordersInner | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable)) {
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Full Name", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableHeadersRow();
        for (size_t i = show_range_start; i < show_range_end; i++) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text(SearchContext::filtered_uobjs[i].type.c_str());
            ImGui::TableNextColumn();
            auto item_name = SearchContext::filtered_uobjs[i].outer + SearchContext::filtered_uobjs[i].name;
            if (ImGui::Selectable(item_name.c_str(), SearchContext::selected_idx == i)) {
                SearchContext::selected_idx = i;
            }
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::Button("Select as `this` for calling UFunction*")) {
                    SearchContext::selected_caller = SearchContext::filtered_uobjs[i].ptr;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Right-click to select as caller of UFunction*");

            if (SearchContext::selected_idx == i) {
                ImGui::SetItemDefaultFocus();
            }
            if (ImGui::IsItemActive() || ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", item_name.c_str());
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


std::string UFunction2Str(UFunction* ufunc)
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

    return signature;
}


bool DrawUFunction(UFunction* ufunc)
{
    bool selected = false;
    static std::map<UFunction*, std::string> ufunc_str_map;
    if (auto it = ufunc_str_map.find(ufunc); it != ufunc_str_map.end()) {
        return ImGui::Selectable(it->second.c_str(), &selected);
    }
    ufunc_str_map[ufunc] = UFunction2Str(ufunc);
    return ImGui::Selectable(ufunc_str_map[ufunc].c_str(), &selected);
}

struct CallFunctionGuiData {
    // new
    std::string s;
    DynProperty dp;
    // select
    bool is_selected;
    UObjectBase* ptr;
    ImGui::ComboFilterState filter_stat;
    char selected_instance_name[1024];
};


bool SelectExistingUObjectInstance(UProperty* uprop, CallFunctionGuiData& data)
{
    std::vector<InfoUObjBase*> result = FindExistingUObjectInstances(uprop->GetClass());


    std::vector<std::string> names;
    for (auto* obj : result) {
        auto [outer, type, name] = get_full_name(obj->ptr);
        names.push_back(outer + type + " " + name);
    }
    UObjectBase* prev_selected_instance = data.ptr;

    if (ImGui::ComboFilter("##instance selector", data.selected_instance_name, sizeof(data.selected_instance_name),
        names, data.filter_stat, ImGuiComboFlags_None)) {
        auto it = std::find(names.begin(), names.end(), data.selected_instance_name);
        if (it != names.end()) {
            auto offset = it - names.begin();
            data.ptr = result[offset]->ptr;
        }
    }
    ImGui::Text("Selected: %s", data.selected_instance_name);
    return data.ptr == prev_selected_instance;
}

void DrawCallUFunction(UFunction* ufunc)
{
    static uintptr_t gameBaseAddr = reinterpret_cast<uintptr_t>(kanan::getModuleHandle(NULL));

    if (SearchContext::selected_caller) {
        ImGui::TextColored({ 0, 1, 0, 1 }, "Caller: %p: %s",
            SearchContext::selected_caller,
            narrow(SearchContext::selected_caller->GetFName()).c_str());
    }

    ImGui::Text("NATIVE: %p: %s", reinterpret_cast<uintptr_t>(ufunc->GetNativeFunc()) - gameBaseAddr, UFunction2Str(ufunc).c_str());
    ImGui::NewLine();

    UFunctionInvoker invoker{ ufunc };
    if (!invoker) {
        ImGui::Text("Failed to parse function");
    }
    
    // make static buffer
    static UFunction* last_function = nullptr;
    static std::vector<CallFunctionGuiData> buffers;
    if (last_function != ufunc) {
        last_function = ufunc;
        buffers.clear();
        for (auto& arg : invoker.m_arguments) {
            CallFunctionGuiData data;
            data.s.resize(512);
            data.ptr = nullptr;
            data.is_selected = false;
            memset(data.selected_instance_name, 0, sizeof(data.selected_instance_name));
            buffers.push_back(data);
        }
    }

    int arg_index = 0;
    for (auto& arg : invoker.m_arguments) {
        CallFunctionGuiData& buf = buffers[arg_index++];
        int new_or_select = buf.is_selected ? 1 : 0;
        bool can_select = arg.is_ptr_or_ref && !DCast<UNumericProperty*>(arg.param);

        if (can_select) {
            ImGui::RadioButton("New", &new_or_select, 0); ImGui::SameLine();
            ImGui::RadioButton("Select", &new_or_select, 1); ImGui::SameLine();

        }
        // FIXME: broken select
        if (false && can_select && new_or_select) {
            buf.is_selected = true;
            if (SelectExistingUObjectInstance(arg.param, buf)) {

            }
            ImGui::Text("Address: %p", buf.ptr);
        }
        else {
            buf.is_selected = false;
            if (!buf.dp.m_prop) {
                buf.dp.Load(arg.param);
            }
            ImGui::SetNextItemWidth(256);
            ImGui::InputText(uprop_to_str(arg.param).c_str(), buf.s.data(), buf.s.size(), ImGuiInputTextFlags_EnterReturnsTrue);
        }
    }

    ImGui::NewLine();

    if (!SearchContext::selected_caller && !(ufunc->FunctionFlags & FUNC_Static)) {
        ImGui::TextColored({ 1, 0, 0, 1 }, "%s", "Select a caller in left panel first!");
    }
    else {
        if (ImGui::Button("Execute")) {
            std::vector<unsigned char> frame;
            for (auto& data : buffers) {
                if (!data.is_selected) {
                    if (data.dp.Parse(data.s)) {
                        // FIXME: alignment
                        frame.insert(frame.end(), data.dp.data.begin(), data.dp.data.end());
                    }
                    else {
                        static std::string errinfo = "Invalid value: ";
                        errinfo += data.s;
                        ImGui::TextColored({ 1, 0, 0, 1 }, "%s", errinfo.c_str());
                    }
                }
                else {
                    // TODO: select as pointer
                }
            }
            if (auto* obj = DCast<UObject*>(SearchContext::selected_caller)) {
                obj->ProcessEvent(ufunc, frame.data());
            }
        }
    }
}

void DrawUPropertyInfoAsField(UObjectBase* uobj, UProperty* uprop)
{
    if (!uprop->GetSize()) {
        return;
    }

    ImGui::Selectable(uprop_to_str(uprop).c_str());
    if (!uobj) {
        return;
    }
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::Button("Select as `this`")) {
            SearchContext::selected_caller = *reinterpret_cast<UObjectBase**>(
                reinterpret_cast<uintptr_t>(uobj) + uprop->GetOffset_ForUFunction());
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

char event_filter_obj[512] = {};
char event_filter_obj_not[512] = {};
char event_filter_func[512] = {};
char event_filter_func_not[512] = {};
bool PreProcessEventHook(UObject* obj, UFunction* Function, void* Parms)
{
    if (event_filter_obj_not[0] != 0) {
        std::string objname = narrow(obj->GetFName());
        if (objname.find(event_filter_obj_not) != std::string::npos) {
            return false;
        }
    }

    if (event_filter_obj[0] != 0) {
        std::string objname = narrow(obj->GetFName());
        if (objname.find(event_filter_obj) == std::string::npos) {
            return false;
        }
    }

    if (event_filter_func_not[0] != 0) {
        std::string funcname = narrow(Function->GetFName());
        if (funcname.find(event_filter_func_not) != std::string::npos) {
            return false;
        }
    }

    if (event_filter_func[0] != 0) {
        std::string funcname = narrow(Function->GetFName());
        if (funcname.find(event_filter_func) == std::string::npos) {
            return false;
        }
    }

    return true;

}
void PostProcessEventHook(UObject* obj, UFunction* Function, void* Parms)
{
    LogToConsole(narrow(obj->GetFName()) + " calling " + narrow(Function->GetFName()) + "\n");
}

typedef void(__fastcall* FnOrigProcessEvent)(UObject* obj, UFunction* Function, void* Parms);

std::map<void*, FnOrigProcessEvent> OrigProcessEvents;

void __fastcall MyProcessEvent(UObject* obj, UFunction* Function, void* Params)
{
    auto native_func = (void*)GetVirtualFunctionAt(obj, UOBJECT_PROCESSEVENT_INDEX);
    auto it_OrigProcessEvent = OrigProcessEvents.find(native_func);
    if(it_OrigProcessEvent != OrigProcessEvents.end()) {
        bool run_post = PreProcessEventHook(obj, Function, Params);
        it_OrigProcessEvent->second(obj, Function, Params);
        if (run_post) {
            PostProcessEventHook(obj, Function, Params);
        }
    }
    else {
        LogToConsole(narrow(obj->GetFName()) + " MISSING " + narrow(Function->GetFName()) + "\n");
    }
}

void DrawUStructInfo(UObjectBase* uobj, UStruct* st) {
    ImGui::TextColored({0.2f, 0.2f, 0.8f, 1.f}, "struct size:0x%x, unaligned:0x%x, align:0x%x", st->GetStructureSize(), st->PropertiesSize, st->GetMinAlignment());

    if (UFunction* ufunc = DCast<UFunction*>(st)) {
        ImGui::TextColored({0.2f, 0.8f, 0.8f, 1.f}, "params size:0x%x", ufunc->ParmsSize);
    }


    // process event
    ImGui::InputText("EventFilter Object", event_filter_obj, sizeof(event_filter_obj));
    ImGui::InputText("EventFilter Object Not", event_filter_obj_not, sizeof(event_filter_obj_not));

    ImGui::InputText("EventFilter Function", event_filter_func, sizeof(event_filter_func));
    ImGui::InputText("EventFilter Function Not", event_filter_func_not, sizeof(event_filter_func_not));

    // at least a UObject which has virtual ProcessEvent
    auto caller = DCast<UObject*>(SearchContext::selected_caller);
    if (caller) {
        auto native_func = (void*)GetVirtualFunctionAt(caller, UOBJECT_PROCESSEVENT_INDEX);
        if (OrigProcessEvents.find(native_func) == OrigProcessEvents.end()) {
            if (ImGui::Button("Hook ProcessEvent")) {
                FnOrigProcessEvent orig_fn = nullptr;
                if (MH_CreateHook(native_func, MyProcessEvent, (void**)&orig_fn) == MH_OK) {
                    OrigProcessEvents[native_func] = orig_fn;
                    MH_EnableHook(native_func);
                }
            }
        }
        else {
            if (ImGui::Button("Unhook ProcessEvent")) {
                MH_DisableHook(native_func);
                MH_RemoveHook(native_func);
                OrigProcessEvents.erase(native_func);
            }
        }
        ImGui::Text("NATIVE: %p", native_func);
    }

    // properties
    for (UStruct* pst = st; pst != nullptr; pst = pst->GetSuperStruct()) {
        ImGui::TextWrapped("======== %s ========", narrow(pst->GetFName()).c_str());
        for (auto* field = pst->Children; field != nullptr; field = field->Next) {
            if (auto* uprop = DCast<UProperty*>(field)) {
                DrawUPropertyInfoAsField(uobj, uprop);
            }
        }
    }

    ImGui::NewLine();

    if (auto* ufunc = DCast<UFunction*>(st)) {
        // static functions
        if (DrawUFunction(ufunc)) {
            SearchContext::calling_function = ufunc;
        }
    } else {
        // class methods
        for (UStruct* pst = st; pst != nullptr; pst = pst->GetSuperStruct()) {
            ImGui::TextWrapped("======== %s ========", narrow(pst->GetFName()).c_str());
            for (auto* field = pst->Children; field != nullptr; field = field->Next) {
                if (UFunction* ufunc = DCast<UFunction*>(field)) {                                                                                                     
                    if (DrawUFunction(ufunc)) {
                        SearchContext::calling_function = ufunc;
                    }
                }
            }
        }
    }

    bool s_function_caller_opening = true;
    if (SearchContext::calling_function) {
        ImGui::SetNextWindowSize({ 600, 300 }, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Function Invoker", &s_function_caller_opening)) {
            DrawCallUFunction(SearchContext::calling_function);
            ImGui::End();
        }
        if (!s_function_caller_opening) {
            SearchContext::calling_function = nullptr;
        }
        

    }
}

UStruct* TryFindUStruct(InfoUObjBase* instance)
{
    static std::map<UObjectBase*, UStruct*> s_cache;
    if (s_cache.find(instance->ptr) != s_cache.end()) {
        return s_cache.find(instance->ptr)->second;
    }
    UStruct* p = nullptr;
    auto type_name = instance->outer + instance->type;
    for (auto& info : SearchContext::all_uobjs) {
        if (info.outer + info.name == type_name) {
            p = DCast<UStruct*>(info.ptr);
            break;
        }
    }
    s_cache[instance->ptr] = p;
    return p;
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
    for (auto& pt : selected->full_types) {
        if (ImGui::Button(pt.c_str())) {
            strncpy_s(SearchContext::select_type, pt.c_str(), pt.size());
        }
        ImGui::SameLine();
        ImGui::Text(">");
        ImGui::SameLine();
    }
    ImGui::NewLine();

    if (SearchContext::selected_caller) {
        ImGui::Text("this=%p, %s", SearchContext::selected_caller, narrow(SearchContext::selected_caller->GetFName()).c_str());
    }

    ImGui::TextWrapped("%s%s", selected->outer.c_str(), selected->name.c_str());
    ImGui::NewLine();

    // QUICK HACK:
    UStruct* pst = TryFindUStruct(selected);
    if (pst && selected->name == "Default__Halloween2020JackoLanternLobby_C") {
        if (ImGui::Button("Hack Lanterns")) {
            UFunction* calling_function = nullptr;
            for (auto* field = pst->Children; field != nullptr; field = field->Next) {
                if (UFunction* ufunc = DCast<UFunction*>(field)) {
                    if (narrow(ufunc->GetFName()).find("UpdateQuestStatus") != -1) {
                        calling_function = ufunc;
                        break;
                    }
                }
            }
            if (calling_function) {
                std::vector<uint8_t> frame;
                frame.resize(64);
                for (auto info : SearchContext::filtered_uobjs) {
                    if (info.outer.find("/Maps/") != info.name.npos && info.name.find("JackOLanternStatic") != info.name.npos) {
                        if (auto* obj = DCast<UObject*>(info.ptr)) {
                            obj->ProcessEvent(calling_function, frame.data());
                        }
                    }
                }
            }
        }
    }

    /*
    static bool refresh_instances = false;
    static std::map<InfoUObjBase*, std::vector<InfoUObjBase*>> instances_uobjs;

    if (instances_uobjs.find(selected) == instances_uobjs.end()) {
        instances_uobjs[selected] = {};
    }
    if (refresh_instances) {
        std::vector<InfoUObjBase*> instances_info = FindExistingUObjectInstances(selected->ptr->GetClass());
        instances_uobjs[selected].insert(instances_uobjs[selected].end(), instances_info.begin(), instances_info.end());
        refresh_instances = false;
    }
    if (ImGui::Button("Instances")) {
        refresh_instances = true;
    }
    if (ImGui::BeginListBox("## Instances")) {
        for (auto instance : instances_uobjs[selected]) {
            if (ImGui::Selectable(instance->name.c_str())) {
                SearchContext::selected_caller = instance->ptr;
            }
        }
        ImGui::EndListBox();
    }
    */

    if (auto* uenum = DCast<UEnum*>(selected->ptr)) {
        DrawUEnumInfo(uenum);
    }
    else if (auto* uprop = DCast<UProperty*>(selected->ptr)) {
        DrawUPropertyInfoAsField(nullptr, uprop);
    }
    else if (auto* ustruct = DCast<UStruct*>(selected->ptr)) {
        DrawUStructInfo(nullptr, ustruct);
    }
    /*
    else if (kanan::strEndsWith(selected->type, "Actor")
        || kanan::strEndsWith(selected->type, "Character")
        || kanan::strEndsWith(selected->type, "Pawn")
    ) {
         // AActor and child classes
         auto* obj = DCast<UObject*>(selected->ptr);
         // uintptr_t process_event = GetVirtualFunctionAt(obj, UOBJECT_PROCESSEVENT_INDEX);
         //obj->ProcessEvent(nullptr, nullptr);
    }*/
    else {
        // assert(false);
        UStruct* cls_type = TryFindUStruct(selected);
        if (cls_type) {
            // SearchContext::selected_caller = selected->ptr;
            DrawUStructInfo(selected->ptr, cls_type);
        }
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
    }
    ImGui::End();

    if (!window_opening) {
        SelfUnload();
    }

	return window_opening;
}
