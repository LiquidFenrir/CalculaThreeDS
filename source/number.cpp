#include "number.h"
#include "colors.h"
#include "text.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>

void Number::render(C2D_SpriteSheet sprites) const
{
    char floattextbuf[64];
    if(value.imag() == 0)
    {
        const double toprint = std::round(value.real() * 100000.0)/100000.0;
        std::snprintf(floattextbuf, 63, "%.12g", toprint + 0.0);
    }
    else
    {
        const double toprint_re = std::round(value.real() * 1000.0)/1000.0;
        const char sig = value.imag() < 0 ? '-' : '+';
        const double toprint_im = std::round(std::abs(value.imag()) * 1000.0)/1000.0;
        std::snprintf(floattextbuf, 63, "%.5g%ci%.5g", toprint_re + 0.0, sig, toprint_im + 0.0);
    }

    C2D_ImageTint text_tint;
    C2D_PlainImageTint(&text_tint, COLOR_BLACK, 1.0f);
    C2D_DrawRectSolid(0, EQU_REGION_HEIGHT, 0.875f, 400, 40, COLOR_GRAY);

    std::string_view sv(floattextbuf);
    const int w = sv.size() * 13;
    int x = 0;
    for(const char c : sv)
    {
        const auto img = TextMap::char_to_sprite->equ.at(std::string_view(&c, 1));
        C2D_DrawImageAt(img, 400 - w + x, EQU_REGION_HEIGHT + (40 - img.subtex->height)/2, 1.0f, &text_tint);
        x += 13;
    }
}

Number::Number(std::string_view in_val) : value(std::stod(std::string(in_val)))
{

}
Number::Number(const Value_t& in_val) : value(in_val)
{

}
Number::Number(const double in_val_re, const double in_val_im) : value(in_val_re, in_val_im)
{

}