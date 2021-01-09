#ifndef INC_KEYBOARD_H
#define INC_KEYBOARD_H

#include <atomic>
#include <memory>
#include <vector>
#include <string_view>
#include <citro2d.h>
#include "equation.h"

struct Keyboard {
    Keyboard(C2D_SpriteSheet sprites);
    ~Keyboard();

    void handle_buttons(const u32 kDown, const u32 kDownRepeat);
    void handle_circle_pad(const int x, const int y);
    void handle_touch(const int x, const int y);

    void do_clears();
    void update_memory(C2D_SpriteSheet sprites);
    void update_equation(C2D_SpriteSheet sprites);
    void draw_memory(C2D_SpriteSheet sprites) const;
    void draw(C2D_SpriteSheet sprites) const;
    void draw_loader() const;
    bool calculating();

private:
    struct MemoryLine {
        std::shared_ptr<Equation> equation;
        Number result;
        Equation::RenderResult render_res;
        int at_x{}, at_y{};
    };
    enum class SelectionType : int {
        BottomScreen,
        TopScreen,

        END
    };
    void select_next_type();

    static inline constexpr int MemorySize = 12;

    static void calculation_loop(void* arg);

    void start_equation(bool save = true);
    void start_calculating();
    void stop_calculating();

    std::map<std::string, Number> variables;

    std::atomic_bool calculating_flag;
    LightEvent wait_thread;
    LightEvent kill_thread;
    LightEvent wakeup;
    mutable C2D_Sprite spinning_loader;

    int editing_part;
    int editing_char;
    int at_x, at_y;
    std::array<PartPos, 320 * Equation::EQU_REGION_HEIGHT> screen_data;
    Equation::RenderResult render_result;
    mutable u64 cursor_toggle_time;
    mutable bool cursor_on;
    bool any_change;
    bool error_eq;

    SelectionType selection;
    int selected_keyboard_screen;

    std::shared_ptr<Equation> current_eq;
    Number result;
    std::vector<MemoryLine> memory;

    mutable Tex current_tex;
    mutable std::array<std::unique_ptr<Tex>, 2> memory_tex;
    size_t memory_scroll;
    size_t memory_index;
    bool redo_top;
    bool redo_bottom;

    Thread calcThread;
};

#endif
