#include "ImGuiAddon.h"
#include "imgui_internal.h"

bool ImGui::ComboFilter(const char* label, char* buffer, int bufferlen, std::vector<std::string>& hints, ComboFilterState& s, ImGuiComboFlags flags) {

    using namespace ImGui;

    s.selectionChanged = false;

    // Always consume the SetNextWindowSizeConstraint() call in our early return paths
    ImGuiContext& g = *GImGui;

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    bool popup_open = IsPopupOpen(id, ImGuiPopupFlags_None);
    bool popupNeedBeOpen = false;  // hints.size() == 0 ? false : (hints[s.activeIdx] != buffer);
    bool popupJustOpened = false;

    IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

    const ImGuiStyle& style = g.Style;

    const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const float expected_w = CalcItemWidth();
    const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : expected_w;
    const ImRect frame_bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + w, window->DC.CursorPos.y + label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(frame_bb.Min, ImVec2((label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f) + frame_bb.Max.x, frame_bb.Max.y));
    const float value_x2 = ImMax(frame_bb.Min.x, frame_bb.Max.x - arrow_size);
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb))
        return false;


    bool hovered, held;
    bool pressed = ButtonBehavior(frame_bb, id, &hovered, &held);

    if (!popup_open) {
        const ImU32 frame_col = GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
        RenderNavHighlight(frame_bb, id);
        if (!(flags & ImGuiComboFlags_NoPreview))
            window->DrawList->AddRectFilled(frame_bb.Min, ImVec2(value_x2, frame_bb.Max.y), frame_col, style.FrameRounding, (flags & ImGuiComboFlags_NoArrowButton) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Left);
    }
    if (!(flags & ImGuiComboFlags_NoArrowButton))
    {
        ImU32 bg_col = GetColorU32((popup_open || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
        ImU32 text_col = GetColorU32(ImGuiCol_Text);
        window->DrawList->AddRectFilled(ImVec2(value_x2, frame_bb.Min.y), frame_bb.Max, bg_col, style.FrameRounding, (w <= arrow_size) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Right);
        if (value_x2 + arrow_size - style.FramePadding.x <= frame_bb.Max.x)
            RenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.y, frame_bb.Min.y + style.FramePadding.y), text_col, ImGuiDir_Down, 1.0f);
    }
    if (!popup_open) {

        RenderFrameBorder(frame_bb.Min, frame_bb.Max, style.FrameRounding);
        if (buffer != NULL && !(flags & ImGuiComboFlags_NoPreview))

            RenderTextClipped(ImVec2(frame_bb.Min.x + style.FramePadding.x, frame_bb.Min.y + style.FramePadding.y), ImVec2(value_x2, frame_bb.Max.y), buffer, NULL, NULL, ImVec2(0.0f, 0.0f));

        if ((pressed || g.NavActivateId == id || popupNeedBeOpen) && !popup_open)
        {
            if (window->DC.NavLayerCurrent == 0)
                window->NavLastIds[0] = id;
            OpenPopupEx(id);
            popup_open = true;
            popupJustOpened = true;
        }
    }

    if (label_size.x > 0)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    if (!popup_open) {
        return false;
    }

    const float totalWMinusArrow = w - arrow_size;
    struct ImGuiSizeCallbackWrapper {
        static void sizeCallback(ImGuiSizeCallbackData* data)
        {
            float* totalWMinusArrow = (float*)(data->UserData);
            data->DesiredSize = ImVec2(*totalWMinusArrow, 200.f);
        }
    };
    SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(totalWMinusArrow, 150.f), ImGuiSizeCallbackWrapper::sizeCallback, (void*)&totalWMinusArrow);

    char name[16];
    ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

    // Peak into expected window size so we can position it
    if (ImGuiWindow* popup_window = FindWindowByName(name))
        if (popup_window->WasActive)
        {
            ImVec2 size_expected = CalcWindowNextAutoFitSize(popup_window);
            if (flags & ImGuiComboFlags_PopupAlignLeft)
                popup_window->AutoPosLastDirection = ImGuiDir_Left;
            ImRect r_outer = GetPopupAllowedExtentRect(popup_window);
            ImVec2 pos = FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, frame_bb, ImGuiPopupPositionPolicy_ComboBox);

            pos.y -= label_size.y + style.FramePadding.y * 2.0f;

            SetNextWindowPos(pos);
        }

    // Horizontally align ourselves with the framed text
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
    //    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.FramePadding.x, style.WindowPadding.y));
    bool ret = Begin(name, NULL, window_flags);

    ImGui::PushItemWidth(ImGui::GetWindowWidth());
    ImGui::SetCursorPos(ImVec2(0.f, window->DC.CurrLineTextBaseOffset));
    if (popupJustOpened) {
        ImGui::SetKeyboardFocusHere(0);
    }
    bool done = InputTextEx("##ComboFilterInputText", NULL, buffer, bufferlen, ImVec2(0, 0), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL);
    ImGui::PopItemWidth();

    if (s.activeIdx < 0) {
        IM_ASSERT(false); //Undefined behaviour
        return false;
    }


    if (!ret)
    {
        ImGui::EndChild();
        ImGui::PopItemWidth();
        EndPopup();
        IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
        return false;
    }


    ImGuiWindowFlags window_flags2 = 0; //ImGuiWindowFlags_HorizontalScrollbar
    ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, window_flags2);




    struct fuzzy {
        static int score(const char* str1, const char* str2) {
            int score = 0, consecutive = 0, maxerrors = 0;
            while (*str1 && *str2) {
                int is_leading = (*str1 & 64) && !(str1[1] & 64);
                if ((*str1 & ~32) == (*str2 & ~32)) {
                    int had_separator = (str1[-1] <= 32);
                    int x = had_separator || is_leading ? 10 : consecutive * 5;
                    consecutive = 1;
                    score += x;
                    ++str2;
                }
                else {
                    int x = -1, y = is_leading * -3;
                    consecutive = 0;
                    score += x;
                    maxerrors += y;
                }
                ++str1;
            }
            return score + (maxerrors < -9 ? -9 : maxerrors);
        }
        static int search(const char* str, std::vector<std::string>& words) {
            int scoremax = 0;
            int best = -1;
            for (int i = 0; i < words.size(); ++i) {
                int score = fuzzy::score(words[i].c_str(), str);
                int record = (score >= scoremax);
                int draw = (score == scoremax);
                if (record) {
                    scoremax = score;
                    if (!draw) best = i;
                    else best = best >= 0 && words[best].size() < words[i].size() ? best : i;
                }
            }
            return best;
        }
    };

    int new_idx = fuzzy::search(buffer, hints);
    int idx = new_idx >= 0 ? new_idx : s.activeIdx;
    s.selectionChanged = s.activeIdx != idx;
    bool selectionChangedLocal = s.selectionChanged;
    s.activeIdx = idx;

    if (done) {
        CloseCurrentPopup();
    }
    bool is_list_selected = false;
    for (int n = 0; n < hints.size(); n++) {
        ;
        bool is_selected = n == s.activeIdx;
        if (is_selected && (IsWindowAppearing() || selectionChangedLocal)) {
            SetScrollHereY();
            //            ImGui::SetItemDefaultFocus();
        }
        if (ImGui::Selectable(hints[n].c_str(), is_selected)) {
            is_list_selected = true;
            s.selectionChanged = s.activeIdx != n;
            s.activeIdx = n;
            strncpy_s(buffer, bufferlen, hints[n].c_str(), hints[n].size());
            CloseCurrentPopup();
        }
    }
    ImGui::EndChild();
    EndPopup();

    return done || is_list_selected || s.selectionChanged && hints[s.activeIdx] == buffer;
}
