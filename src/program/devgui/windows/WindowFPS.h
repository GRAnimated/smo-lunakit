#pragma once

#include "imgui.h"

#include "program/devgui/DevGuiWindowConfig.h"
#include "program/devgui/windows/WindowBase.h"

#include "program/devgui/categories/CategoryBase.h"

class WindowFPS : public WindowBase {
public:
    WindowFPS(DevGuiManager* parent, const char* winName, bool active, sead::Heap* heap);

    void updateWin() override;
    bool tryUpdateWinDisplay() override;

    void setupAnchor(int totalAnchoredWindows, int anchorIdx) override;

private:
    static const int mHistorySize = 200;
    float mHistory[mHistorySize] = {};
    int mHistoryOffset = 0;
    
    double mRefreshTime = 0.f;
};