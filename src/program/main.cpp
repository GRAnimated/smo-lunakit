#include "lib.hpp"
#include "imgui_backend/imgui_impl_nvn.hpp"
#include "patches.hpp"
#include "fs.h"

#include <basis/seadRawPrint.h>
#include <prim/seadSafeString.h>
#include <resource/seadResourceMgr.h>
#include <filedevice/nin/seadNinSDFileDeviceNin.h>
#include <filedevice/seadFileDeviceMgr.h>
#include <filedevice/seadPath.h>
#include <resource/seadArchiveRes.h>
#include <heap/seadHeapMgr.h>
#include <devenv/seadDebugFontMgrNvn.h>
#include <gfx/seadTextWriter.h>
#include <gfx/seadViewport.h>
#include <gfx/seadPrimitiveRenderer.h>

#include "rs/util.hpp"

#include "game/StageScene/StageScene.h"
#include "game/System/GameSystem.h"
#include "game/System/Application.h"
#include "game/HakoniwaSequence/HakoniwaSequence.h"
#include "game/Player/PlayerFunction.h"
#include "game/GameData/GameProgressData.h"

#include "al/util.hpp"
#include "al/util/LiveActorUtil.h"
#include "al/byaml/ByamlIter.h"
#include "al/fs/FileLoader.h"
#include "al/resource/Resource.h"
#include "al/collision/KCollisionServer.h"
#include "al/collision/alCollisionUtil.h"

#include "agl/utl.h"
#include "imgui_nvn.h"
#include "helpers/InputHelper.h"
#include "init.h"
#include "helpers/PlayerHelper.h"
#include "helpers.h"
#include "game/GameData/GameDataFunction.h"

#include "nerve/scene/NrvStageScene.h"

#include "statics.h"

#include "program/GetterUtil.h"
#include "program/devgui/DevGuiManager.h"
#include "program/devgui/DevGuiPrimitive.h"

#include <typeinfo>

static const char* DBG_FONT_PATH = "ImGuiData/Font/nvn_font_jis1.ntx";
static const char* DBG_SHADER_PATH = "ImGuiData/Font/nvn_font_shader_jis1.bin";
static const char* DBG_TBL_PATH = "ImGuiData/Font/nvn_font_jis1_tbl.bin";

#define IMGUI_ENABLED true

sead::TextWriter *gTextWriter;

void drawBackground() {

    agl::DrawContext *context = Application::instance()->mDrawInfo->mDrawContext;

    sead::Vector3<float> p1(-1, .3, 0); // top left
    sead::Vector3<float> p2(-.2, .3, 0); // top right
    sead::Vector3<float> p3(-1, -1, 0); // bottom left
    sead::Vector3<float> p4(-.2, -1, 0); // bottom right
    sead::Color4f c(.1, .1, .1, .9);

    agl::utl::DevTools::beginDrawImm(context, sead::Matrix34<float>::ident, sead::Matrix44<float>::ident);
    agl::utl::DevTools::drawTriangleImm(context, p1, p2, p3, c);
    agl::utl::DevTools::drawTriangleImm(context, p3, p4, p2, c);
}

void drawCursor() {
    agl::DrawContext *context = Application::instance()->mDrawInfo->mDrawContext;

    nn::hid::MouseState state{};
    nn::hid::GetMouseState(&state);
    agl::utl::DevTools::beginDrawImm(context, sead::Matrix34<float>::ident,
                                     sead::Matrix44<float>::ident);
    sead::Vector2f screenSize = sead::Vector2f(1280.0f, 720.0f) / 2.0f;
    agl::utl::DevTools::drawCursor(context, screenSize, sead::Vector2f(((float) state.x - screenSize.x) / screenSize.x,
                                                                       ((float) -state.y + screenSize.y) /
                                                                       screenSize.y), 0.5f);
}

void drawImGuiDebug() {

    auto bd = ImguiNvnBackend::getBackendData();
    ImGuiIO io = ImGui::GetIO();

    ImVec2 mousePos;
    InputHelper::getMouseCoords(&mousePos.x, &mousePos.y);
    ImVec2 scrollDelta;
    InputHelper::getScrollDelta(&scrollDelta.x, &scrollDelta.y);

    gTextWriter->setCursorFromTopLeft(sead::Vector2f(10.f, 480.f));

    nn::TimeSpan curTick = nn::os::GetSystemTick().ToTimeSpan();
    nn::TimeSpan prevTick(bd->lastTick);
    float sec = fabsf((double) (curTick - prevTick).GetNanoSeconds() / 1000000000.0);

    gTextWriter->printf("Delta Time: (u64) %llu (float) %f\n", sec, (float) sec);

    gTextWriter->printf("ImGui Delta Time: %f\n", io.DeltaTime);

    gTextWriter->printf("Mouse Coords: X: %f Y: %f\n", mousePos.x, mousePos.y);
    gTextWriter->printf("Scroll Delta: X: %f Y: %f\n", scrollDelta.x, scrollDelta.y);
}

void graNoclipCode(al::LiveActor *player) {
    static float speed = 25.0f;
    static float speedMax = 150.0f;
    static float vspeed = 20.0f;
    static float speedGain = 0.0f;

    sead::Vector3f *playerPos = al::getTransPtr(player);
    sead::Vector3f *cameraPos = al::getCameraPos(player, 0);
    sead::Vector2f *leftStick = al::getLeftStick(-1);

    // Its better to do this here because loading zones reset this.
    al::offCollide(player);
    al::setVelocityZero(player);

    // Mario slightly goes down even when velocity is 0. This is a hacky fix for that.
    playerPos->y += 1.4553f;

    float d = sqrt(al::powerIn(playerPos->x - cameraPos->x, 2) + (al::powerIn(playerPos->z - cameraPos->z, 2)));
    float vx = ((speed + speedGain) / d) * (playerPos->x - cameraPos->x);
    float vz = ((speed + speedGain) / d) * (playerPos->z - cameraPos->z);

    if (!al::isPadHoldZR(-1)) {
        playerPos->x -= leftStick->x * vz;
        playerPos->z += leftStick->x * vx;

        playerPos->x += leftStick->y * vx;
        playerPos->z += leftStick->y * vz;

        if (al::isPadHoldX(-1)) speedGain -= 0.5f;
        if (al::isPadHoldY(-1)) speedGain += 0.5f;
        if (speedGain <= 0.0f) speedGain = 0.0f;
        if (speedGain >= speedMax) speedGain = speedMax;

        if (al::isPadHoldZL(-1) || al::isPadHoldA(-1)) playerPos->y -= (vspeed + speedGain / 6);
        if (al::isPadHoldB(-1)) playerPos->y += (vspeed + speedGain / 6);
    }
}

void controlLol(StageScene *scene) {
    auto actor = rs::getPlayerActor(scene);

    if(Statics::isNoclip)
        graNoclipCode(actor);
    
    if(!Statics::isHUD && scene->mSceneLayout->isWait()) {
        scene->mSceneLayout->end();
        MapMini* compass = scene->mSceneLayout->mMapMiniLyt;
        if (compass->mIsAlive) compass->end();
    }

    if(!Statics::isPlayMusic) {
        if (al::isPlayingBgm(scene)) {
            al::stopAllBgm(scene, 0);
        }
    }
}

static void drawPlayerInfo(al::LiveActor* actor) {
    // Translation
    static float trans[3];
    static bool transLock = false;
    ImGui::Checkbox("Translation Lock", &transLock);
    if(!transLock) {
        sead::Vector3f seadTrans = al::getTrans(actor);
        trans[0] = seadTrans.x;
        trans[1] = seadTrans.y;
        trans[2] = seadTrans.z;
    }
    ImGui::DragFloat3("Translation", trans, 5.f);
    if(transLock)
        al::setTrans(actor, {trans[0], trans[1], trans[2]});

    // Velocity
    static float velocity[3];
    static bool velocityLock = false;
    ImGui::Checkbox("Velocity Lock", &velocityLock);
    if(!velocityLock) {
        sead::Vector3f seadVelocity = al::getVelocity(actor);
        velocity[0] = seadVelocity.x;
        velocity[1] = seadVelocity.y;
        velocity[2] = seadVelocity.z;
    }
    ImGui::DragFloat3("Velocity", velocity, 0.5f);
    if(velocityLock)
        al::setVelocity(actor, {velocity[0], velocity[1], velocity[2]});

    // Gravity
    float gravity[3];
    sead::Vector3f seadGravity = al::getGravity(actor);
    gravity[0] = seadGravity.x;
    gravity[1] = seadGravity.y;
    gravity[2] = seadGravity.z;
    ImGui::DragFloat3("Gravity", gravity);

    // Quaternion
    float quat[4];
    sead::Quatf seadQuat = al::getQuat(actor);
    quat[0] = seadQuat.w;
    quat[1] = seadQuat.x;
    quat[2] = seadQuat.y;
    quat[3] = seadQuat.z;
    ImGui::DragFloat4("Quaternion", quat);

    // Front
    float front[3];
    sead::Vector3f frontAngle;
    al::calcFrontDir(&frontAngle, actor);
    front[0] = frontAngle.x;
    front[1] = frontAngle.y;
    front[2] = frontAngle.z;
    ImGui::DragFloat3("Front", front);
}

void drawDebugWindow() {
    if(!DevGuiManager::instance()->isMenuActive()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        return;
    }
    
    if(DevGuiManager::instance()->isFirstStep())
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

    al::Sequence *curSequence = GameSystemFunction::getGameSystem()->mCurSequence;

    ImGui::Begin("Mod Debugging Tools");
    ImGui::SetWindowPos(ImVec2(860, 220), ImGuiCond_FirstUseEver);
    ImGui::SetWindowSize(ImVec2(400, 485), ImGuiCond_FirstUseEver);
    ImGui::SetWindowFontScale(1.5f);

    if (curSequence && al::isEqualString(curSequence->getName().cstr(), "HakoniwaSequence")) {
        auto gameSeq = (HakoniwaSequence *) curSequence;
        auto curScene = gameSeq->curScene;
        bool isInGame = curScene && curScene->mIsAlive && al::isEqualString(curScene->mName.cstr(), "StageScene");

        if(ImGui::CollapsingHeader("Settings")) {
            ImGui::Checkbox("Play Music", &Statics::isPlayMusic);
            ImGui::Checkbox("Autosaving", &Statics::isUseAutosaves);   
            ImGui::Checkbox("Show HUD", &Statics::isHUD);
            ImGui::Checkbox("Noclip Flight", &Statics::isNoclip);
        }

        if (isInGame) {
            StageScene *stageScene = (StageScene *) gameSeq->curScene;
            PlayerActorBase *playerBase = rs::getPlayerActor(stageScene);

            if(!Statics::isNoclip && !rs::isActiveDemo(playerBase))
                al::onCollide(playerBase);

            if(ImGui::CollapsingHeader("Player Info")) {
                ImGui::SetWindowFontScale(1.2f);
                ImGui::BulletText("Is Dead: %s", BTOC(PlayerFunction::isPlayerDeadStatus(playerBase)));
                ImGui::BulletText("Is Active Demo: %s", BTOC(rs::isActiveDemo(playerBase)));
                ImGui::BulletText("Is Grounded: %s", BTOC(rs::isPlayerOnGround(playerBase)));

                if (ImGui::TreeNode("Main Player")) {
                    drawPlayerInfo(playerBase);
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("HackCap")) {
                    bool isYukimaru = !playerBase->getPlayerInfo();
                    if(!isYukimaru)
                        drawPlayerInfo(((PlayerActorHakoniwa*)playerBase)->mHackCap);
                    
                    ImGui::TreePop();
                }
                ImGui::SetWindowFontScale(1.5f);
            }
        } // Check is in game

        if(ImGui::CollapsingHeader("Input Info")) {
            sead::Vector2f* lStick = al::getLeftStick(-1);
            sead::Vector2f* rStick = al::getRightStick(-1);
            float stickArray[2];

            stickArray[0] = lStick->x;
            stickArray[1] = lStick->y;
            ImGui::DragFloat2("Left Stick", stickArray);

            stickArray[0] = rStick->x;
            stickArray[1] = rStick->y;
            ImGui::DragFloat2("Right Stick", stickArray);

            if(al::isPadHoldA(-1))
                ImGui::BulletText("Face - A");
            if(al::isPadHoldB(-1))
                ImGui::BulletText("Face - B");
            if(al::isPadHoldX(-1))
                ImGui::BulletText("Face - X");
            if(al::isPadHoldY(-1))
                ImGui::BulletText("Face - Y");
            if(al::isPadHoldL(-1))
                ImGui::BulletText("Shoulder - L");
            if(al::isPadHoldR(-1))
                ImGui::BulletText("Shoulder - R");
            if(al::isPadHoldZL(-1))
                ImGui::BulletText("Trigger - ZL");
            if(al::isPadHoldZR(-1))
                ImGui::BulletText("Trigger - ZR");
        }

    } // Check sequence

    ImGui::End();
}

void drawLunaKit() {
    DevGuiManager::instance()->updateDisplay();
}

HOOK_DEFINE_TRAMPOLINE(ControlHook) {
    static void Callback(StageScene *scene) {
        controlLol(scene);
        Orig(scene);
    }
};

HOOK_DEFINE_TRAMPOLINE(CreateFileDeviceMgr) {
    static void Callback(sead::FileDeviceMgr *thisPtr) {

        Orig(thisPtr);

        thisPtr->mMountedSd = nn::fs::MountSdCardForDebug("sd");

        sead::NinSDFileDevice *sdFileDevice = new sead::NinSDFileDevice();

        thisPtr->mount(sdFileDevice);
    }
};

HOOK_DEFINE_TRAMPOLINE(RedirectFileDevice) {
    static sead::FileDevice *
    Callback(sead::FileDeviceMgr *thisPtr, sead::SafeString &path, sead::BufferedSafeString *pathNoDrive) {

        sead::FixedSafeString<32> driveName;
        sead::FileDevice *device;

        // Logger::log("Path: %s\n", path.cstr());

        if (!sead::Path::getDriveName(&driveName, path)) {

            device = thisPtr->findDevice("sd");

            if (!(device && device->isExistFile(path))) {

                device = thisPtr->getDefaultFileDevice();

                if (!device) {
                    return nullptr;
                }

            } else {
            }

        } else
            device = thisPtr->findDevice(driveName);

        if (!device)
            return nullptr;

        if (pathNoDrive != nullptr)
            sead::Path::getPathExceptDrive(pathNoDrive, path);

        return device;
    }
};

HOOK_DEFINE_TRAMPOLINE(FileLoaderLoadArc) {
    static sead::ArchiveRes *
    Callback(al::FileLoader *thisPtr, sead::SafeString &path, const char *ext, sead::FileDevice *device) {
        
        sead::FileDevice *sdFileDevice = sead::FileDeviceMgr::instance()->findDevice("sd");

        if (sdFileDevice && sdFileDevice->isExistFile(path))
            device = sdFileDevice;

        return Orig(thisPtr, path, ext, device);
    }
};

HOOK_DEFINE_TRAMPOLINE(FileLoaderIsExistFile) {
    static bool Callback(al::FileLoader *thisPtr, sead::SafeString &path, sead::FileDevice *device) {

        sead::FileDevice *sdFileDevice = sead::FileDeviceMgr::instance()->findDevice("sd");

        if (sdFileDevice && sdFileDevice->isExistFile(path))
            device = sdFileDevice;

        return Orig(thisPtr, path, device);
    }
};

HOOK_DEFINE_TRAMPOLINE(FileLoaderIsExistArchive) {
    static bool Callback(al::FileLoader *thisPtr, sead::SafeString &path, sead::FileDevice *device) {
        sead::FileDevice *sdFileDevice = sead::FileDeviceMgr::instance()->findDevice("sd");

        if (sdFileDevice && sdFileDevice->isExistFile(path))
            device = sdFileDevice;

        return Orig(thisPtr, path, device);
    }
};

HOOK_DEFINE_TRAMPOLINE(SaveHook) {
    static bool Callback(StageScene* scene) {
        if (Statics::isUseAutosaves)
            return Orig(scene);
        else
            return false;
    }
};

HOOK_DEFINE_TRAMPOLINE(GameSystemInit) {
    static void Callback(GameSystem *thisPtr) {
        sead::Heap *curHeap = sead::HeapMgr::instance()->getCurrentHeap();

        sead::DebugFontMgrJis1Nvn::createInstance(curHeap);

        if (al::isExistFile(DBG_SHADER_PATH) && al::isExistFile(DBG_FONT_PATH) && al::isExistFile(DBG_TBL_PATH)) {
            sead::DebugFontMgrJis1Nvn::instance()->initialize(curHeap, DBG_SHADER_PATH, DBG_FONT_PATH, DBG_TBL_PATH,
                                                              0x100000);
        }

        DevGuiManager::createInstance(curHeap);
        DevGuiManager::instance()->init(curHeap);

        DevGuiPrimitive::createInstance(curHeap);

        sead::TextWriter::setDefaultFont(sead::DebugFontMgrJis1Nvn::instance());

        al::GameDrawInfo *drawInfo = Application::instance()->mDrawInfo;

        agl::DrawContext *context = drawInfo->mDrawContext;
        agl::RenderBuffer *renderBuffer = drawInfo->mFirstRenderBuffer;

        sead::Viewport *viewport = new sead::Viewport(*renderBuffer);

        gTextWriter = new sead::TextWriter(context, viewport);

        gTextWriter->setupGraphics(context);

        gTextWriter->mColor = sead::Color4f(1.f, 1.f, 1.f, 0.8f);

        Orig(thisPtr);

    }
};

HOOK_DEFINE_TRAMPOLINE(DrawDebugMenu) {
    static void Callback(HakoniwaSequence *thisPtr) {
        Orig(thisPtr);

        DevGuiManager::instance()->update();

        if(!DevGuiManager::instance()->isMenuActive())
            return;
        
        agl::DrawContext* drawContext = thisPtr->getDrawInfo()->mDrawContext;
        DevGuiPrimitive::instance()->draw(drawContext);
    }
};

extern "C" void exl_main(void *x0, void *x1) {
    /* Setup hooking enviroment. */
    envSetOwnProcessHandle(exl::util::proc_handle::Get());
    exl::hook::Initialize();

    runCodePatches();

    GameSystemInit::InstallAtOffset(0x535850);

    // SD File Redirection

    RedirectFileDevice::InstallAtOffset(0x76CFE0);
    FileLoaderLoadArc::InstallAtOffset(0xA5EF64);
    CreateFileDeviceMgr::InstallAtOffset(0x76C8D4);
    FileLoaderIsExistFile::InstallAtOffset(0xA5ED28);
    FileLoaderIsExistArchive::InstallAtOffset(0xA5ED74);

    // Debug Text Writer Drawing
    DrawDebugMenu::InstallAtOffset(0x50F1D8);
    SaveHook::InstallAtSymbol("_ZNK10StageScene12isEnableSaveEv");

    // General Hooks
    ControlHook::InstallAtSymbol("_ZN10StageScene7controlEv");

    // ImGui Hooks
#if IMGUI_ENABLED
    nvnImGui::InstallHooks();

    nvnImGui::addDrawFunc(drawDebugWindow);
    nvnImGui::addDrawFunc(drawLunaKit);
#endif

}

extern "C" NORETURN void exl_exception_entry() {
    /* TODO: exception handling */
    EXL_ABORT(0x420);
}