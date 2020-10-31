#ifndef INC_EQUATION_H
#define INC_EQUATION_H

#include <memory>
#include <vector>
#include <array>
#include <string>
#include <citro2d.h>


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
    enum class Specialty : unsigned char {
        None = 0,
        Equation = 1,
        Fraction = 2,
        Exponent = 4,
        Paren = 8,
    };
    enum class Position : unsigned char {
        None = 0,
        Start = 1,
        End = 2,
        Middle = Start | End,
    };

    struct Meta {
        int next = -1;
        int before = -1;
        int assoc = -1;
        Specialty special = Specialty::None;
        Position position = Position::None;
        bool tmp = false;
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

inline constexpr int EQU_REGION_HEIGHT = 72;
struct PartPos {
    int part{-1}, pos{-1};
};
struct Equation {
    struct RenderResult {
        int w, min_y, max_y;
        int cursor_x, cursor_y;
        bool cursor_visible;
        std::array<PartPos, 320 * EQU_REGION_HEIGHT> screen;
    };
    RenderResult render(const int x, const int y, const int editing_part, const int editing_char, C2D_SpriteSheet sprites);
    std::vector<Part> parts;

    static inline PartPos clipboard_start = {-1, -1}, clipboard_end = {-1, -1};
    static inline std::vector<Part> clipboard;

    Equation();
    void optimize();
    void find_matching_paren(const int paren_pos);
    std::pair<bool, bool> add_part_at(int& current_part_id, int& at_position, const Part::Specialty special = Part::Specialty::None, const Part::Position position = Part::Position::None, const int assoc = -1);
    bool remove_at(int& current_part_id, int& at_position);
    bool left_of(int& current_part_id, int& at_position);
    bool right_of(int& current_part_id, int& at_position);
};

#endif
