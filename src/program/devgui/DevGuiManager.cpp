#include "program/devgui/DevGuiManager.h"

SEAD_SINGLETON_DISPOSER_IMPL(DevGuiManager)
DevGuiManager::DevGuiManager() = default;
DevGuiManager::~DevGuiManager() = default;

void DevGuiManager::init(sead::Heap* heap)
{
    mDevGuiHeap = heap;
    sead::ScopedCurrentHeapSetter heapSetter(heap);

    mWindows.allocBuffer(0x10, mDevGuiHeap);
    mHomeMenuTabs.allocBuffer(0x10, mDevGuiHeap);
    mIsActive = false;

    mCustomList = new CustomStageManager();
    mCustomList->init(heap);
    
    // Create all display windows

    WindowMemoryManage* memWindow = new WindowMemoryManage(this, "LunaKit Memory Manager", mDevGuiHeap);
    mWindows.pushBack(memWindow);

    WindowEditor* editorWindow = new WindowEditor(this, "LunaKit Param Editor", mDevGuiHeap);
    mWindows.pushBack(editorWindow);

    WindowInfo* infoWindow = new WindowInfo(this, "LunaKit Info Viewer", mDevGuiHeap);
    mWindows.pushBack(infoWindow);

    WindowFPS* fpsWindow = new WindowFPS(this, "FPS Window", mDevGuiHeap);
    mWindows.pushBack(fpsWindow);

    // Create all home menu tabs
    HomeMenuFile* homeFile = new HomeMenuFile(this, "File", mDevGuiHeap);
    mHomeMenuTabs.pushBack(homeFile);

    HomeMenuSettings* homeSetting = new HomeMenuSettings(this, "Settings", mDevGuiHeap);
    mHomeMenuTabs.pushBack(homeSetting);

    HomeMenuWindows* homeWindows = new HomeMenuWindows(this, "Windows", mDevGuiHeap);
    mHomeMenuTabs.pushBack(homeWindows);

    HomeMenuWorlds* homeWorld = new HomeMenuWorlds(this, "Kingdoms", mDevGuiHeap);
    mHomeMenuTabs.pushBack(homeWorld);

    HomeMenuCStages* homeCStages = new HomeMenuCStages(this, "Stages", mDevGuiHeap);
    mHomeMenuTabs.pushBack(homeCStages);

    HomeMenuExtra* homeExtra = new HomeMenuExtra(this, "Extras", mDevGuiHeap);
    mHomeMenuTabs.pushBack(homeExtra);
}

void DevGuiManager::update()
{
    // Check for enabling and disabling the window
    if (al::isPadHoldR(-1) && al::isPadHoldZR(-1) && al::isPadTriggerL(-1)) {
        mIsActive = !mIsActive;
        if (mIsActive)
            mIsFirstStep = true;
    }

    // Note: Each window's update function runs even with the menu closed/inactive!
    for (int i = 0; i < mWindows.size(); i++) {
        auto* entry = mWindows.at(i);
        entry->updateWin();
    }
}

void DevGuiManager::updateDisplay()
{
    updateCursorState();

    if(!mIsActive)
        return;

    // Load and draw all windows (and update anchors if needed)
    int totalAnchorWin = calcTotalAnchoredWindows();
    int curAnchorWin = 0;

    for (int i = 0; i < mWindows.size(); i++) {
        auto* entry = mWindows.at(i);
        if(!entry->isActive())
            continue;

        ImGui::Begin(entry->getWindowName(), NULL, entry->getWindowConfig()->mWindowFlags);

        if(mIsAnchorChange) {
            entry->setupAnchor(totalAnchorWin, curAnchorWin);
            curAnchorWin++;
        }

        entry->tryUpdateWinDisplay();
    }

    mIsAnchorChange = false;
    
    // Load and draw all home menu tabs
    if (ImGui::BeginMainMenuBar()) {
        ImGui::SetWindowFontScale(1.5f);
        
        for (int i = 0; i < mHomeMenuTabs.size(); i++) {
            auto* entry = mHomeMenuTabs.at(i);
            if (ImGui::BeginMenu(entry->getMenuName())) {
                entry->updateMenu();

                ImGui::EndMenu();
            }
        }

        ImGui::EndMainMenuBar();
    }

    // Draw the demo window if the settings class has it enabled
    if(mIsDisplayImGuiDemo)
        ImGui::ShowDemoWindow();

    // Reset the first step flag when complete!
    if (mIsFirstStep)
        mIsFirstStep = false;
}

void DevGuiManager::updateCursorState()
{
    if (!mIsActive)
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);

    if (mIsFirstStep)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
}

int DevGuiManager::calcTotalAnchoredWindows()
{
    int total = 0;
    for (int i = 0; i < mWindows.size(); i++) {
        auto* entry = mWindows.at(i);

        if(*(entry->getActiveState()) && entry->isInAnchorList())
            total++;
    }

    return total;
}