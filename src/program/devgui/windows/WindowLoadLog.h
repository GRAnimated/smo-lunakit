
/*
    - LunaKit File Load Logging Window -

    This window displays a log of files which are loaded via al::FileLoader::loadArchive(). 
    The window will automatically scroll as files are loaded.
    Uses same text performance tool as the actor browser list

    Does not use categories!

    (TODO) https://github.com/Amethyst-szs/smo-lunakit/wiki
*/

#pragma once

#include "imgui.h"

#include "game/System/GameSystem.h"
#include "game/System/Application.h"

#include "program/devgui/windows/WindowBase.h"

class WindowLoadLog : public WindowBase {
public:
    WindowLoadLog(DevGuiManager* parent, const char* winName, bool isActiveByDefault, bool isAnchor, int windowPages);
    bool tryUpdateWinDisplay() override;
    void setupAnchor(int totalAnchoredWindows, int anchorIdx) override;
    
    void pushTextToWindow(const char* text);

    bool isAutoScrollEnabled() { return mIsAutoScrollEnabled; }
    int calcRoundedNum(int numToRound, int multiple);
    
private:
    bool mIsAutoScrollEnabled = true;
};