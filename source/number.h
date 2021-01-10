#ifndef INC_NUMBER_H
#define INC_NUMBER_H

#include <complex>
#include <string_view>
#include <citro2d.h>

static inline constexpr int EQU_REGION_HEIGHT = 80;

struct Number {
    using Value_t = std::complex<double>;
    Value_t value;

    void render(C2D_SpriteSheet sprites) const;

    Number() { }
    Number(std::string_view in_val);
    Number(const Value_t& in_val);
    Number(const double in_val_re, const double in_val_im = 0.0);
};

#endif
