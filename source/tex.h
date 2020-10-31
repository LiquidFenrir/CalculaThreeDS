#pragma once

#include <citro3d.h>

#include <memory>

struct TargetCloser {
    void operator()(C3D_RenderTarget* t)
    {
        C3D_RenderTargetDelete(t);
    }
};
using TargetPtr = std::unique_ptr<C3D_RenderTarget, TargetCloser>;

struct Tex {
    C3D_Tex tex;
    TargetPtr target;
    bool inited = false;
    void create(u16 w, u16 h)
    {
        if(inited) return;

        C3D_TexInitVRAM(&tex, w, h, GPU_RGBA8);
        target.reset(C3D_RenderTargetCreateFromTex(&tex, GPU_TEXFACE_2D, 0, -1));
        inited = true;
    }
    void clear()
    {
        if(!inited) return;

        C3D_TexDelete(&tex);
        target.reset(nullptr);
        inited = false;
    }
    ~Tex()
    {
        clear();
    }
};
