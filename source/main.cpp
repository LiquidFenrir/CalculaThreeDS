#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include "keyboard.h"

#include <cstdio>

u32 __stacksize__ = 1 * 1024 * 1024;

void Keyboard::print_info()
{
    printf("\x1b[6;1HRender result (w/y1/y2): %d / %d / %d\x1b[K", render_result.w, render_result.min_y, render_result.max_y);
    printf("\x1b[7;1HCursor at (x/y): %d / %d\x1b[K", render_result.cursor_x, render_result.cursor_y);
    printf("\x1b[8;1HScroll at (x/y): %d / %d\x1b[K", at_x, at_y);
}

int main(int argc, char** argv)
{
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    consoleInit(GFX_TOP, nullptr);
    // C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    consoleDebugInit(debugDevice_SVC);

    romfsInit();
    C2D_SpriteSheet sprites = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
    romfsExit();

    constexpr u32 clear_color = C2D_Color32(255,255,255,255);
    constexpr u32 fade_color = C2D_Color32(80,80,80,100);

    Keyboard kb(sprites);
    constexpr u32 CIRCLE_PAD_VALUES = (KEY_CPAD_UP | KEY_CPAD_DOWN | KEY_CPAD_LEFT | KEY_CPAD_RIGHT);

    while(aptMainLoop())
    {
        gspWaitForVBlank();

        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        
        if(kDown & KEY_START)
            break;

        const bool calculating = kb.calculating();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // C2D_TargetClear(top, clear_color);
        C2D_TargetClear(bottom, clear_color);

        if(!calculating)
        {
            kb.do_clears();
            kb.update_keyboard(sprites);
            kb.update_equation(sprites);
        }

        // C2D_SceneBegin(top);

        C2D_SceneBegin(bottom);

        kb.draw(sprites);

        if(calculating)
        {
            C2D_DrawRectSolid(0.0f, 0.0f, 0.875f, 320.0f, 240.0f, fade_color);
            kb.draw_loader();
        }

        C3D_FrameEnd(0);

        printf("\x1b[1;1HCalculator test");
        printf("\x1b[2;1HCPU:     %6.2f%%\x1b[K", C3D_GetProcessingTime()*6.0f);
        printf("\x1b[3;1HGPU:     %6.2f%%\x1b[K", C3D_GetDrawingTime()*6.0f);
        printf("\x1b[4;1HCmdBuf:  %6.2f%%\x1b[K", C3D_GetCmdBufUsage()*100.0f);

        kb.print_info();

        if(!calculating)
        {
            if(kDown & ~(KEY_TOUCH | CIRCLE_PAD_VALUES))
            {
                kb.handle_buttons(kDown);
            }
            else if(kDown & KEY_TOUCH)
            {
                touchPosition pos;
                hidTouchRead(&pos);
                kb.handle_touch(pos.px, pos.py);
            }
            else
            {
                circlePosition pos;
                hidCircleRead(&pos);
                if(abs(pos.dx) > 20 || abs(pos.dy) > 20) kb.handle_circle_pad(pos.dx, pos.dy);
            }
        }
    }

    C2D_SpriteSheetFree(sprites);

    C2D_Fini();
    C3D_Fini();
    gfxExit();
}
