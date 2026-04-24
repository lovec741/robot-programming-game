#ifndef GAME_H_
#define GAME_H_

#include <memory>
#include <chrono>
#include "robot.h"
#include "levels.h"

constexpr std::string_view CONTROLS_TEXT = "q = quit";
constexpr std::string_view CONTROLS_WITH_MANUAL_TEXT = "q = quit    w/s = forward/backward    a/d = turn left/right    1/2/3/4 = toggle flag red/gree/blue/yellow";
constexpr std::string_view CANVAS_TOO_SMALL_TEXT = "Please resize your window to make the canvas fit!";
constexpr std::string_view WON_TEXT = "Congratulations you have finished the level!";
constexpr std::string_view WON_WITH_MANUAL_TEXT = "Congratulations you have finished the level! Now try solving the level without manual control :)";

class Game {
private:
    std::chrono::milliseconds step_speed_;
    size_t terminal_rows_ = 0;
    size_t terminal_cols_ = 0;
    std::shared_ptr<Level> level_;
    std::shared_ptr<Robot> robot_;
    size_t tick_ = 0;
    bool manual_control_ = false;
    bool canvas_too_small_ = false;
    size_t canvas_width_;
    size_t canvas_height_;
    bool running_ = true;
    bool win_screen_shown_ = false;

    /* updates terminal size variables, returns true if they changed */
    bool check_for_terminal_size_change();
    void draw_border(size_t start_x, size_t start_y, size_t width, size_t height);
    void draw_canvas_too_small();
    void draw_controls_text();
    void draw_won_text();
    void draw_tile(size_t canvas_start_x, size_t canvas_start_y, size_t x, size_t y, Tile& tile);
    void draw(bool full_update);
    void handle_input();
    void setup_terminal();
    void cleanup_terminal();
public:
    Game(size_t step_time_ms = 100);
    bool step();
    void load_level(size_t level_number);
    void run_with_manual_control();
    std::shared_ptr<Robot> robot() {
        return robot_;
    }
    std::shared_ptr<Level> level() {
        return level_;
    }
};

#endif
