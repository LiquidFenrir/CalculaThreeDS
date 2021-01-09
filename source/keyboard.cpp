#include "keyboard.h"
#include "text.h"
#include "sprites.h"
#include "colors.h"
#include <algorithm>
#include <cmath>
#include <array>
struct KeyboardScreen {
    enum class Type : int {
        Basic,
        Functions,
        Variables,

        END
    };

    static void prev(int& v)
    {
        v = (v + static_cast<int>(Type::END) - 1) % static_cast<int>(Type::END);
    }
    static void next(int& v)
    {
        v = (v + 1) % static_cast<int>(Type::END);
    }

    constexpr static inline int W = 5;
    constexpr static inline int H = 4;
    std::string_view name;
    std::array<std::array<std::string_view, KeyboardScreen::W>, KeyboardScreen::H> buttons;
    struct Action {
        using F_t = int(*)(Equation&, int&, int&);
        constexpr Action() : f(nullptr) { }
        constexpr Action& operator=(const F_t& f_) { f = f_; return *this; }

        constexpr int operator()(Equation& e, int& cur_char, int& cur_part) const
        {
            if(f)
            {
                return f(e, cur_char, cur_part);
            }
            return 0;
        }
        constexpr operator bool() const
        {
            return f != nullptr;
        }
private:
        F_t f;
    };
    std::array<std::array<Action, KeyboardScreen::W>, KeyboardScreen::H> actions;
};

static constexpr int EQU_GAP_END = (240 - (KeyboardScreen::H * 36));
static constexpr int EQU_GAP_HEIGHT = EQU_GAP_END - Equation::EQU_REGION_HEIGHT;

static int complete_equ(Equation& e, int& cur_char, int& cur_part)
{
    return -1;
}
static int add_division(Equation& e, int& cur_char, int& cur_part)
{
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
    return 1;
}
static int add_exponent(Equation& e, int& cur_char, int& cur_part)
{
    e.add_part_at(cur_part, cur_char, Part::Specialty::Exponent, Part::Position::Start); // add exponent start at location
    const auto assoc_s = cur_part;
    e.add_part_at(cur_part, cur_char); // add text area after exponent start
    const auto cp_part = cur_part;
    const auto cp_char = cur_char;
    // if location was right before an exponent start, we still have one right after current added part.
    // thus, we need to add an empty text area after its end, then add the current exponent's end after that
    if(const auto m = e.parts[e.parts[cur_part].meta.next].meta; m.special == Part::Specialty::Exponent && m.position == Part::Position::Start)
    {
        cur_part = m.assoc;
        cur_char = 0; // we're on a control chunk, length is 0
        e.add_part_at(cur_part, cur_char); // add text area after exponent start
    }
    const auto [have_any_before, have_any_after] = e.add_part_at(cur_part, cur_char, Part::Specialty::Exponent, Part::Position::End, assoc_s);
    e.parts[assoc_s].meta.assoc = cur_part;
    if(!have_any_after)
    {
        e.add_part_at(cur_part, cur_char);
    }
    cur_part = cp_part;
    cur_char = cp_char;
    return 1;
}
static void add_paren(Equation& e, int& cur_char, int& cur_part, const Part::Position direction)
{
    const auto [have_any_before, have_any_after] = e.add_part_at(cur_part, cur_char, Part::Specialty::TempParen, direction);
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
        e.find_matching_tmp_paren(cp_part);
    }
}
static int add_start_paren(Equation& e, int& cur_char, int& cur_part)
{
    add_paren(e, cur_char, cur_part, Part::Position::Start);
    return 1;
}
static int add_end_paren(Equation& e, int& cur_char, int& cur_part)
{
    add_paren(e, cur_char, cur_part, Part::Position::End);
    return 1;
}
static int add_exponential(Equation& e, int& cur_char, int& cur_part)
{
    e.parts[cur_part].value.insert(cur_char, "e");
    cur_char++;
    add_exponent(e, cur_char, cur_part);
    return 1;
}
static void add_special(Equation& e, int& cur_char, int& cur_part, const Part::Specialty special)
{
    if(e.set_special(cur_part, cur_char, special) == 0)
    {
        e.add_part_at(cur_part, cur_char, special, Part::Position::Start);
        const auto assoc_s = cur_part;
        e.add_part_at(cur_part, cur_char);
        const auto cp_part = cur_part;
        const auto cp_char = cur_char;
        const auto [have_any_before, have_any_after] = e.add_part_at(cur_part, cur_char, special, Part::Position::End, assoc_s);
        e.parts[assoc_s].meta.assoc = cur_part;
        if(!have_any_after)
        {
            e.add_part_at(cur_part, cur_char);
        }
        cur_part = cp_part;
        cur_char = cp_char;
    }
    else
    {
        cur_part = e.parts[e.parts[cur_part].meta.next].meta.next;
        cur_char = 0;
    }
}
static int add_absolute(Equation& e, int& cur_char, int& cur_part)
{
    add_special(e, cur_char, cur_part, Part::Specialty::Absolute);
    return 1;
}
static int add_root(Equation& e, int& cur_char, int& cur_part)
{
    add_special(e, cur_char, cur_part, Part::Specialty::Root);
    return 1;
}
static int remove_char(Equation& e, int& cur_char, int& cur_part)
{
    return e.remove_at(cur_part, cur_char);
}
static constexpr auto keyboard_screens = []() constexpr -> std::array<KeyboardScreen, int(KeyboardScreen::Type::END)> {
    std::array<KeyboardScreen, int(KeyboardScreen::Type::END)> out;
    using P_t = std::pair<std::string_view, KeyboardScreen::Action::F_t>;
    constexpr P_t empty_button{{}, nullptr};

    #define ADD_CHAR_F(v) [](Equation& e, int& cur_char, int& cur_part) -> int { \
        e.parts[cur_part].value.insert(cur_char, v); \
        cur_char += std::size(v) - 1; /* dont count NUL */ \
        return 1; \
    }
    #define ADD_CHAR(v) {v, ADD_CHAR_F(v)}

    #define DO_PART(v, n, d) { \
        v.name = n; \
        for(int y = 0; y < KeyboardScreen::H; ++y) for(int x = 0; x < KeyboardScreen::W; ++x) \
        { \
            const auto [s, f] = d[y][x]; \
            v.buttons[y][x] = s; \
            v.actions[y][x] = f; \
        } \
    }

    constexpr std::array<std::array<P_t, KeyboardScreen::W>, KeyboardScreen::H> basic_data{{
        {{{"(", add_start_paren}, ADD_CHAR("1"), ADD_CHAR("2"), ADD_CHAR("3"), ADD_CHAR("+")}},
        {{{")", add_end_paren}, ADD_CHAR("4"), ADD_CHAR("5"), ADD_CHAR("6"), ADD_CHAR("-")}},
        {{{"^", add_exponent}, ADD_CHAR("7"), ADD_CHAR("8"), ADD_CHAR("9"), ADD_CHAR("*")}},
        {{{"del", remove_char}, ADD_CHAR("."), ADD_CHAR("0"), {"=", complete_equ}, {"/", add_division}}},
    }};

    auto& basic = out[int(KeyboardScreen::Type::Basic)];
    DO_PART(basic, "basic", basic_data)

    constexpr std::array<std::array<P_t, KeyboardScreen::W>, KeyboardScreen::H> funcs_data{{
        {{ADD_CHAR("cos"), ADD_CHAR("sin"), ADD_CHAR("tan"), {"exp", add_exponential}, {"abs", add_absolute}}},
        {{ADD_CHAR("acos"), ADD_CHAR("asin"), ADD_CHAR("atan"), ADD_CHAR("ln"), {"sqrt", add_root}}},
        {{ADD_CHAR("cot"), ADD_CHAR("sec"), ADD_CHAR("csc"), ADD_CHAR("log"), empty_button}},
        {{empty_button, empty_button, empty_button, empty_button, empty_button}},
    }};
    auto& funcs = out[int(KeyboardScreen::Type::Functions)];
    DO_PART(funcs, "functions", funcs_data)

    constexpr std::array<std::array<P_t, KeyboardScreen::W>, KeyboardScreen::H> vars_data{{
        {{ADD_CHAR("a"), {"pi", ADD_CHAR_F("P")}, ADD_CHAR("k"), ADD_CHAR("f"), ADD_CHAR("x")}},
        {{ADD_CHAR("b"), ADD_CHAR("ans"), ADD_CHAR("l"), ADD_CHAR("g"), ADD_CHAR("y")}},
        {{ADD_CHAR("c"), ADD_CHAR("i"), ADD_CHAR("m"), ADD_CHAR("h"), ADD_CHAR("z")}},
        {{ADD_CHAR("d"), ADD_CHAR("j"), ADD_CHAR("n"), empty_button, ADD_CHAR(">")}},
    }};
    auto& vars = out[int(KeyboardScreen::Type::Variables)];
    DO_PART(vars, "variables", vars_data)

    #undef DO_PART

    #undef ADD_CHAR
    #undef ADD_CHAR_F

    return out;
}();

void Keyboard::calculation_loop(void* arg)
{
    auto kb = static_cast<Keyboard*>(arg);
    while(!LightEvent_TryWait(&kb->kill_thread))
    {
        LightEvent_Wait(&kb->wakeup);
        if(LightEvent_TryWait(&kb->kill_thread)) break;

        kb->current_eq->optimize();

        if(kb->current_eq->parts.size() == 3 && kb->current_eq->parts[1].value.empty()) // if the thing to calculate is empty
        {
            if(!kb->memory.empty()) // and if there is a memory, reuse it and do the same calculation
            {
                kb->current_eq = kb->memory.back().equation;
            }
            else // just don't do anything
            {
                kb->error_eq = true;
                kb->stop_calculating();
                continue;
            }
        }

        std::tie(kb->result, kb->error_eq) = kb->current_eq->calculate(kb->variables, kb->editing_part, kb->editing_char);

        kb->stop_calculating();
    }
}

#define REMOVE_ODD(v) (((v) & ~1) + (((v) & 1) << 1))
#define MK_SUBTEX(w, h, w2, h2) {(w), REMOVE_ODD(h), 0.0f, 1.0f, ((w)/(w2)), 1.0f - (REMOVE_ODD(h)/(h2))}
#define MK_SUBTEX_OFFSET(w, h, w2, h2, oX, oY) {(w), REMOVE_ODD(h), ((oX)/(w2)), 1.0f - (REMOVE_ODD(oY)/(h2)), ((w)/(w2)), 1.0f - (REMOVE_ODD(oY)/(h2)) - (REMOVE_ODD(h)/(h2))}
static constexpr Tex3DS_SubTexture EQUATION_SUBTEX = MK_SUBTEX(320, Equation::EQU_REGION_HEIGHT, 512.0f, 128.0f);
static constexpr Tex3DS_SubTexture EQUATION_MEM_SUBTEX = MK_SUBTEX_OFFSET(400, Equation::EQU_REGION_HEIGHT, 512.0f, 128.0f, 0, 0);
static constexpr Tex3DS_SubTexture ANSWER_MEM_SUBTEX = MK_SUBTEX_OFFSET(400, 120 - Equation::EQU_REGION_HEIGHT, 512.0f, 128.0f, 0, Equation::EQU_REGION_HEIGHT);
#undef MK_SUBTEX_OFFSET
#undef MK_SUBTEX
#undef REMOVE_ODD
static C2D_ImageTint YES_TINT, NO_TINT, TEXT_YES_TINT, TEXT_NO_TINT;

Keyboard::Keyboard(C2D_SpriteSheet sprites)
:
calculating_flag(false),
selected_keyboard_screen(int(KeyboardScreen::Type::Basic)),
current_tex(512, 128, GPU_RGBA8),
memory_scroll(0), memory_index(0),
redo_top(false), redo_bottom(false),
calcThread(nullptr)
{
    LightEvent_Init(&wakeup, RESET_ONESHOT);
    LightEvent_Init(&kill_thread, RESET_ONESHOT);
    LightEvent_Init(&wait_thread, RESET_STICKY);

    for(auto& p : memory_tex)
    {
        p = std::make_unique<Tex>(512, 128, GPU_RGBA8);
    }

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
        LightEvent_Signal(&kill_thread);
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
    else if(selection == SelectionType::BottomScreen)
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
        else if(kDown & KEY_L)
        {
            KeyboardScreen::prev(selected_keyboard_screen);
        }
        else if(kDown & KEY_R)
        {
            KeyboardScreen::next(selected_keyboard_screen);
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
    else if(selection == SelectionType::TopScreen)
    {
        if(kDown & KEY_A)
        {
            if(memory.size() >= 1)
            {
                if(current_eq->parts[0].meta.next == current_eq->parts[current_eq->parts[0].meta.assoc].meta.before && current_eq->parts[current_eq->parts[0].meta.next].value.empty()) // if the current equation is empty
                {
                    start_equation(false, memory[memory_index].equation.get());
                }
            }
        }
        else if(kDownRepeat & KEY_DUP)
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
    const auto do_scroll = [](int& at_x, int& at_y, const int x, const int y,const int w, const Equation::RenderResult& render_result, bool& any_change) -> void {
        if(x < -20 && at_x != 0)
        {
            at_x -= 2;
            any_change = true;
        }
        else if(x > 20 && (render_result.w > w && at_x < (render_result.w - w)))
        {
            at_x += 2;
            any_change = true;
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
    };

    if(selection == SelectionType::BottomScreen)
    {
        do_scroll(at_x, at_y, x, y, 320, render_result, any_change);
    }
    else if(selection == SelectionType::TopScreen)
    {
        if(!memory.empty())
        {
            auto& line = memory[memory.size() - memory_index - 1];
            do_scroll(line.at_x, line.at_y, x, y, 400, line.render_res, memory_index == memory_scroll ? redo_bottom : redo_top);
        }
    }
}
void Keyboard::handle_touch(const int x, const int y)
{
    if(y < Equation::EQU_REGION_HEIGHT)
    {
        const auto& pos = screen_data[x + y * 320];
        if(pos.part != -1 && pos.pos != -1 && (pos.part != editing_part || pos.pos != editing_char))
        {
            editing_part = pos.part;
            editing_char = pos.pos;
            any_change = true;
        }
    }
    else if(Equation::EQU_REGION_HEIGHT <= y && y < EQU_GAP_END)
    {
        if(x < 10)
        {
            KeyboardScreen::prev(selected_keyboard_screen);
        }
        else if(310 <= x)
        {
            KeyboardScreen::next(selected_keyboard_screen);
        }
    }
    else
    {
        const int by = (y - EQU_GAP_END)/36;
        const int bx = x/64;
        const auto& scr = keyboard_screens[selected_keyboard_screen];
        const int r = scr.actions[by][bx](*current_eq, editing_char, editing_part);
        if(r < 0)
        {
            start_calculating();
        }
        else
        {
            any_change = r;
        }
    }
}

void Keyboard::do_clears()
{
    if(any_change)
    {
        auto t = current_tex.get_target();
        C2D_TargetClear(t, COLOR_TRANSPARENT);
    }
    if(redo_top)
    {
        auto t = memory_tex[0]->get_target();
        C2D_TargetClear(t, COLOR_TRANSPARENT);
    }
    if(redo_bottom)
    {
        auto t = memory_tex[1]->get_target();
        C2D_TargetClear(t, COLOR_TRANSPARENT);
    }
}

void Keyboard::update_memory(C2D_SpriteSheet sprites)
{
    if(redo_top)
    {
        redo_top = false;
        auto t = memory_tex[0]->get_target();
        C2D_SceneBegin(t);
        auto& mem_line = memory[memory.size() - memory_scroll - 2];
        mem_line.render_res = mem_line.equation->render_memory(mem_line.at_x, mem_line.at_y, sprites);
        mem_line.result.render(sprites);
    }
    if(redo_bottom)
    {
        redo_bottom = false;
        auto t = memory_tex[1]->get_target();
        C2D_SceneBegin(t);
        auto& mem_line = memory[memory.size() - memory_scroll - 1];
        mem_line.render_res = mem_line.equation->render_memory(mem_line.at_x, mem_line.at_y, sprites);
        mem_line.result.render(sprites);
    }
}
void Keyboard::update_equation(C2D_SpriteSheet sprites)
{
    if(any_change)
    {
        any_change = false;
        auto t = current_tex.get_target();
        C2D_SceneBegin(t);
        render_result = current_eq->render_main(at_x, at_y, editing_part, editing_char, sprites, screen_data.data());
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

        if(l.render_res.w > 400 && l.at_x < (l.render_res.w - 400))
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
        draw_mem_at(*memory_tex[0], 0, memory[memory.size() - memory_scroll - 2]);
    }

    if(memory.size() >= 1)
    {
        draw_mem_at(*memory_tex[1], 120, memory[memory.size() - memory_scroll - 1]);
    }

    if(selection == SelectionType::TopScreen)
    {
        if(memory.size() >= 2)
        {
            C2D_DrawRectSolid(0, (memory_index - memory_scroll) * 120, 0.75f, 400, 120, COLOR_HIDE);
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
    if(render_result.w > 320 && at_x < (render_result.w - 320))
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

    C2D_DrawRectSolid(0, Equation::EQU_REGION_HEIGHT, 0.0f, 320, EQU_GAP_HEIGHT, COLOR_BLACK);


    const auto& scr = keyboard_screens[selected_keyboard_screen];
   
    C2D_Image button_img = C2D_SpriteSheetGetImage(sprites, sprites_keyboard_button_idx);
    C2D_ImageTint button_tint;
    C2D_PlainImageTint(&button_tint, COLOR_BLUE, 1.0f);
    for(int y = 0, py = EQU_GAP_END; y < KeyboardScreen::H; ++y, py += 36)
    {
        for(int x = 0, px = 0; x < KeyboardScreen::W; ++x, px += 64)
        {
            if(scr.actions[y][x])
            {
                const auto txtmap = TextMap::char_to_sprite->menu.at(scr.buttons[y][x]);
                C2D_DrawImageAt(button_img, px, py, 0.0f, &button_tint);
                auto dx = px + (64 - txtmap.width)/2;
                for(const auto& img : txtmap.sprites)
                {
                    C2D_DrawImageAt(img, dx, py + (36 - img.subtex->height)/2, 0.25f, &button_tint);
                    dx += img.subtex->width;
                }
            }
        }
    }

    C2D_ImageTint text_tint;
    C2D_PlainImageTint(&text_tint, COLOR_WHITE, 1.0f);
    const int w = (320 - (scr.name.size() * 13))/2;
    int x = w;
    for(const char c : scr.name)
    {
        const auto img = TextMap::char_to_sprite->equ.at(std::string_view(&c, 1));
        C2D_DrawImageAt(img, x, Equation::EQU_REGION_HEIGHT + (EQU_GAP_HEIGHT - img.subtex->height)/2, 0.5f, &text_tint);
        x += 13;
    }

    C2D_PlainImageTint(&arrow_tint, COLOR_GRAY, 1.0f);
    const auto arrow_left = C2D_SpriteSheetGetImage(sprites, sprites_arrow_left_idx);
    const auto arrow_right = C2D_SpriteSheetGetImage(sprites, sprites_arrow_right_idx);
    C2D_DrawImageAt(arrow_left, 2, Equation::EQU_REGION_HEIGHT + (EQU_GAP_HEIGHT - arrow_left.subtex->height)/2, 0.5f, &arrow_tint);
    C2D_DrawImageAt(arrow_right, 320 - 2 - arrow_right.subtex->width, Equation::EQU_REGION_HEIGHT + (EQU_GAP_HEIGHT - arrow_right.subtex->height)/2, 0.5f, &arrow_tint);


    if(selection != SelectionType::BottomScreen)
    {
        C2D_DrawRectSolid(0, 0, 0.75f + 0.0625f, 320, 240, COLOR_HIDE);
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
    if(LightEvent_TryWait(&wait_thread))
    {
        LightEvent_Clear(&wait_thread);
        if(!error_eq) start_equation();
    }
    return calculating_flag.load();
}
void Keyboard::start_equation(bool save, const Equation* to_copy)
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

        memory.back() = {current_eq, std::move(result)};
        variables.insert_or_assign("ans", memory.back().result);
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
    current_eq = to_copy ? std::make_shared<Equation>(*to_copy) : std::make_shared<Equation>();
    editing_part = 1;
    editing_char = 0;
    at_x = 0;
    at_y = 0;
    cursor_toggle_time = osGetTime();
    cursor_on = true;
    any_change = true;
    error_eq = false;
    selection = SelectionType::BottomScreen;
}
void Keyboard::start_calculating()
{
    error_eq = false;
    calculating_flag.store(true);
    if(!calcThread)
    {
        calcThread = threadCreate(calculation_loop, this, 256 * 1024, 31, 1, false);
        svcSleepThread(10ULL * 1000ULL * 1000ULL);
    }
    LightEvent_Signal(&wakeup);
    C2D_SpriteSetRotationDegrees(&spinning_loader, 0.0f);
}
void Keyboard::stop_calculating()
{
    calculating_flag.store(false);
    LightEvent_Signal(&wait_thread);
}
