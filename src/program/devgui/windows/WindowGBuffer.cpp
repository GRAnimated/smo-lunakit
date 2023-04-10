#include "WindowGBuffer.h"

#include "game/GameData/GameDataHolder.h"
#include "game/GameData/GameDataFunction.h"
#include "helpers.h"

#include "devgui/DevGuiManager.h"

WindowGBuffer::WindowGBuffer(DevGuiManager* parent, const char* winName, bool isActiveByDefault, bool isAnchor, int windowPages)
    : WindowBase(parent, winName, isActiveByDefault, isAnchor, windowPages)
{

}

bool WindowGBuffer::tryUpdateWinDisplay() {
    auto settings = DevGuiManager::instance()->getGBufferSettings();

    ImGui::Checkbox("Enabled", &settings->mEnable);

    const char* imageTypes[] = { "Base Color", "Normal", "Depth", "Light", "Motion" };
    ImGui::Combo("Select Image Type", &settings->mShowType, imageTypes, 5);

    ImGui::SliderInt("Channel", &settings->mChannel, 0, 5);
    
    return true;
}