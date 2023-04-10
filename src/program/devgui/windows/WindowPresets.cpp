#include "WindowPresets.h"

#include "game/GameData/GameDataHolder.h"
#include "game/GameData/GameDataFunction.h"
#include "helpers.h"

#include "devgui/DevGuiManager.h"

WindowPresets::WindowPresets(DevGuiManager* parent, const char* winName, bool isActiveByDefault, bool isAnchor, int windowPages)
    : WindowBase(parent, winName, isActiveByDefault, isAnchor, windowPages)
{
    auto settings = DevGuiManager::instance()->getPresetSettings();
    settings->mTotalPreset = IM_ARRAYSIZE(presetList);
    settings->mTotalCubemap = IM_ARRAYSIZE(cubemapList);
    settings->mTotalSky = IM_ARRAYSIZE(skyList);
}

bool WindowPresets::tryUpdateWinDisplay() {
    auto settings = DevGuiManager::instance()->getPresetSettings();
    ImGui::Checkbox("Enabled", &settings->mIsOverride);

    static bool translate = false;
    ImGui::Checkbox("Translate List", &translate);

    ImGui::Checkbox("Override Sky", &settings->mIsOverrideSky);

    bool reload = ImGui::Button("Reload Scene");

    if (translate)
        drawDropdown("Preset", presetList, presetListTranslated, settings->mTotalPreset, &settings->mPreset);
    else
        drawDropdown("Preset", presetList, presetList, settings->mTotalPreset, &settings->mPreset);
    
    if (settings->mIsOverrideSky) drawDropdown("Sky", skyList, skyList, settings->mTotalSky, &settings->mSky);
    drawDropdown("Cubemap", cubemapList, cubemapList, settings->mTotalCubemap, &settings->mCubemap);
    ImGui::SliderInt("Cubemap Scenario", &settings->mScenario, 1, 16);

    if (reload) {
        StageScene* scene = tryGetStageScene();
        if (scene) {
            GameDataHolder* holder = tryGetGameDataHolder();
            ChangeStageInfo stageInfo(holder, "start", GameDataFunction::getCurrentStageName(scene->mHolder), false, -1, ChangeStageInfo::SubScenarioType::UNK);
            GameDataFunction::tryChangeNextStage(scene->mHolder, &stageInfo);
        }
    }
    return true;
}

void WindowPresets::drawDropdown(const char* header, const char* options[], const char* translatedOptions[], const int totalOptions, const char** output)
{
    if(ImGui::BeginCombo(header, *output, mComboFlags)) {
        for(int n = 0; n < totalOptions; n++) {
            bool is_selected = (*output == options[n]); // You can store your selection however you want, outside or inside your objects
            if (ImGui::Selectable(translatedOptions[n], is_selected))
                *output = options[n];
            if (is_selected)
                ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
        }

        ImGui::EndCombo();
    }
}