#include "text.h"
#include "sprites.h"

void TextMap::generate(C2D_SpriteSheet sprites)
{
    std::map<std::string_view, TextMapEntry> menu;
    std::map<std::string_view, C2D_Image> equ;

    { // punctuation
    menu.insert_or_assign("+", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_add_idx),
    }));
    menu.insert_or_assign("-", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_sub_idx),
    }));
    menu.insert_or_assign("*", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_mul_idx),
    }));
    menu.insert_or_assign("/", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_div_idx),
    }));
    menu.insert_or_assign(".", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_decimals_idx),
    }));
    menu.insert_or_assign("^", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_pow_idx),
    }));
    menu.insert_or_assign("=", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_equals_idx),
    }));
    menu.insert_or_assign("(", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_lparen_idx),
    }));
    menu.insert_or_assign(")", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_rparen_idx),
    }));
    }

    { // functions
    // exponential
    menu.insert_or_assign("exp", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_exp_idx),
    }));
    menu.insert_or_assign("ln", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_l_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_n_idx),
    }));
    menu.insert_or_assign("log", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_l_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_o_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_g_idx),
    }));
    // trigonometric
    menu.insert_or_assign("acos", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_a_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_c_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_o_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_s_idx),
    }));
    menu.insert_or_assign("asin", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_a_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_s_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_i_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_n_idx),
    }));
    menu.insert_or_assign("atan", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_a_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_t_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_a_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_n_idx),
    }));
    menu.insert_or_assign("cos", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_c_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_o_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_s_idx),
    }));
    menu.insert_or_assign("sin", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_s_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_i_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_n_idx),
    }));
    menu.insert_or_assign("tan", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_t_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_a_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_n_idx),
    }));
    // hyperbolic
    menu.insert_or_assign("acosh", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_a_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_c_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_o_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_s_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_h_idx),
    }));
    menu.insert_or_assign("asinh", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_a_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_s_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_i_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_n_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_h_idx),
    }));
    menu.insert_or_assign("atanh", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_a_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_t_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_a_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_n_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_h_idx),
    }));
    menu.insert_or_assign("cosh", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_c_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_o_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_s_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_h_idx),
    }));
    menu.insert_or_assign("sinh", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_s_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_i_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_n_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_h_idx),
    }));
    menu.insert_or_assign("tanh", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_t_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_a_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_n_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_h_idx),
    }));
    // other
    menu.insert_or_assign("abs", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_abs_idx),
    }));
    menu.insert_or_assign("sqrt", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_root_idx),
    }));
    menu.insert_or_assign("conj", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_conj_idx),
    }));
    }

    { // numbers
    menu.insert_or_assign("0", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_0_idx),
    }));
    menu.insert_or_assign("1", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_1_idx),
    }));
    menu.insert_or_assign("2", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_2_idx),
    }));
    menu.insert_or_assign("3", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_3_idx),
    }));
    menu.insert_or_assign("4", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_4_idx),
    }));
    menu.insert_or_assign("5", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_5_idx),
    }));
    menu.insert_or_assign("6", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_6_idx),
    }));
    menu.insert_or_assign("7", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_7_idx),
    }));
    menu.insert_or_assign("8", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_8_idx),
    }));
    menu.insert_or_assign("9", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_9_idx),
    }));
    menu.insert_or_assign("pi", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_pi_idx),
    }));
    menu.insert_or_assign("ans", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_a_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_n_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_s_idx),
    }));
    }

    menu.insert_or_assign("del", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_delete_idx),
    }));
    menu.insert_or_assign(">", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_assign_arrow_idx),
    }));

    static char arr_dig[2 * 10] = {0};
    for(int i = 0; i < 10; ++i)
    {
        char* p = &arr_dig[i * 2];
        p[0] = '0' + i;
        equ.insert_or_assign(p, C2D_SpriteSheetGetImage(sprites, sprites_0_idx + i));
        menu.insert_or_assign(p, TextMapEntry({
            C2D_SpriteSheetGetImage(sprites, sprites_0_idx + i),
        }));
    }

    static char arr_let[2 * 26] = {0};
    for(int i = 0; i < 26; ++i)
    {
        char* p = &arr_let[i * 2];
        p[0] = 'a' + i;
        equ.insert_or_assign(p, C2D_SpriteSheetGetImage(sprites, sprites_a_idx + i));
        menu.insert_or_assign(p, TextMapEntry({
            C2D_SpriteSheetGetImage(sprites, sprites_a_idx + i),
        }));
    }

    equ.insert_or_assign("P", C2D_SpriteSheetGetImage(sprites, sprites_pi_idx));
    equ.insert_or_assign("+", C2D_SpriteSheetGetImage(sprites, sprites_add_idx));
    equ.insert_or_assign("-", C2D_SpriteSheetGetImage(sprites, sprites_sub_idx));
    equ.insert_or_assign("*", C2D_SpriteSheetGetImage(sprites, sprites_mul_idx));
    equ.insert_or_assign(".", C2D_SpriteSheetGetImage(sprites, sprites_decimals_idx));

    equ.insert_or_assign(">", C2D_SpriteSheetGetImage(sprites, sprites_assign_arrow_idx));

    char_to_sprite = std::make_unique<TextMap>(std::move(menu), std::move(equ));
}
