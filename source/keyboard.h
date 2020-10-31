#ifndef INC_KEYBOARD_H
#define INC_KEYBOARD_H

#include <atomic>
#include <memory>
#include <vector>
#include <string_view>
#include <citro2d.h>
#include "equation.h"

struct MemoryLine {
    std::unique_ptr<Equation> equation;
    float previousAnswer;
    float result;
};

struct Keyboard {
    friend void calculation_loop(void* arg);

    Keyboard(C2D_SpriteSheet sprites);
    ~Keyboard();

    void handle_buttons(const u32 kDown);
    void handle_circle_pad(const int x, const int y);
    void handle_touch(const int x, const int y);

    void print_info();

    void update_equation(C2D_SpriteSheet sprites);
    void draw(C2D_SpriteSheet sprites) const;
    void draw_loader() const;
    bool calculating();

private:
    struct Menu {
        using ActionType = void(*)(Equation&, int&, int&);

        Menu* const parent;
        std::string_view name;
        ActionType func;
        std::vector<Menu> entries;

        size_t add(std::string_view submenu, ActionType f = nullptr);

        Menu(Menu* p, std::string_view n, ActionType f = nullptr) : parent(p), name(n), func(f) { }
    };

    static inline constexpr int MemorySize = 4;

    void start_equation(bool save = true);
    void start_calculating();
    void stop_calculating();

    std::atomic_bool calculating_flag;
    std::atomic_bool wait_thread;
    mutable C2D_Sprite spinning_loader;

    int editing_part;
    int editing_char;
    int at_x, at_y;
    Equation::RenderResult render_result;
    mutable u64 cursor_toggle_time;
    mutable bool cursor_on;
    bool any_change;
    bool in_equ;

    std::unique_ptr<Equation> current_eq;
    float previousAnswer;
    float result;
    std::vector<MemoryLine> memory;

    mutable Tex current_tex;
    mutable Tex memory_tex; // only one extra visible at once

    Thread calcThread;

    Menu root_menu;
    Menu* current_menu;
    int selected_entry;
};

#endif
