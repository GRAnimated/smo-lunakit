#pragma once

#include "imgui.h"

#include "program/devgui/windows/WindowBase.h"
#include "al/graphics/GBufferArray.h"

struct GBufferSet {
    al::GBufferArray *mBuffer = nullptr;
    bool mEnable = false;
    int mShowType = 0;
    int mChannel = 0;
};

class WindowGBuffer : public WindowBase {
public:
    WindowGBuffer(DevGuiManager* parent, const char* winName, bool isActiveByDefault, bool isAnchor, int windowPages);
    bool tryUpdateWinDisplay() override;

    ImGuiComboFlags mComboFlags;
};