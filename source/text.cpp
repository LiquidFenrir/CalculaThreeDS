#include "text.h"
#include "sprites.h"

void TextMap::generate(C2D_SpriteSheet sprites)
{
    MapType menu/*, equ*/;

    // menu

    { // Main menu
    menu.insert_or_assign("punctuation", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_add_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_sub_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_mul_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_div_idx),
    }));

    menu.insert_or_assign("functions", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_c_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_o_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_s_idx),
    }));

    menu.insert_or_assign("numbers", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_1_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_2_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_3_idx),
    }));

    menu.insert_or_assign("text", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_t_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_x_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_t_idx),
    }));
    }

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
    menu.insert_or_assign("(", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_lparen_idx),
    }));
    menu.insert_or_assign(")", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_rparen_idx),
    }));
    }

    { // functions
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
    menu.insert_or_assign("abs", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_abs_idx),
    }));
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
    menu.insert_or_assign("sqrt", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_root_idx),
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
    
    {
    menu.insert_or_assign("clip", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_c_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_l_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_p_idx),
    }));

    menu.insert_or_assign("delete", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_d_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_e_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_l_idx),
    }));

    menu.insert_or_assign("copy", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_c_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_p_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_y_idx),
    }));
    
    menu.insert_or_assign("cut", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_c_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_u_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_t_idx),
    }));

    menu.insert_or_assign("paste", TextMapEntry({
        C2D_SpriteSheetGetImage(sprites, sprites_p_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_s_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_t_idx),
    }));
    }

    char_to_sprite = std::make_unique<TextMap>(std::move(menu)/*, std::move(equ)*/);
}