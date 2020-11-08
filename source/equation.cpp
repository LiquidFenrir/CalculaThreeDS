#include "equation.h"
#include "sprites.h"
#include "text.h"
#include "colors.h"
#include <algorithm>
#include <climits>

void Number::render(C2D_SpriteSheet sprites) const
{
    static char floattextbuf[64];
    std::snprintf(floattextbuf, 63, "%.6g", value);

    C2D_ImageTint text_tint;
    C2D_PlainImageTint(&text_tint, COLOR_BLACK, 1.0f);
    C2D_DrawRectSolid(0, Equation::EQU_REGION_HEIGHT, 0.875f, 400, 40, COLOR_GRAY);

    std::string_view sv(floattextbuf);
    const int w = sv.size() * 13;
    int x = 0;
    for(const char c : sv)
    {
        const auto& img = TextMap::char_to_sprite->equ.at(std::string_view(&c, 1)).sprites.front();
        C2D_DrawImageAt(img, 400 - w + x, Equation::EQU_REGION_HEIGHT + (40 - img.subtex->height)/2, 1.0f, &text_tint);
        x += 13;
    }
}

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
    const auto str_to_len = [](const std::string& s) -> int {
        return ((s.empty()) ? (1) : (s.size()));
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

        ps.data = str_to_len(p.value);
        ps.content_width = ps.data;
        ps.y_start = 1;
        ps.y_end = -1;

        add_content_info(ps);
    };

    int current_idx = 0;

    while(current_idx != -1)
    {
        const auto& part = parts[current_idx];
        if(part.meta.special == Part::Specialty::None)
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
                else if(part.meta.special == Part::Specialty::Paren)
                {
                    aps.associated = current_idx;
                    ps.associated = associated_id;

                    ps.paren_y_start = aps.paren_y_start;
                    ps.paren_y_end = aps.paren_y_end;

                    aps.content_width += 2;
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

    C2D_ImageTint text_tint;
    C2D_PlainImageTint(&text_tint, COLOR_BLACK, 1.0f);
    C2D_ImageTint temp_tint;
    C2D_PlainImageTint(&temp_tint, COLOR_GRAY, 1.0f);

    const auto draw_paren = [&](const C2D_Image* sprites, const int vertical_offset, const int y_start, const int y_end, const bool tmp) -> void {
        const int span = (y_start - y_end) - 2;
        const int span_pixels = (span * 24 / 4) + 1;
        const int pixel_y = info.get_y(vertical_offset) - ((y_start - 1) * 24 / 4);
        const int x = info.get_x();
        C2D_ImageTint* tnt = tmp ? &temp_tint : &text_tint;
        C2D_DrawImageAt(sprites[0], x, pixel_y, 0.0f, tnt);
        const int middle_y = pixel_y + sprites[0].subtex->height;
        C2D_DrawImageAt(sprites[1], x, middle_y, 0.0f, tnt, 1.0f, span_pixels);
        const int bottom_y = middle_y + span_pixels;
        C2D_DrawImageAt(sprites[2], x, bottom_y, 0.0f, tnt);
    };
    const auto set_cursor = [&](const int y) -> void {
        out.cursor_x = info.get_x();
        out.cursor_y = y;
        if(!(out.cursor_x <= -2 || out.cursor_x >= (320 - 2) || out.cursor_y <= (-24) || out.cursor_y >= Equation::EQU_REGION_HEIGHT))
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
                if(info.can_draw(vertical_offset))
                {
                    const auto& img = empty_img;
                    C2D_DrawImageAt(img, info.get_x(), info.get_y(vertical_offset), 0.0f, &text_tint);

                    if(screen)
                    {
                        pos.pos = 0;
                        for(int y = 0; y < 24; y++)
                        {
                            const int actual_y = info.get_y(vertical_offset) + y;
                            if(actual_y >= 0 && actual_y < Equation::EQU_REGION_HEIGHT)
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

                info.current_x += 13;
            }
            else
            {
                int char_idx = 0;
                for(const char c : part.value)
                {
                    if(info.can_draw(vertical_offset))
                    {
                        const auto& img = TextMap::char_to_sprite->equ.at(std::string_view(&c, 1)).sprites.front();
                        C2D_DrawImageAt(img, info.get_x(), info.get_y(vertical_offset), 0.0f, &text_tint);
                        if(screen)
                        {
                            pos.pos = char_idx;
                            for(int y = 0; y < 24; y++)
                            {
                                const int actual_y = info.get_y(vertical_offset) + y;
                                if(actual_y >= 0 && actual_y < Equation::EQU_REGION_HEIGHT)
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

            pos.pos += 1; // makes parens put the cursor at the end
        }
        else if(part.meta.special != Part::Specialty::Equation)
        {
            if(part.meta.special == Part::Specialty::Paren)
            {
                const C2D_Image* sprs = nullptr;
                if(part.meta.position == Part::Position::Start)
                {
                    sprs = lpa_sprites;
                }
                else if(part.meta.position == Part::Position::End)
                {
                    sprs = rpa_sprites;
                }

                draw_paren(sprs, vertical_offset, part_size.paren_y_start, part_size.paren_y_end, part.meta.tmp);

                if(screen)
                {
                    const int h = ((part_size.paren_y_start - part_size.paren_y_end) * 24 / 2) + 1;
                    for(int y = 0; y < h; y++)
                    {
                        const int actual_y = info.get_y(vertical_offset) - ((part_size.paren_y_start - 1) * 24 / 2) + y;
                        if(actual_y >= 0 && actual_y < Equation::EQU_REGION_HEIGHT)
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

Equation::RenderResult Equation::render(const int x, const int y, const int editing_part, const int editing_char, C2D_SpriteSheet sprites, PartPos* screen)
{
    RenderResult out{0,0,0,-1,-1, false};
    RenderInfo info{
        0,
        x, x + 320, y,
        Equation::EQU_REGION_HEIGHT,
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
        Equation::EQU_REGION_HEIGHT,
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

Number Equation::calculate(const Number& input)
{
    return input;
}

void Equation::find_matching(const int original_pos, const Part::Specialty special)
{
    using HelperType = int(*)(Equation&, const int);
    const auto do_find = [](Equation& e, const int original_pos, const Part::Specialty special, int inc_on_start, HelperType get_following, HelperType get_append_pos) {
        const auto do_append = [](Equation& e, const int p, const int original_pos, const Part::Specialty special, HelperType get_append_pos) -> void {
            int char_position = get_append_pos(e, p);
            int part_id = p;
            const auto [have_any_before, have_any_after] = e.add_part_at(part_id, char_position, special, !e.parts[original_pos].meta.position, original_pos);
            
            if(special == Part::Specialty::Paren)
            {
                e.parts[part_id].meta.tmp = true;
            }

            e.parts[original_pos].meta.assoc = part_id;
            if(!have_any_after)
            {
                e.add_part_at(part_id, char_position);
            }
        };

        int current_count = 0;
        int prev_pos = original_pos;
        int pos = get_following(e, prev_pos);

        while(true) // will eventually hit a part
        {
            if(check_pos_is(e.parts[pos].meta.position, Part::Position::End))
            {
                current_count -= inc_on_start;
            }

            if(check_pos_is(e.parts[pos].meta.position, Part::Position::Start))
            {
                current_count += inc_on_start;
            }

            if(current_count < 0)
            {
                do_append(e, prev_pos, original_pos, special, get_append_pos);
                return;
            }

            prev_pos = pos;
            pos = get_following(e, pos);
        }
    };

    if(parts[original_pos].meta.position == Part::Position::Start)
    {
        // go forward
        do_find(*this, original_pos, special, +1,
            [](Equation& e, const int pos) -> int {
                return e.parts[pos].meta.next;
            },
            [](Equation& e, const int pos) -> int {
                return e.parts[pos].value.size();
            }
        );
    }
    else if(parts[original_pos].meta.position == Part::Position::End)
    {
        // go backwards
        do_find(*this, original_pos, special, -1,
            [](Equation& e, const int pos) -> int {
                return e.parts[pos].meta.before;
            },
            [](Equation& e, const int pos) -> int {
                return 0;
            }
        );
    }
}
std::pair<bool, bool> Equation::add_part_at(int& current_part_id, int& at_position, const Part::Specialty special, const Part::Position position, const int assoc)
{
    if(assoc == -1 && static_cast<size_t>(at_position) == parts[current_part_id].value.size())
    {
        auto& next_part = parts[parts[current_part_id].meta.next];
        if(special == Part::Specialty::Paren && next_part.meta.special == Part::Specialty::Paren && next_part.meta.position == position && next_part.meta.tmp)
        {
            next_part.meta.tmp = false;
            current_part_id = parts[current_part_id].meta.next;
            at_position = 0;
            return {true, true}; // if it's a paren, it has to have a text part after
        }
    }

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

        Part::Meta start_meta = parts[parts[current_part_id].meta.before].meta;
        if(start_meta.special == Part::Specialty::Equation) // Don't allow deleting the first chunk
        {
            return false;
        }
        else if(start_meta.special == Part::Specialty::Fraction)
        {
            if(start_meta.position != Part::Position::Start)
            {
                return false;
            }

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
