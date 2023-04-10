
/*
    Looking for LunaKit code?
    Head to src/program/devgui/DevGuiManager.h to get started!
*/



#include "devgui/windows/WindowPresets.h"
#include "lib.hpp"
#include "imgui_backend/imgui_impl_nvn.hpp"
#include "patches.hpp"
#include "fs.h"

#include "logger/Logger.hpp"
#include "logger/LoadLogger.hpp"

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
#include "game/Player/PlayerTrigger.h"
#include "game/GameData/GameProgressData.h"
#include "game/GameData/GameDataHolderWriter.h"
#include "game/GameData/GameDataFile.h"


#include "al/util.hpp"
#include "al/util/LiveActorUtil.h"
#include "al/byaml/ByamlIter.h"
#include "al/fs/FileLoader.h"
#include "al/resource/Resource.h"
#include "al/collision/KCollisionServer.h"
#include "al/collision/alCollisionUtil.h"

#include "imgui_nvn.h"
#include "helpers/InputHelper.h"
#include "init.h"
#include "helpers/PlayerHelper.h"
#include "helpers.h"
#include "game/GameData/GameDataFunction.h"
#include "al/LiveActor/LiveActor.h"
#include "game/StageScene/StageScene.h"
#include "logger/Logger.hpp"
#include "os/os_tick.hpp"
#include "patch/code_patcher.hpp"

#include "helpers/GetHelper.h"
#include "devgui/DevGuiManager.h"
#include "devgui/settings/HooksSettings.h"

#include <typeinfo>

#include "ExceptionHandler.h"

namespace patch = exl::patch;
namespace inst = exl::armv8::inst;
namespace reg = exl::armv8::reg;

static const char* DBG_FONT_PATH = "ImGuiData/Font/nvn_font_jis1.ntx";
static const char* DBG_SHADER_PATH = "ImGuiData/Font/nvn_font_shader_jis1.bin";
static const char* DBG_TBL_PATH = "ImGuiData/Font/nvn_font_jis1_tbl.bin";

#define IMGUI_ENABLED true

sead::TextWriter *gTextWriter;

void drawLunaKit() {
    DevGuiManager::instance()->updateDisplay();
}

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
        LoadLog::pushTextToVector(path.cstr());
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

HOOK_DEFINE_REPLACE(ReplaceSeadPrint) {
    static void Callback(const char *format, ...) {
        va_list args;
        va_start(args, format);
        Logger::log(format, args);
        va_end(args);
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

        sead::Heap* lkHeap = sead::ExpHeap::create(256000, "LunaKitHeap", al::getStationedHeap(), 8,
            sead::Heap::HeapDirection::cHeapDirection_Reverse, false);

        Logger::instance().init(lkHeap).value;

        DevGuiManager::createInstance(lkHeap);
        DevGuiManager::instance()->init(lkHeap);

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

HOOK_DEFINE_TRAMPOLINE(UpdateLunaKit) {
    static void Callback(HakoniwaSequence *thisPtr) {
        Orig(thisPtr);
        DevGuiManager::instance()->update();
    }
};

namespace al {
    class GraphicsSystemInfo;
}

HOOK_DEFINE_TRAMPOLINE(CubeMapSetter) {
    static void Callback(al::GraphicsSystemInfo *i, al::Resource const*r, char const*a, char const*b) {
        a = "SandWorldHomeStage";
        Orig(i, r, a, b);
        svcOutputDebugString(a, 0x50);
    }
};

HOOK_DEFINE_TRAMPOLINE(PresetTest) {
    static void Callback(al::Scene *s, const char *stage, int scenario) {
        //stage = "SandWorldHomeStage";
        //scenario = 0;
        rs::registerGraphicsPresetPause(s);
        Orig(s, stage, scenario);
        //svcOutputDebugString(stage, 0x50);
    }
};

namespace al {
    class GraphicsPresetDirector {
        public:
        void registerPreset(char const*,char const*,char const*,bool);
    };
}

HOOK_DEFINE_TRAMPOLINE(StageSceneControlHook) {
    static void Callback(StageScene *scene) {
        Orig(scene);
    }
};

HOOK_DEFINE_TRAMPOLINE(InitGraphicsInfoHook) {
    static void Callback(al::Scene *scene, char const *stage, int scenario) {
        auto settings = DevGuiManager::instance()->getPresetSettings();
        if (settings->mIsOverride) {
            scenario = settings->mScenario;
        }

        Orig(scene, stage, scenario);
    }
};

HOOK_DEFINE_TRAMPOLINE(RegisterPresetHook) {
    static void Callback(al::GraphicsPresetDirector *presetDirector, char const* preset, char const* cubemap_location, char const* file, bool d) {
        auto settings = DevGuiManager::instance()->getPresetSettings();

        if (settings->mIsOverride) {
            preset = settings->mPreset;
            file = settings->mCubemap;
            cubemap_location = "Default";
        }

        Orig(presetDirector, preset, cubemap_location, file, d);
    }
};

HOOK_DEFINE_TRAMPOLINE(RequestPresetHook) {
    static void Callback(al::GraphicsPresetDirector *presetDirector, char const* preset, int b, int c, int d, sead::Vector3<float> const& rot) {
        auto settings = DevGuiManager::instance()->getPresetSettings();
        if (settings->mIsOverride) {
            preset = settings->mPreset;
            b = 1000;
            c = 0;
            d = 0;
        }

        Orig(presetDirector, preset, b, c, d, rot);
    }
};

HOOK_DEFINE_TRAMPOLINE(RequestCubeMapHook) {
    static void Callback(void *cubeMapKeeper, int a, char const* location, char const* file) {
        auto settings = DevGuiManager::instance()->getPresetSettings();
        if (settings->mIsOverride) {
            a = 1000;
            location = "Default";
            file = settings->mCubemap;
        }

        Orig(cubeMapKeeper, a, location, file);
    }
};

void SkyInitHook(al::LiveActor *actor, al::ActorInitInfo const& info, sead::SafeStringBase<char> const& preset, char const* unk) {
    auto settings = DevGuiManager::instance()->getPresetSettings();
    if (settings->mIsOverride && settings->mIsOverrideSky) {
        return al::initActorWithArchiveName(actor, info, settings->mSky, unk);
    }
    al::initActorWithArchiveName(actor, info, preset, unk);
}

#include <gfx/seadGraphicsContext.h>
#include <agl/TextureSampler.h>
#include <agl/DevTools.h>

void ViewportApplyHook(sead::Viewport *viewport, agl::DrawContext *ctx, agl::RenderBuffer *buffer) {
    auto settings = DevGuiManager::instance()->getGBufferSettings();

    if (settings->mBuffer && settings->mEnable) {
        auto b = settings->mBuffer;
        agl::TextureData *apply = b->mGBufBaseColor;

        switch (settings->mShowType) {
            case 0: // Base Color
                apply = b->mGBufBaseColor;
                break;
            case 1: // Normal
                apply = b->mGBufNrmWorld;
                break;
            case 2: // Depth
                apply = b->mGBufDepthView;
                break;
            case 3: // Light
                apply = b->mGBufLightBuffer;
                break;
            case 4: // Motion
                apply = b->mGBufMotionVec;
                break;
        }

        sead::GraphicsContext context;
        context.apply(ctx);

        auto sampler = agl::TextureSampler();
        sampler.applyTextureData(*apply);

        agl::utl::ImageFilter2D::drawColorQuadTriangle(ctx, sead::Color4f::cBlack, 1.0f);
        agl::utl::ImageFilter2D::drawTextureChannel(ctx, sampler, *viewport, settings->mChannel, sead::Vector2f(1280.f/apply->mSurface.mWidth, 720.f/apply->mSurface.mHeight), sead::Vector2f::zero);
    
    }

    sead::Camera *cam;
    __asm("MOV %0, X28" : "=r" (cam));

    sead::Projection *projection;
    __asm("MOV %0, X26" : "=r" (projection));

    viewport->apply(ctx, (sead::LogicalFrameBuffer &)buffer);
}

HOOK_DEFINE_TRAMPOLINE(GetTexBufferHook) {
    static void Callback(al::GBufferArray *buf) {
        DevGuiManager::instance()->getGBufferSettings()->mBuffer = buf;

        Orig(buf);
    }
};

extern "C" void exl_main(void *x0, void *x1) {
    /* Setup hooking enviroment. */
    // envSetOwnProcessHandle(exl::util::proc_handle::Get());
    exl::hook::Initialize();

    nn::os::SetUserExceptionHandler(exception_handler, nullptr, 0, nullptr);

    runCodePatches();

    GameSystemInit::InstallAtOffset(0x535850);
    ReplaceSeadPrint::InstallAtOffset(0xB59E28);

    //StageSceneControlHook::InstallAtOffset(0x4CC348);

    // SD File Redirection
    RedirectFileDevice::InstallAtOffset(0x76CFE0);
    FileLoaderLoadArc::InstallAtOffset(0xA5EF64);
    CreateFileDeviceMgr::InstallAtOffset(0x76C8D4);
    FileLoaderIsExistFile::InstallAtOffset(0xA5ED28);
    FileLoaderIsExistArchive::InstallAtOffset(0xA5ED74);

    // Debug Text Writer Drawing
    UpdateLunaKit::InstallAtOffset(0x50F1D8);

    InitGraphicsInfoHook::InstallAtOffset(0x9D0294);
    RegisterPresetHook::InstallAtOffset(0x8764C0);
    RequestPresetHook::InstallAtOffset(0x876FF0);
    RequestCubeMapHook::InstallAtOffset(0xA02F3C);
    patch::CodePatcher p(0x96F848);
    p.BranchLinkInst((void*)SkyInitHook);

    GetTexBufferHook::InstallAtOffset(0x9FEB70);
    exl::patch::CodePatcher render(0x0087FF74);
    render.BranchLinkInst((void*) ViewportApplyHook);

    // DevGui cheats
    exlSetupSettingsHooks(); // Located in devgui/settings/HooksSettings

    // ImGui Hooks
#if IMGUI_ENABLED
    nvnImGui::InstallHooks();

    nvnImGui::addDrawFunc(drawLunaKit);
#endif
}

extern "C" NORETURN void exl_exception_entry() {
    /* TODO: exception handling */
    EXL_ABORT(0x420);
}
