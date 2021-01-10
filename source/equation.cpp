#include "equation.h"
#include "sprites.h"
#include "text.h"
#include "colors.h"
#include <algorithm>
#include <cmath>

static const Number E_VAL(std::exp(1.0));
static const Number PI_VAL(M_PI);
static const Number I_VAL(0.0, 1.0);

Equation::Equation() : parts(3)
{
    parts[0].meta.assoc = 2;
    parts[0].meta.special = Part::Specialty::Equation;
    parts[0].meta.position = Part::Position::Start;
    parts[0].meta.next = 1;

    parts[2].meta.assoc = 0;
    parts[2].meta.special = Part::Specialty::Equation;
    parts[2].meta.position = Part::Position::End;
    parts[2].meta.before = 1;

    parts[1].meta.before = 0;
    parts[1].meta.next = 2;
}

struct RenderInfo {
    int current_x;
    const int min_x, max_x, center_y;
    const int height;
    const int editing_part;
    const int editing_char;

    bool can_draw(const int vertical_offset) const
    {
        const bool x_correct = (min_x - (13 -1)) <= current_x && current_x < max_x;
        const bool y_correct = -(height/2 + 24/2) < (vertical_offset - center_y) && (vertical_offset - center_y) <= (height/2 + 24);
        return x_correct && y_correct;
    }
    int get_x() const
    {
        return current_x - min_x;
    }
    int get_y(const int vertical_offset) const
    {
        return center_y - vertical_offset + height/2 - 24/2;
    }
};

struct RenderPart {
    int data{};
    // in deviation from the previous, counted in half parts
    int middle_change{};
    // in number of half parts, from the middle
    int y_start{}; // positive, up
    int y_end{}; // negative, down
    // only used for special parts
    int associated{};
    int content_width{};
    int content_middle_high{};
    int paren_y_start{};
    int paren_y_end{};
};
static void find_part_sizes(const std::vector<Part>& parts, std::vector<RenderPart>& part_sizes)
{
    static std::vector<int> id_stack;

    const auto pop_id = []() -> int {
        const int i = id_stack.back();
        id_stack.pop_back();
        return i;
    };

    enum CareAbout : int {
        CA_Width = 1,
        CA_HeightBelow = 2,
        CA_HeightAbove = 4,
        CA_All = 1 | 2 | 4,
    };
    const auto add_content_info = [&](const RenderPart& ps, const CareAbout care = CA_All) -> void {
        if(!id_stack.empty())
        {
            auto& container_size = part_sizes[id_stack.back()];
            if(care & CA_Width)
            {
                container_size.content_width += ps.content_width;
            }

            if(care & (CA_HeightAbove | CA_HeightBelow))
            {
                if(care & CA_HeightAbove)
                {
                    container_size.y_start = std::max(container_size.y_start, ps.y_start + ps.middle_change);
                    container_size.paren_y_start = std::max(container_size.paren_y_start, container_size.y_start);
                    container_size.paren_y_start = std::max(container_size.paren_y_start, ps.paren_y_start);
                }

                if(care & CA_HeightBelow)
                {
                    container_size.y_end = std::min(container_size.y_end, ps.y_end + ps.middle_change);
                    container_size.paren_y_end = std::min(container_size.paren_y_end, container_size.y_end);
                    container_size.paren_y_end = std::min(container_size.paren_y_end, ps.paren_y_end);
                }

                container_size.content_middle_high = std::max(container_size.content_middle_high, ps.middle_change + ps.content_middle_high);
            }
        }
    };
    const auto do_part_basic = [&](const int idx) -> void {
        const auto& p = parts[idx];
        auto& ps = part_sizes[idx];

        ps.data = [&]() -> int {;
            if(p.meta.special == Part::Specialty::TempParen)
            {
                return 1;
            }
            else if(p.value.empty())
            {
                const auto& prev_meta = parts[p.meta.before].meta;
                const auto& next_meta = parts[p.meta.next].meta;
                const bool sandwiched = (check_pos_is(prev_meta.position, Part::Position::Start) && check_pos_is(next_meta.position, Part::Position::End)) && prev_meta.special == next_meta.special;
                if(
                    (sandwiched && (
                        prev_meta.special == Part::Specialty::Equation ||
                        prev_meta.special == Part::Specialty::Paren ||
                        prev_meta.special == Part::Specialty::Absolute
                    ))
                    ||
                    (!sandwiched && (prev_meta.special == Part::Specialty::TempParen
                        || next_meta.special != Part::Specialty::Exponent
                        || (next_meta.special == Part::Specialty::Exponent && next_meta.position != Part::Position::Start)
                        || (next_meta.special == Part::Specialty::Exponent && prev_meta.special == Part::Specialty::Paren && prev_meta.position == Part::Position::End)
                    ))
                )
                {
                    return 0;
                }
                return 1;
            }
            else
            {
                return p.value.size();
            }
        }();

        ps.content_width = ps.data;
        ps.y_start = 1;
        ps.y_end = -1;
        ps.paren_y_start = 1;
        ps.paren_y_end = -1;

        add_content_info(ps);
    };

    int current_idx = 0;

    while(current_idx != -1)
    {
        const auto& part = parts[current_idx];
        if(part.meta.special == Part::Specialty::None || part.meta.special == Part::Specialty::TempParen)
        {
            do_part_basic(current_idx);
        }
        else if(part.meta.special != Part::Specialty::Equation)
        {
            if(check_pos_is(part.meta.position, Part::Position::End))
            {
                const int associated_id = pop_id();
                const auto& ap = parts[associated_id];
                auto& aps = part_sizes[associated_id];
                auto& ps = part_sizes[current_idx];

                if(part.meta.special == Part::Specialty::Exponent)
                {
                    const int delta = (aps.y_start - aps.content_middle_high) - aps.y_end;

                    aps.middle_change = delta;
                    ps.middle_change = -delta;

                    aps.associated = current_idx;
                    ps.associated = associated_id;
                    add_content_info(aps, CA_All);
                }
                else if(part.meta.special == Part::Specialty::Fraction)
                {
                    if(check_pos_is(ap.meta.position, Part::Position::End)) // ending bottom part
                    {
                        ps.middle_change = aps.y_start + 1;

                        aps.middle_change += -ps.middle_change; // middle
                        aps.associated = current_idx; // middle.associated points to end
                        ps.associated = associated_id; // end.associated points to middle

                        RenderPart cp = aps;
                        cp.content_width = std::max(aps.content_width, aps.data);
                        cp.middle_change = -ps.middle_change;
                        add_content_info(cp, CareAbout(CA_HeightBelow | CA_Width));
                    }
                    else // ending top part
                    {
                        aps.middle_change = 2 + (-aps.y_end - 1); // start

                        ps.middle_change = -aps.middle_change;
                        aps.associated = current_idx; // start.associated points to middle
                        ps.data = aps.content_width; // middle.data is start.content_width

                        add_content_info(aps, CA_HeightAbove);
                    }
                }
                else if(part.meta.special == Part::Specialty::Paren || part.meta.special == Part::Specialty::Absolute)
                {
                    aps.associated = current_idx;
                    ps.associated = associated_id;

                    ps.paren_y_start = aps.paren_y_start;
                    ps.paren_y_end = aps.paren_y_end;

                    aps.content_width += 2;
                    add_content_info(aps, CA_All);
                }
                else if(part.meta.special == Part::Specialty::Root)
                {
                    aps.associated = current_idx;
                    ps.associated = associated_id;

                    ps.paren_y_start = aps.paren_y_start + 1;
                    ps.paren_y_end = aps.paren_y_end;

                    aps.content_width += 2;
                    add_content_info(aps, CA_All);
                }
                else if(part.meta.special == Part::Specialty::Conjugate)
                {
                    aps.associated = current_idx;
                    ps.associated = associated_id;

                    aps.y_start += 1;
                    ps.y_start = aps.y_start;

                    aps.paren_y_start = aps.y_start;
                    ps.paren_y_start = aps.paren_y_start;
                    ps.paren_y_end = aps.paren_y_end;

                    add_content_info(aps, CA_All);
                }
            }

            if(check_pos_is(part.meta.position, Part::Position::Start))
            {
                id_stack.push_back(current_idx);
            }
        }
        current_idx = part.meta.next;
    }

    id_stack.clear();
}
static void render_parts(const std::vector<Part>& parts, RenderInfo& info, Equation::RenderResult& out, PartPos* screen, C2D_SpriteSheet sprites)
{
    static std::vector<RenderPart> part_sizes;
    part_sizes.resize(parts.size());
    find_part_sizes(parts, part_sizes);

    int selected_multi_id = -1;
    int selected_multi_assoc = -1;
    if(info.editing_char == 0)
    {
        if(const auto& part_before = parts[parts[info.editing_part].meta.before]; part_before.meta.special == Part::Specialty::Paren || part_before.meta.special == Part::Specialty::Absolute)
        {
            selected_multi_id = parts[info.editing_part].meta.before;
            selected_multi_assoc = part_before.meta.assoc;
        }
    }

    int current_idx = 0;

    C2D_Image empty_img = C2D_SpriteSheetGetImage(sprites, sprites_empty_but_clickable_idx);

    C2D_Image lpa_sprites[] = {
        C2D_SpriteSheetGetImage(sprites, sprites_lparen_begin_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_lparen_middle_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_lparen_end_idx),
    };
    
    C2D_Image rpa_sprites[] = {
        C2D_SpriteSheetGetImage(sprites, sprites_rparen_begin_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_rparen_middle_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_rparen_end_idx),
    };

    C2D_Image abs_sprites[] = {
        C2D_SpriteSheetGetImage(sprites, sprites_abs_begin_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_abs_middle_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_abs_end_idx),
    };

    C2D_Image sqrt_sprites[] = {
        C2D_SpriteSheetGetImage(sprites, sprites_sqrt_begin_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_sqrt_middle_idx),
        C2D_SpriteSheetGetImage(sprites, sprites_sqrt_end_idx),
    };

    C2D_ImageTint text_tint;
    C2D_PlainImageTint(&text_tint, COLOR_BLACK, 1.0f);
    C2D_ImageTint temp_tint;
    C2D_PlainImageTint(&temp_tint, COLOR_GRAY, 1.0f);
    C2D_ImageTint paren_selected_tint;
    C2D_PlainImageTint(&paren_selected_tint, COLOR_BLUE, 1.0f);

    const auto draw_paren = [&](const C2D_Image* sprites, const int vertical_offset, const int y_start, const int y_end, const C2D_ImageTint* tnt) -> void {
        const int span = (y_start - y_end) - 2;
        const int span_pixels = (span * 24 / 4) + 1;
        const int pixel_y = info.get_y(vertical_offset) - ((y_start - 1) * 24 / 4);
        const int x = info.get_x();
        C2D_DrawImageAt(sprites[0], x, pixel_y, 0.0f, tnt);
        const int middle_y = pixel_y + sprites[0].subtex->height;
        C2D_DrawImageAt(sprites[1], x, middle_y, 0.0f, tnt, 1.0f, span_pixels);
        const int bottom_y = middle_y + span_pixels;
        C2D_DrawImageAt(sprites[2], x, bottom_y, 0.0f, tnt);
    };
    const auto set_cursor = [&](const int y) -> void {
        out.cursor_x = info.get_x();
        out.cursor_y = y;
        if(!(out.cursor_x <= -2 || out.cursor_x >= (320 - 2) || out.cursor_y <= (-24) || out.cursor_y >= EQU_REGION_HEIGHT))
        {
            out.cursor_visible = true;
        }
    };

    int vertical_offset = 0;
    int min_vert = 0;
    int max_vert = 0;
    PartPos pos;
    while(current_idx != -1)
    {
        const auto& part = parts[current_idx];
        const auto& part_size = part_sizes[current_idx];
        if(part.meta.special == Part::Specialty::None)
        {
            pos.part = current_idx;
            if(part.value.empty())
            {
                if(part_size.data != 0 && info.can_draw(vertical_offset))
                {
                    const auto& img = empty_img;
                    C2D_DrawImageAt(img, info.get_x(), info.get_y(vertical_offset), 0.0f, &text_tint);

                    if(screen)
                    {
                        pos.pos = 0;
                        for(int y = 0; y < 24; y++)
                        {
                            const int actual_y = info.get_y(vertical_offset) + y;
                            if(actual_y >= 0 && actual_y < EQU_REGION_HEIGHT)
                            {
                                const int y_part = actual_y * 320;
                                for(int x = 0; x < 13; x++)
                                {
                                    const int actual_x = info.get_x() + x;
                                    if(actual_x >= 0 && actual_x < 320)
                                    {
                                        screen[actual_x + y_part] = pos;
                                    }
                                }
                            }
                        }
                    }
                }

                if(info.editing_part == current_idx && info.editing_char == 0)
                {
                    set_cursor(info.get_y(vertical_offset));
                }

                if(part_size.data != 0) info.current_x += 13;
            }
            else
            {
                int char_idx = 0;
                for(const char c : part.value)
                {
                    if(info.can_draw(vertical_offset))
                    {
                        const auto img = TextMap::char_to_sprite->equ.at(std::string_view(&c, 1));
                        C2D_DrawImageAt(img, info.get_x(), info.get_y(vertical_offset), 0.0f, &text_tint);
                        if(screen)
                        {
                            pos.pos = char_idx + 1;
                            for(int y = 0; y < 24; y++)
                            {
                                const int actual_y = info.get_y(vertical_offset) + y;
                                if(actual_y >= 0 && actual_y < EQU_REGION_HEIGHT)
                                {
                                    const int y_part = actual_y * 320;
                                    for(int x = 0; x < 13; x++)
                                    {
                                        const int actual_x = info.get_x() + x;
                                        if(actual_x >= 0 && actual_x < 320)
                                        {
                                            screen[actual_x + y_part] = pos;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if(info.editing_part == current_idx && info.editing_char == char_idx)
                    {
                        set_cursor(info.get_y(vertical_offset));
                    }

                    info.current_x += 13;;
                    char_idx += 1;
                }

                if(info.editing_part == current_idx && info.editing_char == char_idx)
                {
                    set_cursor(info.get_y(vertical_offset));
                }
            }
        }
        else if(part.meta.special != Part::Specialty::Equation)
        {
            if(part.meta.special == Part::Specialty::Paren || part.meta.special == Part::Specialty::TempParen || part.meta.special == Part::Specialty::Absolute)
            {
                pos.part = part.meta.next;
                const C2D_Image* sprs = nullptr;
                if(part.meta.special == Part::Specialty::Absolute)
                {
                    sprs = abs_sprites;
                }
                else if(part.meta.position == Part::Position::Start)
                {
                    sprs = lpa_sprites;
                }
                else if(part.meta.position == Part::Position::End)
                {
                    sprs = rpa_sprites;
                }

                const C2D_ImageTint* tnt = &temp_tint;
                if(part.meta.special == Part::Specialty::Paren || part.meta.special == Part::Specialty::Absolute)
                {
                    if(current_idx == selected_multi_id || current_idx == selected_multi_assoc)
                    {
                        tnt = &paren_selected_tint;
                    }
                    else
                    {
                        tnt = &text_tint;
                    }
                }
                draw_paren(sprs, vertical_offset, part_size.paren_y_start, part_size.paren_y_end, tnt);

                if(screen)
                {
                    const int h = ((part_size.paren_y_start - part_size.paren_y_end) * 24 / 2) + 1;
                    for(int y = 0; y < h; y++)
                    {
                        const int actual_y = info.get_y(vertical_offset) - ((part_size.paren_y_start - 1) * 24 / 2) + y;
                        if(actual_y >= 0 && actual_y < EQU_REGION_HEIGHT)
                        {
                            const int y_part = actual_y * 320;
                            for(int x = 0; x < 13; x++)
                            {
                                const int actual_x = info.get_x() + x;
                                if(actual_x >= 0 && actual_x < 320)
                                {
                                    screen[actual_x + y_part] = pos;
                                }
                            }
                        }
                    }
                }

                info.current_x += 13;
            }
            else if(part.meta.special == Part::Specialty::Root)
            {
                if(part.meta.position == Part::Position::Start)
                {
                    draw_paren(sqrt_sprites, vertical_offset, part_size.paren_y_start, part_size.paren_y_end, &text_tint);
                    const int pixel_y = info.get_y(vertical_offset) - ((part_size.paren_y_start - 1) * 24 / 4);
                    const int bar_x = info.get_x() + 10;
                    const int bar_w = (part_size.content_width - 2) * 13 + 10;
                    const int notch_x = bar_x + bar_w;
                    C2D_DrawRectSolid(bar_x, pixel_y, 0.125f, bar_w, 2, COLOR_BLACK);
                    C2D_DrawRectSolid(notch_x, pixel_y, 0.125f, 2, 10, COLOR_BLACK);
                }
                info.current_x += 13;
            }
            else if(part.meta.special == Part::Specialty::Conjugate)
            {
                if(part.meta.position == Part::Position::Start)
                {
                    C2D_DrawRectSolid(info.get_x(), info.get_y(vertical_offset) - ((part_size.paren_y_start - 1) * 24 / 4), 0.125f, part_size.content_width * 13, 2, COLOR_BLACK);
                }
            }
            else if(part.meta.special == Part::Specialty::Fraction)
            {
                if(part.meta.position == Part::Position::Start)
                {
                    const auto& middle_size = part_sizes[part_size.associated];
                    const int max_width = std::max(middle_size.content_width, part_size.content_width);
                    C2D_DrawRectSolid(info.get_x(), info.get_y(vertical_offset) + 24 / 2 - 1, 0.0f, max_width * 13, 2.0f, COLOR_BLACK);
                    info.current_x += ((max_width - part_size.content_width) * 13)/2; // top align
                }
                else if(part.meta.position == Part::Position::Middle)
                {
                    const int max_width = std::max(part_size.content_width, part_size.data);
                    info.current_x -= (part_size.data * 13); // top size
                    info.current_x -= ((max_width - part_size.data) * 13)/2; // top align
                    info.current_x += ((max_width - part_size.content_width) * 13)/2; // bottom align
                }
                else if(part.meta.position == Part::Position::End)
                {
                    const auto& middle_size = part_sizes[part_size.associated];
                    const int max_width = std::max(middle_size.content_width, middle_size.data);
                    info.current_x -= (middle_size.content_width * 13); // bottom size
                    info.current_x -= ((max_width - middle_size.content_width) * 13)/2; // bottom align
                    info.current_x += max_width * 13; // full size
                }
            }

            vertical_offset += (part_size.middle_change * 24) / 4;
            max_vert = std::max(max_vert, vertical_offset);
            min_vert = std::min(min_vert, vertical_offset);
        }
        current_idx = part.meta.next;
    }

    out.w = info.current_x;
    out.min_y = min_vert - 24 / 2;
    out.max_y = max_vert + 24 / 2;
    part_sizes.clear();
}

Equation::RenderResult Equation::render_main(const int x, const int y, const int editing_part, const int editing_char, C2D_SpriteSheet sprites, PartPos* screen)
{
    RenderResult out{0,0,0,-1,-1, false};
    RenderInfo info{
        0,
        x, x + 320, y,
        EQU_REGION_HEIGHT,
        editing_part,
        editing_char,
    };
    render_parts(parts, info, out, screen, sprites);
    return out;
}

Equation::RenderResult Equation::render_memory(const int x, const int y, C2D_SpriteSheet sprites)
{
    RenderResult out{0,0,0,-1,-1, false};
    RenderInfo info{
        0,
        x, x + 400, y, 
        EQU_REGION_HEIGHT,
        -1,
        -1,
    };
    render_parts(parts, info, out, nullptr, sprites);
    return out;
}

void Equation::optimize()
{
    std::vector<Part> tmp;
    tmp.push_back(std::move(parts[0]));
    int prev_id = 0;
    int next_id_parts = tmp.back().meta.next;
    while(next_id_parts != -1)
    {
        int next_id_tmp = tmp.size();
        tmp.back().meta.next = next_id_tmp;
        tmp.push_back(std::move(parts[next_id_parts]));
        next_id_parts = tmp.back().meta.next;
        tmp.back().meta.before = prev_id;
        prev_id = next_id_tmp;
    }
    parts = std::move(tmp);
}

std::pair<Number, bool> Equation::calculate(std::map<std::string, Number>& variables, int& error_part, int& error_position)
{
    #define ERROR_AT(part, pos) { error_part = part; error_position = pos; return {}; }

    struct Token {
        enum class Type {
            Number,
            Variable,
            Function,
            Operator,
            ParenOpen,
            ParenClose,
        };

        std::string_view value;
        int part, position;
        Type type;
    };
    const auto base_tokens = [&, this]() -> std::vector<Token> {
        std::vector<Token> toks;
        int start = 0;
        int len = 0;
        int tmp_is_num = -1;

        int current_idx = 0;
        while(current_idx != -1)
        {
            const auto& p = parts[current_idx];
            if(p.meta.special != Part::Specialty::None)
            {
                if(p.meta.special == Part::Specialty::TempParen)
                {
                    ERROR_AT(p.meta.next, 0);
                }
                else if(p.meta.special == Part::Specialty::Absolute)
                {
                    if(p.meta.position == Part::Position::Start)
                    {
                        toks.push_back(Token{"abs", p.meta.next, 0, Token::Type::Function});
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenOpen});
                    }
                    else
                    {
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenClose});
                    }
                }
                else if(p.meta.special == Part::Specialty::Root)
                {
                    if(p.meta.position == Part::Position::Start)
                    {
                        if(toks.size() && (toks.back().type == Token::Type::Variable || toks.back().type == Token::Type::Number))
                        {
                            toks.push_back(Token{"*", toks.back().part, toks.back().position, Token::Type::Operator});
                        }
                        toks.push_back(Token{"sqrt", p.meta.next, 0, Token::Type::Function});
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenOpen});
                    }
                    else
                    {
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenClose});
                    }
                }
                else if(p.meta.special == Part::Specialty::Conjugate)
                {
                    if(p.meta.position == Part::Position::Start)
                    {
                        toks.push_back(Token{"conj", p.meta.next, 0, Token::Type::Function});
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenOpen});
                    }
                    else
                    {
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenClose});
                    }
                }
                else if(p.meta.special == Part::Specialty::Paren)
                {
                    if(p.meta.position == Part::Position::Start)
                    {
                        if(toks.size() && (toks.back().type == Token::Type::Variable || toks.back().type == Token::Type::Number))
                        {
                            toks.push_back(Token{"*", toks.back().part, toks.back().position, Token::Type::Operator});
                        }
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenOpen});
                    }
                    else
                    {
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenClose});
                    }
                }
                else if(p.meta.special == Part::Specialty::Exponent)
                {
                    if(p.meta.position == Part::Position::Start)
                    {
                        if(toks.size() >= 1)
                        {
                            if(const auto b = toks.back(); b.value == "e" && b.type == Token::Type::Variable)
                            {
                                toks.pop_back();
                                if(toks.size() && (toks.back().type == Token::Type::Variable || toks.back().type == Token::Type::Number))
                                {
                                    toks.push_back(Token{"*", toks.back().part, toks.back().position, Token::Type::Operator});
                                }
                                toks.push_back(Token{"exp", b.part, b.position, Token::Type::Function});
                            }
                            else
                                toks.push_back(Token{"^", p.meta.next, 0, Token::Type::Operator});
                        }
                        else
                            toks.push_back(Token{"^", p.meta.next, 0, Token::Type::Operator});
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenOpen});
                    }
                    else
                    {
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenClose});
                    }
                }
                else if(p.meta.special == Part::Specialty::Fraction)
                {
                    if(p.meta.position == Part::Position::Start)
                    {
                        if(toks.size() && (toks.back().type == Token::Type::Variable || toks.back().type == Token::Type::Number))
                        {
                            toks.push_back(Token{"*", toks.back().part, toks.back().position, Token::Type::Operator});
                        }
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenOpen});
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenOpen});
                    }
                    else if(p.meta.position == Part::Position::Middle)
                    {
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenClose});
                        toks.push_back(Token{"/", p.meta.next, 0, Token::Type::Operator});
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenOpen});
                    }
                    else
                    {
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenClose});
                        toks.push_back(Token{std::string_view{}, p.meta.next, 0, Token::Type::ParenClose});
                    }
                }
            }
            else
            {
                int pos = 0;
                const char* beg = p.value.c_str();
                for(const char c : p.value)
                {
                    if(('0' <= c && c <= '9') || c == '.')
                    {
                        if(tmp_is_num == 1)
                        {
                            ++len;
                        }
                        else
                        {
                            if(len != 0)
                            {
                                if(toks.size() && toks.back().type == Token::Type::ParenClose)
                                {
                                    toks.push_back(Token{"*", current_idx, start, Token::Type::Operator});
                                }
                                toks.push_back(Token{{beg + start, size_t(len)}, current_idx, start, Token::Type::Variable});
                                toks.push_back(Token{"*", current_idx, start, Token::Type::Operator});
                            }

                            start = pos;
                            len = 1;
                            tmp_is_num = 1;
                        }
                    }
                    else if(('a' <= c && c <= 'z') || c == 'P')
                    {
                        if(tmp_is_num == 0)
                        {
                            ++len;
                        }
                        else
                        {
                            if(len != 0)
                            {
                                if(toks.size() && toks.back().type == Token::Type::ParenClose)
                                {
                                    toks.push_back(Token{"*", current_idx, start, Token::Type::Operator});
                                }
                                toks.push_back(Token{{beg + start, size_t(len)}, current_idx, start, Token::Type::Number});
                                toks.push_back(Token{"*", current_idx, start, Token::Type::Operator});
                            }

                            start = pos;
                            len = 1;
                            tmp_is_num = 0;
                        }
                    }
                    else
                    {
                        if(len != 0)
                        {
                            toks.push_back(Token{{beg + start, size_t(len)}, current_idx, start, tmp_is_num ? Token::Type::Number : Token::Type::Variable});
                        }

                        start = 0;
                        len = 0;
                        tmp_is_num = -1;

                        toks.push_back(Token{{beg + pos, 1}, current_idx, pos, Token::Type::Operator});
                    }

                    ++pos;
                }

                if(tmp_is_num == 0)
                {
                    if(const auto& pn = parts[p.meta.next].meta; pn.position == Part::Position::Start && pn.special == Part::Specialty::Paren)
                    {
                        if(len == 1 || std::string_view{beg + start, size_t(len)} == "ans") // 1 letter names can only be variables
                        {
                            toks.push_back(Token{{beg + start, size_t(len)}, current_idx, start, Token::Type::Variable});
                            toks.push_back(Token{"*", current_idx, start, Token::Type::Operator});
                        }
                        else
                        {
                            toks.push_back(Token{{beg + start, size_t(len)}, current_idx, start, Token::Type::Function});
                        }
                    }
                    else
                    {
                        toks.push_back(Token{{beg + start, size_t(len)}, current_idx, start, Token::Type::Variable});
                    }
                }
                else if(tmp_is_num == 1)
                {
                    toks.push_back(Token{{beg + start, size_t(len)}, current_idx, start, Token::Type::Number});
                }

                start = 0;
                len = 0;
                tmp_is_num = -1;
            }
            current_idx = p.meta.next;
        }

        return toks;
    }();
    if(base_tokens.empty()) return std::make_pair(Number{}, true);

    const auto final_rpn = [&]() -> std::vector<const Token*> {
        std::vector<const Token*> postfix;
        std::vector<const Token*> opstack;

        const auto get_prec = [](std::string_view op) -> int {
            switch(op.front())
            {
                case '^': return 4;
                case '*': return 3;
                case '/': return 3;
                case '+': return 2;
                case '-': return 2;
                case '>': return 1;
                default: return 0;
            }
        };

        const auto get_assoc = [](std::string_view op) -> bool {
            return op.front() == '^';
        };

        for(const Token& tok : base_tokens)
        {
            if(tok.type == Token::Type::Variable || tok.type == Token::Type::Number)
            {
                postfix.push_back(&tok);
            }
            else if(tok.type == Token::Type::ParenOpen)
            {
                opstack.push_back(&tok);
            }
            else if(tok.type == Token::Type::ParenClose)
            {
                while(opstack.size() && opstack.back()->type != Token::Type::ParenOpen)
                {
                    postfix.push_back(opstack.back());
                    opstack.pop_back();
                }

                opstack.pop_back();  // open paren (mismatch cannot happen)
            }
            else if(tok.type == Token::Type::Function)
            {
                opstack.push_back(&tok);
            }
            else if(tok.type == Token::Type::Operator)
            {
                auto prec = get_prec(tok.value);
                while(opstack.size())
                {
                    if(const Token* op = opstack.back();
                        op->type != Token::Type::ParenOpen && (
                            op->type == Token::Type::Function ||
                            get_prec(op->value) >= prec ||
                            (get_prec(op->value) == prec && get_assoc(op->value))
                        )
                    )
                    {
                        postfix.push_back(op);
                        opstack.pop_back();
                    }
                    else
                        break;
                }
                opstack.push_back(&tok);
            }
        }

        while(opstack.size())
        {
            postfix.push_back(opstack.back());
            opstack.pop_back();
        }

        return postfix;
    }();
    if(final_rpn.empty()) std::make_pair(Number{}, true);

    #undef ERROR_AT
    #define ERROR_AT(part, pos) { error_part = part; error_position = pos; return std::make_pair(Number{}, true); }

    struct Value {
        Number val;
        std::string_view assoc_variable;

        Value(std::string_view v) : val(v) { }
        Value(const Number& v) : val(v) { }
        Value(const Number& v, std::string_view a) : val(v), assoc_variable(a) { }
    };
    const auto get_var = [&](std::string_view name) -> Value {
        if(auto it = variables.find(std::string(name)); it != variables.end())
        {
            return {it->second, name};
        }
        return {{}, name};
    };

    #define MK_WRAPPER_FN(fname, fn) {#fname, [](std::vector<Value>& vals) { \
        vals.back() = Value(std::fn(vals.back().val.value)); \
    }}
    #define MK_WRAPPER(fname) MK_WRAPPER_FN(fname, fname)

    const std::map<std::string_view, void(*)(std::vector<Value>&)> function_handlers{
        MK_WRAPPER(abs),
        MK_WRAPPER(sqrt),
        MK_WRAPPER(conj),

        MK_WRAPPER(cos),
        MK_WRAPPER(sin),
        MK_WRAPPER(tan),

        MK_WRAPPER(acos),
        MK_WRAPPER(asin),
        MK_WRAPPER(atan),

        MK_WRAPPER(cosh),
        MK_WRAPPER(sinh),
        MK_WRAPPER(tanh),

        MK_WRAPPER(acosh),
        MK_WRAPPER(asinh),
        MK_WRAPPER(atanh),

        MK_WRAPPER(exp),
        MK_WRAPPER_FN(ln, log),
        MK_WRAPPER_FN(log, log10),
    };

    std::vector<Value> value_stack;
    for(const Token* tok : final_rpn)
    {
        if(tok->type == Token::Type::Number)
        {
            if(tok->value.front() == '.' || tok->value.back() == '.')
            {
                ERROR_AT(tok->part, tok->position);
            }
            else
            {
                value_stack.emplace_back(tok->value);
            }
        }
        else if(tok->type == Token::Type::Variable)
        {
            if(tok->value == "P")
            {
                value_stack.emplace_back(PI_VAL);
            }
            else if(tok->value == "e")
            {
                value_stack.emplace_back(E_VAL);
            }
            else if(tok->value == "i")
            {
                value_stack.emplace_back(I_VAL);
            }
            else
            {
                value_stack.push_back(get_var(tok->value));
            }
        }
        else if(tok->type == Token::Type::Operator)
        {
            #define OP_CASE(op) case #op[0] : { \
                const Value left = value_stack.back(); \
                value_stack.pop_back(); \
                const Value right = value_stack.back(); \
                value_stack.pop_back(); \
                value_stack.emplace_back(right.val.value op left.val.value); \
            } break;
            switch(tok->value.front())
            {
                OP_CASE(+)
                OP_CASE(-)
                OP_CASE(*)
                OP_CASE(/)
                case '^':
                {
                    const Value left = value_stack.back();
                    value_stack.pop_back();
                    const Value right = value_stack.back();
                    value_stack.pop_back();

                    value_stack.emplace_back(std::pow(right.val.value, left.val.value));
                }
                break;
                case '>':
                {
                    const Value left = value_stack.back();
                    value_stack.pop_back();
                    const Value right = value_stack.back();
                    value_stack.pop_back();

                    variables.insert_or_assign(std::string(left.assoc_variable), right.val.value);
                    value_stack.push_back(right);
                }
                break;
            }
        }
        else if(tok->type == Token::Type::Function)
        {
            if(auto it = function_handlers.find(tok->value); it != function_handlers.end())
            {
                it->second(value_stack);
            }
        }
    }

    return {value_stack.back().val, false};
    #undef ERROR_AT
}

int Equation::set_special(const int current_part_id, const int at_position, const Part::Specialty special)
{
    if(static_cast<size_t>(at_position) != parts[current_part_id].value.size()) return 0;

    const int next_id = parts[current_part_id].meta.next;
    if(next_id == -1) return 0;

    auto& p = parts[next_id];
    if(p.meta.special != Part::Specialty::Paren || p.meta.position != Part::Position::Start) return 0;

    p.meta.special = special;
    parts[p.meta.assoc].meta.special = special;

    return 1;
}
void Equation::find_matching_tmp_paren(const int original_pos)
{
    using HelperType = int(*)(Equation&, const int);
    const auto do_find = [](Equation& e, const int original_pos, int inc_on_start, HelperType get_following) {
        int current_count = 0;
        int prev_pos = original_pos;
        const Part::Position searching_for = !e.parts[original_pos].meta.position;
        int pos = get_following(e, prev_pos);

        while(current_count >= 0 /* && pos != -1 */) // second test redundant, since we have the Equation start and end chunks
        {
            if(current_count == 0 && e.parts[pos].meta.special == Part::Specialty::TempParen && e.parts[pos].meta.position == searching_for)
            {
                e.parts[pos].meta.special = Part::Specialty::Paren;
                e.parts[pos].meta.assoc = original_pos;
                e.parts[original_pos].meta.special = Part::Specialty::Paren;
                e.parts[original_pos].meta.assoc = pos;
                break;
            }

            if(check_pos_is(e.parts[pos].meta.position, Part::Position::End) && e.parts[pos].meta.special != Part::Specialty::TempParen)
            {
                current_count -= inc_on_start;
            }

            if(check_pos_is(e.parts[pos].meta.position, Part::Position::Start) && e.parts[pos].meta.special != Part::Specialty::TempParen)
            {
                current_count += inc_on_start;
            }

            prev_pos = pos;
            pos = get_following(e, pos);
        }
    };

    if(parts[original_pos].meta.position == Part::Position::Start)
    {
        // go forward
        do_find(*this, original_pos, +1,
            [](Equation& e, const int pos) -> int {
                return e.parts[pos].meta.next;
            }
        );
    }
    else if(parts[original_pos].meta.position == Part::Position::End)
    {
        // go backwards
        do_find(*this, original_pos, -1,
            [](Equation& e, const int pos) -> int {
                return e.parts[pos].meta.before;
            }
        );
    }
}
std::pair<bool, bool> Equation::add_part_at(int& current_part_id, int& at_position, const Part::Specialty special, const Part::Position position, const int assoc)
{
    std::string before_val = parts[current_part_id].value.substr(0, at_position);
    std::string after_val = parts[current_part_id].value.substr(at_position);
    const bool after_val_empty = after_val.empty();
    const bool before_val_empty = before_val.empty();

    const int new_part_id = parts.size();
    if(after_val_empty)
    {
        parts.emplace_back();
        auto& current_part = parts[current_part_id];
        auto& new_part = parts[new_part_id];

        new_part.meta.special = special;
        new_part.meta.position = position;
        new_part.meta.assoc = assoc;

        new_part.meta.before = current_part_id;
        new_part.meta.next = current_part.meta.next;
        parts[new_part.meta.next].meta.before = new_part_id;
        current_part.meta.next = new_part_id;
    }
    else if(before_val_empty)
    {
        parts.emplace_back();
        auto& current_part = parts[current_part_id];
        auto& new_part = parts[new_part_id];

        new_part.meta.special = special;
        new_part.meta.position = position;
        new_part.meta.assoc = assoc;

        new_part.meta.before = current_part.meta.before;
        new_part.meta.next = current_part_id;
        parts[new_part.meta.before].meta.next = new_part_id;
        current_part.meta.before = new_part_id;
    }
    else
    {
        parts.emplace_back();
        const int after_part_id = parts.size();
        parts.emplace_back();

        auto& current_part = parts[current_part_id];
        auto& new_part = parts[new_part_id];
        auto& after_part = parts[after_part_id];

        new_part.meta.special = special;
        new_part.meta.position = position;
        new_part.meta.assoc = assoc;

        new_part.meta.before = current_part_id;
        new_part.meta.next = after_part_id;
        after_part.meta.before = new_part_id;
        after_part.meta.next = current_part.meta.next;
        current_part.meta.next = new_part_id;

        current_part.value = std::move(before_val);
        after_part.value = std::move(after_val);
    }

    current_part_id = new_part_id;
    at_position = 0;
    const auto& new_part = parts[new_part_id];
    return {parts[new_part.meta.before].meta.special == Part::Specialty::None, parts[new_part.meta.next].meta.special == Part::Specialty::None};
}

bool Equation::remove_at(int& current_part_id, int& at_position)
{
    if(at_position == 0)
    {
        const auto merge_single_part = [this](Part::Meta single_meta) -> std::pair<int, int> {
            const int before_part_start_id = single_meta.before;
            const int after_part_start_id = single_meta.next;

            const int part_id = before_part_start_id;
            const int at_char = parts[part_id].value.size();

            /*
             * equ_start -> A -> single -> B -> equ_end
             * append B to A
             * make A's next into B's next
             * make B's next's before into A
             * equ_start -> AB -> equ_end
             */
            
            Part::Meta after_start_meta = parts[after_part_start_id].meta;

            parts[before_part_start_id].value.append(parts[after_part_start_id].value);
            parts[before_part_start_id].meta.next = after_start_meta.next;
            parts[after_start_meta.next].meta.before = before_part_start_id;

            return {part_id, at_char};
        };
        const auto merge_parts = [this](Part::Meta start_meta, Part::Meta end_meta) -> std::pair<int, int> {
            const int before_part_start_id = start_meta.before;
            const int after_part_start_id = start_meta.next;
            const int before_part_end_id = end_meta.before;
            const int after_part_end_id = end_meta.next;

            const int part_id = before_part_start_id;
            const int at_char = parts[part_id].value.size();

            /*
             * if after_part_start_id != before_part_end_id:
             * equ_start -> A -> start -> B -> ... -> C -> end -> D -> equ_end
             * append D to C
             * make C's next into D's next
             * make C's new next's before into C
             * equ_start -> A -> start -> B -> ... -> CD -> equ_end
             * append B to A
             * make A's next into B's next
             * make CD's next's before into C
             * equ_start -> A -> start -> B -> ... -> CD -> equ_end
             */

            /*
             * if after_part_start_id == before_part_end_id:
             * equ_start -> A -> start -> B -> end -> C -> equ_end
             * append C to B
             * make B's next into C's next
             * make B's new next's before into B
             * equ_start -> A -> start -> BC -> equ_end
             * append BC to A
             * make A's next into BC's next
             * make A's next's before into A
             * equ_start -> ABC -> equ_end
             */

            Part::Meta after_end_meta = parts[after_part_end_id].meta;

            parts[before_part_end_id].value.append(parts[after_part_end_id].value);
            parts[before_part_end_id].meta.next = after_end_meta.next;
            parts[after_end_meta.next].meta.before = before_part_end_id;

            Part::Meta after_start_meta = parts[after_part_start_id].meta;

            parts[before_part_start_id].value.append(parts[after_part_start_id].value);
            parts[before_part_start_id].meta.next = after_start_meta.next;
            parts[after_start_meta.next].meta.before = before_part_start_id;

            return {part_id, at_char};
        };

        Part::Meta start_meta = parts[parts[current_part_id].meta.before].meta; // select the special chunk before the writing area we're in
        
        // Don't allow deleting the first chunk or chunks we're after the end of. chunks have to be deleted from the front of the inside
        if((!(start_meta.special == Part::Specialty::Paren || start_meta.special == Part::Specialty::TempParen) && check_pos_is(start_meta.position, Part::Position::End)) || start_meta.special == Part::Specialty::Equation) return false;

        if(start_meta.special == Part::Specialty::Fraction)
        {
            Part::Meta middle_meta = parts[start_meta.assoc].meta;
            Part::Meta end_meta = parts[middle_meta.assoc].meta;

            merge_parts(middle_meta, end_meta);

            const int before_part_start_id = start_meta.before;

            const int part_id = before_part_start_id;
            const int at_char = parts[part_id].value.size();

            Part::Meta after_start_meta = parts[start_meta.next].meta;

            parts[before_part_start_id].value.append(parts[start_meta.next].value);
            parts[before_part_start_id].meta.next = after_start_meta.next;
            parts[after_start_meta.next].meta.before = before_part_start_id;

            current_part_id = part_id;
            at_position = at_char;
            return true;
        }
        else if(start_meta.special == Part::Specialty::Paren)
        {
            parts[start_meta.assoc].meta.assoc = -1;
            parts[start_meta.assoc].meta.special = Part::Specialty::TempParen;
            std::tie(current_part_id, at_position) = merge_single_part(start_meta);
            return true;
        }
        else if(start_meta.special == Part::Specialty::TempParen)
        {
            std::tie(current_part_id, at_position) = merge_single_part(start_meta);
            return true;
        }
        else
        {
            std::tie(current_part_id, at_position) = merge_parts(start_meta, parts[start_meta.assoc].meta);
            return true;
        }
    }
    else
    {
        at_position -= 1;
        parts[current_part_id].value.erase(at_position, 1);
        return true;
    }
}

bool Equation::left_of(int& current_part_id, int& at_position)
{
    if(at_position != 0)
    {
        at_position--;
        return true;
    }

    if(parts[parts[current_part_id].meta.before].meta.special != Part::Specialty::Equation)
    {
        do {
            current_part_id = parts[current_part_id].meta.before;
        } while(parts[current_part_id].meta.special != Part::Specialty::None);
        at_position = parts[current_part_id].value.size();
        return true;
    }

    return false;
}

bool Equation::right_of(int& current_part_id, int& at_position)
{
    if(static_cast<size_t>(at_position) != parts[current_part_id].value.size())
    {
        at_position++;
        return true;
    }

    if(parts[parts[current_part_id].meta.next].meta.special != Part::Specialty::Equation)
    {
        do {
            current_part_id = parts[current_part_id].meta.next;
        } while(parts[current_part_id].meta.special != Part::Specialty::None);
        at_position = 0;
        return true;
    }

    return false;
}
