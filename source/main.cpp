#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include "keyboard.h"
#include "colors.h"

#include <cstdio>

u32 __stacksize__ = 512 * 1024;

static void termhandler()
{
    svcBreak(USERBREAK_PANIC);
}

int main(int argc, char** argv)
{
    std::set_terminate(termhandler);
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    u32 old_time_limit;
    APT_GetAppCpuTimeLimit(&old_time_limit);
    APT_SetAppCpuTimeLimit(30);

    hidSetRepeatParameters(15, 8);

    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    consoleDebugInit(debugDevice_SVC);

    romfsInit();
    C2D_SpriteSheet sprites = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
    romfsExit();

    Keyboard kb(sprites);
    constexpr u32 CIRCLE_PAD_VALUES = (KEY_CPAD_UP | KEY_CPAD_DOWN | KEY_CPAD_LEFT | KEY_CPAD_RIGHT);

    while(aptMainLoop())
    {
        gspWaitForVBlank();

        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kDownRepeat = hidKeysDownRepeat();
        u32 kHeld = hidKeysHeld();
        
        if(kDown & KEY_START)
            break;

        const bool calculating = kb.calculating();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C2D_TargetClear(top, COLOR_BLACK);
        C2D_TargetClear(bottom, COLOR_WHITE);

        if(!calculating)
        {
            kb.do_clears();
            kb.update_memory(sprites);
            kb.update_equation(sprites);
        }

        C2D_SceneBegin(top);

        kb.draw_memory(sprites);
        
        if(calculating)
        {
            C2D_DrawRectSolid(0, 0, 1.0f, 400, 240, COLOR_FADE);
        }

        C2D_SceneBegin(bottom);

        kb.draw(sprites);

        if(calculating)
        {
            C2D_DrawRectSolid(0, 0, 0.875f, 320, 240, COLOR_FADE);
            kb.draw_loader();
        }

        C3D_FrameEnd(0);

        if(!calculating)
        {
            if((kDownRepeat | kDown) & ~(KEY_TOUCH | CIRCLE_PAD_VALUES))
            {
                kb.handle_buttons(kDown, kDownRepeat);
            }
            else if(kDownRepeat & KEY_TOUCH)
            {
                touchPosition pos;
                hidTouchRead(&pos);
                kb.handle_touch(pos.px, pos.py);
            }
            else if(kHeld & CIRCLE_PAD_VALUES)
            {
                circlePosition pos;
                hidCircleRead(&pos);
                if(abs(pos.dx) > 20 || abs(pos.dy) > 20) kb.handle_circle_pad(pos.dx, pos.dy);
            }
        }
    }

    C2D_SpriteSheetFree(sprites);

    if (old_time_limit != UINT32_MAX) APT_SetAppCpuTimeLimit(old_time_limit);

    C2D_Fini();
    C3D_Fini();
    gfxExit();
}
