
/*
    - LunaKit Info Viewer Window -

    This window displays general information about various parts of the game
    Uses categories

    https://github.com/Amethyst-szs/smo-lunakit/wiki#info-viewer
*/

#pragma once

#include "imgui.h"

#include "program/devgui/windows/WindowBase.h"

#include "program/devgui/categories/CategoryBase.h"
#include "program/devgui/categories/info/CategoryInfSequence.h"
#include "program/devgui/categories/info/CategoryInfScene.h"
#include "program/devgui/categories/info/CategoryInfPlayer.h"

namespace al {
    class SkyParam;
}

struct GraphicsPresetSet {
    bool mIsOverride = false;
    bool mIsOverrideSky = false;
    int mTotalPreset = -1;
    int mTotalCubemap = -1;
    int mTotalSky = -1;
    const char *mPreset = "Default";
    const char *mCubemap = "LightingPlayerMoonGetStage";
    const char *mSky = "null";
    int mScenario = 1;
};

class WindowPresets : public WindowBase {
public:
    WindowPresets(DevGuiManager* parent, const char* winName, bool isActiveByDefault, bool isAnchor, int windowPages);
    bool tryUpdateWinDisplay() override;
private:
    void drawDropdown(const char* header, const char* options[], const char* translatedOptions[], const int totalOptions, const char** output);

    ImGuiComboFlags mComboFlags;
};