#pragma once
#include "imgui.h"
#include <string>
#include <vector>

namespace ImGui {

    struct ComboFilterState {
        int  activeIdx;
        bool selectionChanged;
    };

    bool ComboFilter(const char* label, char* buffer, int bufferlen, std::vector<std::string>& hints, ComboFilterState& s, ImGuiComboFlags flags = 0);

};