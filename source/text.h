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
    using MapType = std::map<std::string_view, TextMapEntry>;
    const MapType menu/*, equ*/;

    TextMap(MapType&& m /*, MapType&& e */) : menu(m)/*, equ(e)*/ { }

    static inline std::unique_ptr<TextMap> char_to_sprite;
    static void generate(C2D_SpriteSheet sprites);
};

#endif
