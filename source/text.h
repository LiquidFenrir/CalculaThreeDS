#ifndef INC_TEXT_H
#define INC_TEXT_H

#include <map>
#include <vector>
#include <memory>
#include <string_view>
#include <citro2d.h>

struct TextMapEntry {
    std::vector<C2D_Image> sprites;
    int width;

    TextMapEntry(std::vector<C2D_Image>&& s) : sprites(std::move(s)), width(0)
    {
        for(const auto& i : sprites)
        {
            width += i.subtex->width;
        }
    }
};

struct TextMap {
    std::map<std::string_view, TextMapEntry> menu;
    std::map<std::string_view, C2D_Image> equ;

    TextMap(decltype(menu)&& m, decltype(equ)&& e) : menu(std::move(m)), equ(std::move(e)) { }

    static inline std::unique_ptr<const TextMap> char_to_sprite;
    static void generate(C2D_SpriteSheet sprites);
};

#endif
