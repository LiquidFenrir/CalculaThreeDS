#ifndef INC_EQUATION_H
#define INC_EQUATION_H

#include <memory>
#include <vector>
#include <map>
#include <array>
#include <string>
#include <citro2d.h>
#include "number.h"

class Tex {
    C3D_Tex t;
    C3D_RenderTarget* target;

public:
    Tex(u16 w, u16 h, GPU_TEXCOLOR mode)
    {
        C3D_TexInitVRAM(&t, w, h, mode);
        target = C3D_RenderTargetCreateFromTex(&t, GPU_TEXFACE_2D, 0, -1);
    }

    Tex(const Tex&) = delete;
    Tex(Tex&&) = delete;
    Tex& operator=(const Tex&) = delete;
    Tex& operator=(Tex&&) = delete;

    C3D_Tex* get_tex()
    {
        return &t;
    }
    C3D_RenderTarget* get_target()
    {
        return target;
    }

    ~Tex()
    {
        C3D_RenderTargetDelete(target);
        C3D_TexDelete(&t);
    }
};

struct Part {
    enum class Specialty : unsigned short {
        None = 0,
        Equation = BIT(0),
        Fraction = BIT(1),
        Exponent = BIT(2),
        Absolute = BIT(3),
        Conjugate = BIT(4),
        Root = BIT(5),
        Paren = BIT(6),
        TempParen = BIT(7),
    };
    enum class Position : unsigned char {
        None = 0,
        Start = BIT(0),
        End = BIT(1),
        Middle = Start | End,
    };

    struct Meta {
        int next = -1;
        int before = -1;
        int assoc = -1;
        Specialty special = Specialty::None;
        Position position = Position::None;
    };
    Meta meta;
    std::string value{};
};

inline Part::Position operator&(Part::Position l, Part::Position r)
{
    return static_cast<Part::Position>(
        static_cast<unsigned char>(l)
        &
        static_cast<unsigned char>(r)
    );
}
inline Part::Position operator!(Part::Position v)
{
    if(v == Part::Position::Start)
    {
        return Part::Position::End;
    }
    else if(v == Part::Position::End)
    {
        return Part::Position::Start;
    }
    return Part::Position::None;
}
inline bool check_pos_is(Part::Position v, Part::Position other)
{
    if(other == Part::Position::None)
    {
        return v == Part::Position::None;
    }
    else
    {
        return (v & other) == other;
    }
}

struct PartPos {
    int part{-1}, pos{-1};
};

struct Equation {
    struct RenderResult {
        int w, min_y, max_y;
        int cursor_x, cursor_y;
        bool cursor_visible;
    };

    RenderResult render_main(const int x, const int y, const int editing_part, const int editing_char, C2D_SpriteSheet sprites, PartPos* screen);
    RenderResult render_memory(const int x, const int y, C2D_SpriteSheet sprites);
    std::vector<Part> parts;

    Equation();
    void optimize();
    std::pair<Number, bool> calculate(std::map<std::string, Number>& variables, int& error_part, int& error_position);

    int set_special(const int current_part_id, const int at_position, const Part::Specialty special);
    void find_matching_tmp_paren(const int original_pos);
    std::pair<bool, bool> add_part_at(int& current_part_id, int& at_position, const Part::Specialty special = Part::Specialty::None, const Part::Position position = Part::Position::None, const int assoc = -1);
    bool remove_at(int& current_part_id, int& at_position);
    bool left_of(int& current_part_id, int& at_position);
    bool right_of(int& current_part_id, int& at_position);
};

#endif
