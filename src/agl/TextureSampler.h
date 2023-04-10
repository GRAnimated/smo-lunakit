#pragma once

#include "agl/util.h"
#include "detail/Surface.h"
#include "driver/NVNtexture.h"
#include <prim/seadSafeString.h>

namespace agl
{
    class ShaderLocation;

    struct TextureSampler {
        TextureSampler();
        TextureSampler(agl::TextureData const&);
        void applyTextureData(agl::TextureData const&);
        bool activate(agl::DrawContext *,agl::ShaderLocation const&,int,bool) const;
    };
};