#include <algorithm>
#include <cmath>
#include "keyboard.h"
#include "text.h"
#include "sprites.h"

void calculation_loop(void* arg)
{
    auto kb = static_cast<Keyboard*>(arg);
    while(!kb->kill_thread.load())
    {
        LightEvent_Wait(&kb->wakeup);
        if(kb->kill_thread.load()) break;

        fprintf(stderr, "optimize\n");
        kb->current_eq->optimize();
        fprintf(stderr, "calc\n");
        kb->result = kb->current_eq->calculate(kb->memory.empty() ? Number{} : kb->memory.back().result);
        fprintf(stderr, "done\n");
        kb->stop_calculating();
    }
}

size_t Keyboard::Menu::add(std::string_view submenu, ActionType f)
{
    const size_t o = entries.size();
    entries.emplace_back(this, submenu, f);
    return o;
}

static constexpr u32 COLOR_HIDE = C2D_Color32(32, 32, 32, 64);
static constexpr u32 COLOR_WHITE = C2D_Color32(255,255,255,255);
static constexpr u32 COLOR_GRAY = C2D_Color32(160,160,160,255);
static constexpr u32 COLOR_BLACK = C2D_Color32(0,0,0,255);
#define REMOVE_ODD(v) (((v) & ~1) + (((v) & 1) << 1))
#define MK_SUBTEX(w, h, w2, h2) {(w), REMOVE_ODD(h), 0.0f, 1.0f, ((w)/(w2)), 1.0f - (REMOVE_ODD(h)/(h2))}
#define MK_SUBTEX_OFFSET(w, h, w2, h2, oX, oY) {(w), REMOVE_ODD(h), ((oX)/(w2)), 1.0f - (REMOVE_ODD(oY)/(h2)), ((w)/(w2)), 1.0f - (REMOVE_ODD(oY)/(h2)) - (REMOVE_ODD(h)/(h2))}
static constexpr Tex3DS_SubTexture EQUATION_SUBTEX = MK_SUBTEX(320, Equation::EQU_REGION_HEIGHT, 512.0f, 128.0f);
static constexpr Tex3DS_SubTexture EQUATION_MEM_SUBTEX = MK_SUBTEX_OFFSET(400, Equation::EQU_REGION_HEIGHT, 512.0f, 128.0f, 0, 0);
static constexpr Tex3DS_SubTexture ANSWER_MEM_SUBTEX = MK_SUBTEX_OFFSET(400, 120 - Equation::EQU_REGION_HEIGHT, 512.0f, 128.0f, 0, Equation::EQU_REGION_HEIGHT);
static constexpr Tex3DS_SubTexture KEYBOARD_SUBTEX = MK_SUBTEX(320, 240 - (Equation::EQU_REGION_HEIGHT + 1), 512.0f, 256.0f);
#undef MK_SUBTEX_OFFSET
#undef MK_SUBTEX
#undef REMOVE_ODD
static C2D_ImageTint YES_TINT, NO_TINT, TEXT_YES_TINT, TEXT_NO_TINT;

void add_exponent(Equation& e, int& cur_char, int& cur_part)
{
    e.add_part_at(cur_part, cur_char, Part::Specialty::Exponent, Part::Position::Start);
    const auto assoc_s = cur_part;
    e.add_part_at(cur_part, cur_char);
    const auto cp_part = cur_part;
    const auto cp_char = cur_char;
    const auto [have_any_before, have_any_after] = e.add_part_at(cur_part, cur_char, Part::Specialty::Exponent, Part::Position::End, assoc_s);
    e.parts[assoc_s].meta.assoc = cur_part;
    if(!have_any_after)
    {
        e.add_part_at(cur_part, cur_char);
    }
    cur_part = cp_part;
    cur_char = cp_char;
}
void add_paren(Equation& e, int& cur_char, int& cur_part, const Part::Position direction)
{
    const auto [have_any_before, have_any_after] = e.add_part_at(cur_part, cur_char, Part::Specialty::Paren, direction);
    const int cp_part = cur_part;
    if(!have_any_after)
    {
        e.add_part_at(cur_part, cur_char);
    }
    else
    {
        e.right_of(cur_part, cur_char);
    }

    if(e.parts[cp_part].meta.assoc == -1)
    {
        e.find_matching(cp_part, Part::Specialty::Paren);
    }
}

Keyboard::Keyboard(C2D_SpriteSheet sprites)
:
calculating_flag(false), wait_thread(false), kill_thread(false),
keyboard_tex(512, 256, GPU_RGBA8),
current_tex(512, 128, GPU_RGBA8),
memory_scroll(0), memory_index(0),
redo_top(false), redo_bottom(false),
calcThread(nullptr),
root_menu(nullptr, ""), current_menu(&root_menu),
selected_entry(-1), changed_keyboard(true)
{
    LightEvent_Init(&wakeup, RESET_ONESHOT);

    for(auto& p : memory_tex)
    {
        p = std::make_unique<Tex>(512, 128, GPU_RGBA8);
    }

    root_menu.entries.reserve(3);
    const auto punct_menu_id = root_menu.add("punctuation");
    const auto func_menu_id = root_menu.add("functions");
    const auto num_menu_id = root_menu.add("numbers");

    #define ADD_CHAR(v) [](Equation& e, int& cur_char, int& cur_part) -> void { \
        e.parts[cur_part].value.insert(cur_char, v); \
        cur_char += std::size(v) - 1; /* dont count NUL */ \
    }

    auto& punct_menu = root_menu.entries[punct_menu_id];

    punct_menu.add("+", ADD_CHAR("+"));
    punct_menu.add(")", [](Equation& e, int& cur_char, int& cur_part) -> void {
        add_paren(e, cur_char, cur_part, Part::Position::End);
    });
    punct_menu.add(".", ADD_CHAR("."));
    punct_menu.add("/", [](Equation& e, int& cur_char, int& cur_part) -> void {
        e.add_part_at(cur_part, cur_char, Part::Specialty::Fraction, Part::Position::Start);
        const auto assoc_s = cur_part;
        e.add_part_at(cur_part, cur_char);
        const auto cp_part = cur_part;
        const auto cp_char = cur_char;
        e.add_part_at(cur_part, cur_char, Part::Specialty::Fraction, Part::Position::Middle);
        e.parts[assoc_s].meta.assoc = cur_part;
        const auto assoc_m = cur_part;
        e.add_part_at(cur_part, cur_char);
        const auto [have_any_before, have_any_after] = e.add_part_at(cur_part, cur_char, Part::Specialty::Fraction, Part::Position::End, assoc_m);
        e.parts[assoc_m].meta.assoc = cur_part;
        if(!have_any_after)
        {
            e.add_part_at(cur_part, cur_char);
        }
        cur_part = cp_part;
        cur_char = cp_char;
    });
    punct_menu.add("*", ADD_CHAR("*"));
    punct_menu.add("^", add_exponent);
    punct_menu.add("(", [](Equation& e, int& cur_char, int& cur_part) -> void {
        add_paren(e, cur_char, cur_part, Part::Position::Start);
    });
    punct_menu.add("-", ADD_CHAR("-"));

    auto& func_menu = root_menu.entries[func_menu_id];
    func_menu.add("ln", ADD_CHAR("ln"));
    func_menu.add("log", ADD_CHAR("log"));
    func_menu.add("abs", [](Equation& e, int& cur_char, int& cur_part) -> void {
        const auto [have_any_before, have_any_after] = e.add_part_at(cur_part, cur_char, Part::Specialty::Absolute, Part::Position::Start);
        const int cp_part = cur_part;
        if(!have_any_after)
        {
            e.add_part_at(cur_part, cur_char);
        }
        else
        {
            e.right_of(cur_part, cur_char);
        }

        e.find_matching(cp_part, Part::Specialty::Absolute);
    });
    func_menu.add("cos", ADD_CHAR("cos"));
    func_menu.add("sin", ADD_CHAR("sin"));
    func_menu.add("tan", ADD_CHAR("tan"));
    func_menu.add("acos", ADD_CHAR("acos"));
    func_menu.add("asin", ADD_CHAR("asin"));
    func_menu.add("atan", ADD_CHAR("atan"));
    func_menu.add("exp", [](Equation& e, int& cur_char, int& cur_part) -> void {
        e.parts[cur_part].value.insert(cur_char, "e");
        cur_char++;
        add_exponent(e, cur_char, cur_part);
    });

    auto& num_menu = root_menu.entries[num_menu_id];
    num_menu.add("0", ADD_CHAR("0"));
    num_menu.add("1", ADD_CHAR("1"));
    num_menu.add("2", ADD_CHAR("2"));
    num_menu.add("3", ADD_CHAR("3"));
    num_menu.add("4", ADD_CHAR("4"));
    num_menu.add("5", ADD_CHAR("5"));
    num_menu.add("6", ADD_CHAR("6"));
    num_menu.add("7", ADD_CHAR("7"));
    num_menu.add("8", ADD_CHAR("8"));
    num_menu.add("9", ADD_CHAR("9"));
    num_menu.add("pi", ADD_CHAR("P"));
    num_menu.add("ans", ADD_CHAR("ans"));

    #undef ADD_CHAR

    TextMap::generate(sprites);
    C2D_PlainImageTint(&YES_TINT, COLOR_GRAY, 1.0f);
    C2D_PlainImageTint(&NO_TINT, COLOR_BLACK, 1.0f);
    C2D_PlainImageTint(&TEXT_YES_TINT, COLOR_BLACK, 1.0f);
    C2D_PlainImageTint(&TEXT_NO_TINT, COLOR_GRAY, 1.0f);

    C2D_SpriteFromSheet(&spinning_loader, sprites, sprites_loader_idx);

    C2D_SpriteSetPos(&spinning_loader, 320.0f/2.0f, 240.0f/2.0f);
    C2D_SpriteSetDepth(&spinning_loader, 1.0f);
    C2D_SpriteSetCenter(&spinning_loader, 0.5f, 0.5f);

    memory.reserve(MemorySize);
    start_equation(false);
}

Keyboard::~Keyboard()
{
    if(calcThread)
    {
        kill_thread.store(true);
        LightEvent_Signal(&wakeup);
        threadJoin(calcThread, U64_MAX);
        threadFree(calcThread);
    }

    TextMap::char_to_sprite.reset();
}

void Keyboard::handle_buttons(const u32 kDown, const u32 kDownRepeat)
{
    if(kDown & KEY_X)
    {
        select_next_type();
    }
    else if(selection == SelectionType::Equation)
    {
        if(kDown & KEY_A)
        {
            start_calculating();
        }
        else if(kDown & KEY_B)
        {
            any_change = current_eq->remove_at(editing_part, editing_char);
        }
        else if(kDown & KEY_Y)
        {
            start_equation(false);
        }
        else if(kDownRepeat & KEY_DLEFT)
        {
            any_change = current_eq->left_of(editing_part, editing_char);
            cursor_on = true;
            cursor_toggle_time = osGetTime();
        }
        else if(kDownRepeat & KEY_DRIGHT)
        {
            any_change = current_eq->right_of(editing_part, editing_char);
        }
    }
    else if(selection == SelectionType::Keyboard)
    {
        if(kDown & KEY_A)
        {
            if(selected_entry != -1)
            {
                if(auto& m = current_menu->entries[selected_entry]; m.func != nullptr)
                {
                    any_change = true;
                    m.func(*current_eq, editing_char, editing_part);
                }
                else
                {
                    current_menu = &m;
                    selected_entry = -1;
                    changed_keyboard = true;
                }
            }
        }
        else if(kDown & KEY_B)
        {
            if(selected_entry != -1)
            {
                selected_entry = -1;
                changed_keyboard = true;
            }
            else if(current_menu->parent != nullptr)
            {
                auto prev_menu = current_menu->parent;
                current_menu = prev_menu;
                selected_entry = -1;
                changed_keyboard = true;
            }
        }
        else if(kDown & KEY_DUP)
        {
            if(selected_entry == -1)
            {
                selected_entry = 0;
                changed_keyboard = true;
            }
        }
        else if(kDown & KEY_DDOWN)
        {
            if(selected_entry != -1)
            {
                selected_entry = -1;
                changed_keyboard = true;
            }
        }
        else if(kDownRepeat & KEY_DLEFT)
        {
            if(selected_entry != -1)
            {
                if(selected_entry == 0)
                {
                    selected_entry = current_menu->entries.size();
                }
                selected_entry--;
                changed_keyboard = true;
            }
        }
        else if(kDownRepeat & KEY_DRIGHT)
        {
            if(selected_entry != -1)
            {
                selected_entry++;
                if(static_cast<size_t>(selected_entry) == current_menu->entries.size())
                {
                    selected_entry = 0;
                }
                changed_keyboard = true;
            }
        }
    }
    else if(selection == SelectionType::Memory)
    {
        if(kDownRepeat & KEY_DUP)
        {
            if(memory.size() >= 2 && memory_index != (memory.size() - 1))
            {
                memory_index++;
                if(memory_index == (memory_scroll + 2))
                {
                    memory_scroll++;
                    std::swap(memory_tex[0], memory_tex[1]);
                    redo_top = true;
                }
            }
        }
        else if(kDownRepeat & KEY_DDOWN)
        {
            if(memory_index != 0)
            {
                memory_index--;
                if(memory_index < memory_scroll)
                {
                    memory_scroll--;
                    std::swap(memory_tex[0], memory_tex[1]);
                    redo_bottom = true;
                }
            }
        }
    }
}
void Keyboard::handle_circle_pad(const int x, const int y)
{
    if(selection == SelectionType::Equation)
    {
        if(x < -20 && at_x != 0)
        {
            if(auto t = osGetTime(); t - cursor_toggle_time > 300)
            {
                at_x -= 13;
                any_change = true;
            }
        }
        else if(x > 20 && (render_result.w > 320 && at_x != (render_result.w - 320)))
        {
            if(auto t = osGetTime(); t - cursor_toggle_time > 300)
            {
                at_x += 13;
                any_change = true;
            }
        }

        if(y < -20)
        {
            if((std::min(-Equation::EQU_REGION_HEIGHT/2, render_result.min_y) + Equation::EQU_REGION_HEIGHT/2) < at_y)
            {
                any_change = true;
                at_y -= 2;
            }
        }
        else if(y > 20)
        {
            if(((std::max(Equation::EQU_REGION_HEIGHT/2, render_result.max_y) - Equation::EQU_REGION_HEIGHT/2) > at_y))
            {
                any_change = true;
                at_y += 2;
            }
        }
    }
    else if(selection == SelectionType::Keyboard)
    {
        if((x * x + y * y) > 20000)
        {
            const int ang_deg = int((atan2f(x, y) * 180.0f / 3.14159f) + 720) % 360;

            const int partCount = current_menu->entries.size();
            const auto degrees_per_part_full = std::div(360, partCount);
            const auto on_part = std::div(int(ang_deg), degrees_per_part_full.quot);
            if(on_part.rem < 2 || on_part.rem >= (degrees_per_part_full.quot - 2))
            {
                if(selected_entry != -1)
                {
                    selected_entry = -1;
                    changed_keyboard = true;
                }
            }
            else
            {
                if(on_part.quot != selected_entry)
                {
                    selected_entry = on_part.quot;
                    changed_keyboard = true;
                }
            }
        }
    }
}
void Keyboard::handle_touch(const int x, const int y)
{
    if(y < Equation::EQU_REGION_HEIGHT)
    {
        if(selection != SelectionType::Equation)
        {
            selection = SelectionType::Equation;
        }
        else
        {
            const auto& pos = screen_data[x + y * 320];
            if(pos.part != -1 && pos.pos != -1 && (pos.part != editing_part || pos.pos != editing_char))
            {
                editing_part = pos.part;
                editing_char = pos.pos;
                any_change = true;
            }
        }
    }
    else if(y > (Equation::EQU_REGION_HEIGHT + 1))
    {
        if(selection != SelectionType::Keyboard)
        {
            selection = SelectionType::Keyboard;
        }
        else
        {

        }
    }
}

void Keyboard::do_clears()
{
    constexpr u32 transparent_color = C2D_Color32(0,0,0,0);
    if(any_change)
    {
        auto t = current_tex.get_target();
        C2D_TargetClear(t, transparent_color);
    }
    if(changed_keyboard)
    {
        auto t = keyboard_tex.get_target();
        C2D_TargetClear(t, transparent_color);
    }
    if(redo_top)
    {
        auto t = memory_tex[0]->get_target();
        C2D_TargetClear(t, transparent_color);
    }
    if(redo_bottom)
    {
        auto t = memory_tex[1]->get_target();
        C2D_TargetClear(t, transparent_color);
    }
}

void Keyboard::update_memory(C2D_SpriteSheet sprites)
{
    const int top_idx = memory.size() - memory_scroll  - 1;
    if(redo_top)
    {
        redo_top = false;
        auto t = memory_tex[0]->get_target();
        C2D_SceneBegin(t);
        auto& mem_line = memory[top_idx - 1];
        mem_line.render_res = mem_line.equation->render_memory(mem_line.at_x, mem_line.at_y, sprites);
        mem_line.result.render(sprites);
    }
    if(redo_bottom)
    {
        redo_bottom = false;
        auto t = memory_tex[1]->get_target();
        C2D_SceneBegin(t);
        auto& mem_line = memory[top_idx];
        mem_line.render_res = mem_line.equation->render_memory(mem_line.at_x, mem_line.at_y, sprites);
        mem_line.result.render(sprites);
    }
}
void Keyboard::update_keyboard(C2D_SpriteSheet sprites)
{
    if(changed_keyboard)
    {
        changed_keyboard = false;
        auto t = keyboard_tex.get_target();
        C2D_SceneBegin(t);

        using DrawType = void(*)(const std::vector<Menu>&, const int, C2D_SpriteSheet);
        static constexpr DrawType DrawParts = [](const std::vector<Menu>& parts, const int selected, C2D_SpriteSheet sprites) -> void {
            const int partCount = parts.size();
            const auto degrees_per_part_full = std::div(360, partCount);
            const auto degrees_per_part = degrees_per_part_full.quot - 4;

            C2D_Sprite partYes;
            C2D_SpriteFromSheet(&partYes, sprites, sprites_circle_yes_idx);
            C2D_SpriteSetDepth(&partYes, 0.625f);
            C2D_SpriteSetCenter(&partYes, 0.5f, 0.5f);
            C2D_SpriteSetPos(&partYes, KEYBOARD_SUBTEX.width/2.0f, (KEYBOARD_SUBTEX.height)/2.0f);

            C2D_Sprite partNo;
            C2D_SpriteFromSheet(&partNo, sprites, sprites_circle_not_idx);
            C2D_SpriteSetDepth(&partNo, 0.625f);
            C2D_SpriteSetCenter(&partNo, 0.5f, 0.5f);
            C2D_SpriteSetPos(&partNo, KEYBOARD_SUBTEX.width/2.0f, KEYBOARD_SUBTEX.height/2.0f);

            int deg = (degrees_per_part_full.rem/2) + 1;
            for(int p = 0; p < partCount; ++p)
            {
                C2D_Sprite* s = (p == selected ? &partYes : &partNo);
                const C2D_ImageTint* t = (p == selected ? &YES_TINT : &NO_TINT );

                deg += 2;
                for(int d = 0; d < degrees_per_part; ++d)
                {
                    C2D_SpriteSetRotationDegrees(s, deg);
                    C2D_DrawSpriteTinted(s, t);
                    deg++;
                }
                deg += 2;
            }

            deg = (degrees_per_part_full.rem/2) + 1;
            for(int p = 0; p < partCount; ++p)
            {
                const auto& part = TextMap::char_to_sprite->menu.at(parts[p].name);
                const C2D_ImageTint* it = (p == selected ? &TEXT_YES_TINT : &TEXT_NO_TINT );
                const float distance = (p == selected ? 58.0f : 55.0f);

                deg += 2;
                const int d = deg + degrees_per_part/2;

                const float rad_ang = ((d - 90)) * 3.14159f / 180.0f;
                const int dx = std::cos(rad_ang) * distance;
                const int dy = std::sin(rad_ang) * distance;
                float x = (KEYBOARD_SUBTEX.width/2.0f) + dx - (part.width / 2.0f);
                const float y = (KEYBOARD_SUBTEX.height/2.0f) + dy - (24 / 2.0f);
                for(const auto& i : part.sprites)
                {
                    C2D_DrawImageAt(i, x, y, 0.75f, it);
                    x += i.subtex->width;
                }

                deg += degrees_per_part;
                deg += 2;
            }
        };

        DrawParts(current_menu->entries, selected_entry, sprites);
    }
}
void Keyboard::update_equation(C2D_SpriteSheet sprites)
{
    if(any_change)
    {
        any_change = false;
        auto t = current_tex.get_target();
        C2D_SceneBegin(t);
        render_result = current_eq->render(at_x, at_y, editing_part, editing_char, sprites, screen_data.data());
    }
}

void Keyboard::draw_memory(C2D_SpriteSheet sprites) const
{
    C2D_DrawRectSolid(0, 0, 0.0f, 400, 240, COLOR_WHITE);
    C2D_ImageTint arrow_tint;
    C2D_PlainImageTint(&arrow_tint, C2D_Color32(0, 0, 0, 128), 1.0f);

    const auto draw_mem_at = [&](Tex& t, const int y, const MemoryLine& l) {
        C2D_DrawRectSolid(0, y - 1, 0.5f, 400, 1, COLOR_BLACK);
        C2D_DrawRectSolid(0, y + Equation::EQU_REGION_HEIGHT, 0.0f, 400, 40, COLOR_GRAY);
        C2D_DrawRectSolid(0, y + Equation::EQU_REGION_HEIGHT, 0.5f, 400, 1, COLOR_BLACK);

        C2D_DrawImageAt(C2D_Image{t.get_tex(), &EQUATION_MEM_SUBTEX}, 0.0f, y, 0.25f);
        C2D_DrawImageAt(C2D_Image{t.get_tex(), &ANSWER_MEM_SUBTEX}, 0.0f, y + Equation::EQU_REGION_HEIGHT, 0.25f);

        if(l.render_res.w > 400 && l.at_x != (l.render_res.w - 400))
        {
            const auto spr = C2D_SpriteSheetGetImage(sprites, sprites_arrow_right_idx);
            C2D_DrawImageAt(spr, 400 - spr.subtex->width, y + (Equation::EQU_REGION_HEIGHT - spr.subtex->height) / 2.0f, 0.5f, &arrow_tint);
        }
        if(l.at_x != 0)
        {
            const auto spr = C2D_SpriteSheetGetImage(sprites, sprites_arrow_left_idx);
            C2D_DrawImageAt(spr, 0.0f, y + (Equation::EQU_REGION_HEIGHT - spr.subtex->height) / 2.0f, 0.5f, &arrow_tint);
        }

        if((std::max(Equation::EQU_REGION_HEIGHT/2, l.render_res.max_y) - Equation::EQU_REGION_HEIGHT/2) > l.at_y)
        {
            const auto spr = C2D_SpriteSheetGetImage(sprites, sprites_arrow_up_idx);
            C2D_DrawImageAt(spr, (400.0f - spr.subtex->width)/2.0f, y, 0.5f, &arrow_tint);
        }
        if((std::min(-Equation::EQU_REGION_HEIGHT/2, l.render_res.min_y) + Equation::EQU_REGION_HEIGHT/2) < l.at_y)
        {
            const auto spr = C2D_SpriteSheetGetImage(sprites, sprites_arrow_down_idx);
            C2D_DrawImageAt(spr, (400.0f - spr.subtex->width)/2.0f, y + Equation::EQU_REGION_HEIGHT - spr.subtex->height, 0.5f, &arrow_tint);
        }
    };

    if(memory.size() >= 2)
    {
        draw_mem_at(*memory_tex[0], 0, memory[memory_scroll + 1]);
    }

    if(memory.size() >= 1)
    {
        draw_mem_at(*memory_tex[1], 120, memory[memory_scroll]);
    }

    if(selection == SelectionType::Memory)
    {
        if(memory.size() >= 2)
        {
            C2D_DrawRectSolid(0, (1 + (memory_scroll - memory_index)) * 120, 0.75f, 400, 120, COLOR_HIDE);
        }
    }
    else
    {
        C2D_DrawRectSolid(0, 0, 0.75f, 400, 240, COLOR_HIDE);
    }
}

void Keyboard::draw(C2D_SpriteSheet sprites) const
{
    C2D_DrawImageAt(C2D_Image{current_tex.get_tex(), &EQUATION_SUBTEX}, 0.0f, 0.0f, 0.0f);

    if(cursor_on && render_result.cursor_visible)
    {
        const int h = (render_result.cursor_y + 24) > Equation::EQU_REGION_HEIGHT ? (Equation::EQU_REGION_HEIGHT - render_result.cursor_y) : 24;
        C2D_DrawRectSolid(render_result.cursor_x, render_result.cursor_y, 0.25f, 2.0f, h, C2D_Color32(0,0,0,255));
    }

    if(auto t = osGetTime(); t - cursor_toggle_time > 300)
    {
        cursor_on = !cursor_on;
        cursor_toggle_time = t;
    }

    C2D_ImageTint arrow_tint;
    C2D_PlainImageTint(&arrow_tint, C2D_Color32(0, 0, 0, 128), 1.0f);
    if(render_result.w > 320 && at_x != (render_result.w - 320))
    {
        const auto spr = C2D_SpriteSheetGetImage(sprites, sprites_arrow_right_idx);
        C2D_DrawImageAt(spr, 320 - spr.subtex->width, (Equation::EQU_REGION_HEIGHT - spr.subtex->height) / 2.0f, 0.5f, &arrow_tint);
    }
    if(at_x != 0)
    {
        const auto spr = C2D_SpriteSheetGetImage(sprites, sprites_arrow_left_idx);
        C2D_DrawImageAt(spr, 0.0f, (Equation::EQU_REGION_HEIGHT - spr.subtex->height) / 2.0f, 0.5f, &arrow_tint);
    }

    if((std::max(Equation::EQU_REGION_HEIGHT/2, render_result.max_y) - Equation::EQU_REGION_HEIGHT/2) > at_y)
    {
        const auto spr = C2D_SpriteSheetGetImage(sprites, sprites_arrow_up_idx);
        C2D_DrawImageAt(spr, (320.0f - spr.subtex->width)/2.0f, 0, 0.5f, &arrow_tint);
    }
    if((std::min(-Equation::EQU_REGION_HEIGHT/2, render_result.min_y) + Equation::EQU_REGION_HEIGHT/2) < at_y)
    {
        const auto spr = C2D_SpriteSheetGetImage(sprites, sprites_arrow_down_idx);
        C2D_DrawImageAt(spr, (320.0f - spr.subtex->width)/2.0f, Equation::EQU_REGION_HEIGHT - spr.subtex->height, 0.5f, &arrow_tint);
    }

    C2D_DrawRectSolid(0, Equation::EQU_REGION_HEIGHT, 0.0f, 320, 1, C2D_Color32(0,0,0,255));

    C2D_DrawImageAt(C2D_Image{keyboard_tex.get_tex(), &KEYBOARD_SUBTEX}, 0, Equation::EQU_REGION_HEIGHT + 1, 0.0f);

    if(selection != SelectionType::Keyboard)
    {
        C2D_DrawRectSolid(0, Equation::EQU_REGION_HEIGHT + 1, 0.75f + 0.0625f, 320, 240 - 1 - Equation::EQU_REGION_HEIGHT, COLOR_HIDE);
    }
    
    if(selection != SelectionType::Equation)
    {
        C2D_DrawRectSolid(0, 0, 0.75f + 0.0625f, 320, Equation::EQU_REGION_HEIGHT, COLOR_HIDE);
    }
}
void Keyboard::draw_loader() const
{
    C2D_DrawSprite(&spinning_loader);
    C2D_SpriteRotateDegrees(&spinning_loader, -6.0f);
}

void Keyboard::select_next_type()
{
    selection = static_cast<SelectionType>((static_cast<int>(selection) + 1) % static_cast<int>(SelectionType::END));
}

bool Keyboard::calculating()
{
    if(wait_thread.load())
    {
        wait_thread.store(false);
        start_equation();
    }
    return calculating_flag.load();
}
void Keyboard::start_equation(bool save)
{
    if(current_eq && save)
    {
        if(memory.size() == MemorySize)
        {
            std::rotate(memory.begin(), memory.begin() + 1, memory.end());
        }
        else
        {
            memory.emplace_back();
        }

        memory.back() = {std::move(current_eq), std::move(result)};
        redo_top = true;
        redo_bottom = true;
        if(memory.size() == 1)
        {
            redo_top = false;
        }
        memory_index = 0;
        memory_scroll = 0;
    }

    result = Number{};
    current_eq = std::make_unique<Equation>();
    editing_part = 1;
    editing_char = 0;
    at_x = 0;
    at_y = 0;
    cursor_toggle_time = osGetTime();
    cursor_on = true;
    any_change = true;
    selection = SelectionType::Keyboard;
}
void Keyboard::start_calculating()
{
    calculating_flag.store(true);
    LightEvent_Signal(&wakeup);
    if(!calcThread)
    {
        calcThread = threadCreate(calculation_loop, this, 256 * 1024, 31, 1, false);
        svcSleepThread(10ULL * 1000ULL * 1000ULL);
    }
    C2D_SpriteSetRotationDegrees(&spinning_loader, 0.0f);
}
void Keyboard::stop_calculating()
{
    calculating_flag.store(false);
    wait_thread.store(true);
}
