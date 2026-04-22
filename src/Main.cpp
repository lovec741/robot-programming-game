#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <set>
#include <utility>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

std::atomic<bool> running(true);

constexpr size_t HORIZONTAL_TILES_COUNT = 20;
constexpr size_t VERTICAL_TILES_COUNT = 14;
constexpr size_t TILE_WIDTH = 8;
constexpr size_t TILE_HEIGHT = 3;

constexpr size_t CANVAS_WIDTH = HORIZONTAL_TILES_COUNT * TILE_WIDTH;
constexpr size_t CANVAS_HEIGHT = VERTICAL_TILES_COUNT * TILE_HEIGHT;

constexpr char LINE_WALLS[][4] = {"┏", "┓", "┗", "┛", "━", "┃"};
constexpr char BLOCK_ART[TILE_HEIGHT][TILE_WIDTH][4] = {
    { " ", "╔", "═", "═", "═", "═", "╗", " " },
    { " ", "║", " ", " ", " ", " ", "║", " " },
    { " ", "╚", "═", "═", "═", "═", "╝", " " },
};
constexpr char ROBOT_UP_ART[TILE_HEIGHT][TILE_WIDTH][5] = {
    { " ", "┢", "━", "🯑", "🯒", "━", "┪", " " },
    { " ", "┃", " ", " ", " ", " ", "┃", " " },
    { " ", "┗", "━", "━", "━", "━", "┛", " " },
};
constexpr char ROBOT_DOWN_ART[TILE_HEIGHT][TILE_WIDTH][5] = {
    { " ", "┏", "━", "━", "━", "━", "┓", " " },
    { " ", "┃", " ", " ", " ", " ", "┃", " " },
    { " ", "┡", "━", "🯓", "🯐", "━", "┩", " " },
};
constexpr char ROBOT_RIGHT_ART[TILE_HEIGHT][TILE_WIDTH][5] = {
    { " ", "┏", "━", "━", "━", "━", "┱", "─" },
    { " ", "┃", " ", " ", " ", " ", " ", "🯛" },
    { " ", "┗", "━", "━", "━", "━", "┹", "─" },
};
constexpr char ROBOT_LEFT_ART[TILE_HEIGHT][TILE_WIDTH][5] = {
    { "─", "┲", "━", "━", "━", "━", "┓", " " },
    { "🯙", " ", " ", " ", " ", " ", "┃", " " },
    { "─", "┺", "━", "━", "━", "━", "┛", " " },
};
constexpr char WALL_ART_ODD[TILE_HEIGHT][TILE_WIDTH][4] = {
    { "░", "█", "█", "█", "█", "▒", "▒", "▒" },
    { "█", "░", "░", "░", "░", "▓", "▓", "▓" },
    { "▒", "▒", "▒", "█", "█", "█", "█", "░" },
};

constexpr char WALL_ART_EVEN[TILE_HEIGHT][TILE_WIDTH][4] = {
    { "▒", "▓", "▓", "▓", "▓", "░", "░", "░" },
    { "▓", "▒", "▒", "▒", "▒", "█", "█", "█" },
    { "░", "░", "░", "▓", "▓", "▓", "▓", "▒" },
};


constexpr char ANSI_CLEAR[] = "\033[2J\033[H";
constexpr char ANSI_HIDE_CURSOR[] = "\033[?25l";
constexpr char ANSI_SHOW_CURSOR[] = "\033[?25h";
constexpr char ANSI_RESET[] = "\033[0m";
constexpr char ANSI_BG_RED[] = "\033[41m";
constexpr char ANSI_BG_GREEN[] = "\033[42m";
constexpr char ANSI_BG_BLUE[] = "\033[44m";
constexpr char ANSI_BG_YELLOW[] = "\033[43m";

constexpr std::string_view CONTROLS_TEXT = "q = quit    w/s = forward/backward    a/d = turn left/right";


void set_raw_mode(bool enable) {
    static termios original;
    if (enable) {
        termios raw;
        tcgetattr(STDIN_FILENO, &original);
        raw = original;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &original);
    }
}

void get_terminal_size(size_t &rows, size_t &cols) {
    winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    rows = w.ws_row;
    cols = w.ws_col;
}

void ansi_move_cursor(size_t x, size_t y) {
    std::cout << "\033[" << y+1 << ";" << x+1 << "H";
}

enum class FlagColor {
    Red, Green, Blue, Yellow
};

enum class TileArt {
    Empty, RobotUp, RobotDown, RobotLeft, RobotRight, Wall, Coin
};

enum class TileInfo {
    Empty, Robot, Wall, Coin, Unknown
};

enum class Direction {
    Up, Right, Down, Left
};

enum class RobotAction {
    MoveForward, MoveBackward, TurnLeft, TurnRight
};

enum class RobotActionResult {
    Ok, Blocked, ActionAlreadyPending
};

class Tile {
public:
    TileArt art = TileArt::Empty;
    bool changed = false;
    std::set<FlagColor> flags{};
    TileInfo get_tile_info() const {
        switch (art)
        {
        case TileArt::Empty:
            return TileInfo::Empty;
        case TileArt::RobotUp:
        case TileArt::RobotDown:
        case TileArt::RobotLeft:
        case TileArt::RobotRight:
            return TileInfo::Robot;
        case TileArt::Wall:
            return TileInfo::Wall;
        case TileArt::Coin:
            return TileInfo::Coin;
        }
    };
    void change_art(TileArt new_art) {
        art = new_art;
        changed = true;
    }
    bool is_free() const {
        return art == TileArt::Empty;
    };
};


class Level {
protected:
    size_t width_;
    size_t height_;
    size_t coins_goal_;
    std::pair<size_t, size_t> initial_robot_xy_;
    Direction initial_robot_direction_;
    std::vector<std::vector<Tile>> tiles_;
    bool check_bounds(int x, int y) const {
        return x >= 0 && y >= 0 && x < (int)width() && y < (int)height();
    }
    bool is_free(int x, int y) const {
        return check_bounds(x, y) && tiles_[y][x].is_free();
    }
public:
    Level(size_t width, size_t height, size_t coins_goal, std::pair<size_t, size_t> initial_robot_xy, Direction initial_robot_direction) 
        : width_(width), height_(height), coins_goal_(coins_goal), initial_robot_xy_(initial_robot_xy), initial_robot_direction_(initial_robot_direction), tiles_{height, std::vector<Tile>(width)} {};
    virtual ~Level() = default;
    size_t width() const {return width_;};
    size_t height() const {return height_;};
    // std::vector<std::vector<Tile>> tiles() { return tiles_; };
    size_t coins_goal() const {return coins_goal_;};
    std::pair<size_t, size_t> initial_robot_xy() const {return initial_robot_xy_;};
    Direction initial_robot_direction() const  {return initial_robot_direction_;};
friend class Robot;
friend class Game;
};

class Level1 : public Level {
public:
    Level1() : Level(20, 14, 1, std::make_pair(4, 4), Direction::Right) {
        tiles_[5][5].art = TileArt::Wall;
        tiles_[6][6].art = TileArt::Wall;
        tiles_[7][6].art = TileArt::Wall;
    };
};

class Robot {
private:
    size_t x_;
    size_t y_;
    Direction direction_;
    std::shared_ptr<Level> level_;
    std::vector<std::vector<bool>> tile_visibility_{level_->height(), std::vector<bool>(level_->width(), true)};
    std::optional<RobotAction> action_;

    bool is_tile_visible(int radius, size_t x, size_t y) {
        int offset_x = x - x_;
        int offset_y = y - y_;
        int sign_offset_x = offset_x < 0 ? -1 : 1;
        int sign_offset_y = offset_y < 0 ? -1 : 1;

        auto is_cover = [&](size_t x, size_t y) -> bool {
            return !tile_visibility_[y][x] || !level_->is_free(x, y);
        };

        if (radius == 1) {
            if (offset_x*sign_offset_x == offset_y*sign_offset_y) { // corner
                return level_->is_free(x-sign_offset_x, y) || level_->is_free(x, y-sign_offset_y);
            }
            return true;
        }

        if (offset_x*sign_offset_x == offset_y*sign_offset_y) { // corner
            return !is_cover(x-sign_offset_x, y-sign_offset_y) && (level_->is_free(x-sign_offset_x, y) || level_->is_free(x, y-sign_offset_y));
        }
        if (offset_x == 0) {
            return !is_cover(x, y-sign_offset_y);
        }
        if (offset_y == 0) {
            return !is_cover(x-sign_offset_x, y);
        }
        if (offset_y*sign_offset_y == radius) {
            return !is_cover(x, y-sign_offset_y) && !is_cover(x-sign_offset_x, y-sign_offset_y);
        } else if (offset_x*sign_offset_x == radius) {
            return !is_cover(x-sign_offset_x, y) && !is_cover(x-sign_offset_x, y-sign_offset_y);
        }
        return true;
    }

    void refresh_visible_tiles() {
        // for (size_t y = 0; y < VERTICAL_TILES_COUNT; ++y) {
        //     for (size_t x = 0; x < HORIZONTAL_TILES_COUNT; ++x) {
        //         tile_visibility_[y][x] = true;
        //     }
        // }
        
        tile_visibility_[y_][x_] = true;
        int radius = 1;
        bool tile_in_radius_exists = true;
        auto check_visibility = [&](size_t work_x, size_t work_y) {
            if (level_->check_bounds(work_x, work_y)) {
                bool new_state = is_tile_visible(radius, work_x, work_y);
                if (new_state != tile_visibility_[work_y][work_x]) {
                    tile_visibility_[work_y][work_x] = new_state;
                    level_->tiles_[work_y][work_x].changed = true;
                }
                tile_in_radius_exists = true;
            }
        };
        while (tile_in_radius_exists) {
            tile_in_radius_exists = false;
            for (int offset = -radius; offset < radius; ++offset) {
                check_visibility(x_+offset, y_+radius);
                check_visibility(x_-offset, y_-radius);
                check_visibility(x_+radius, y_+offset+1);
                check_visibility(x_-radius, y_-offset-1);
            }
            ++radius;
        }
    }

    void handle_action() {
        if (action_ == RobotAction::MoveForward || action_ == RobotAction::MoveBackward) {
            auto [new_x, new_y] = get_xy_after_move(action_ == RobotAction::MoveForward);
            level_->tiles_[y_][x_].change_art(TileArt::Empty);
            x_ = new_x;
            y_ = new_y;
            level_->tiles_[y_][x_].change_art(get_tile_art_for_direction());
            refresh_visible_tiles();
        } else if (action_ == RobotAction::TurnLeft || action_ == RobotAction::TurnRight) {
            auto new_direction = get_direction_after_turn(action_ == RobotAction::TurnRight);
            direction_ = new_direction;
            level_->tiles_[y_][x_].change_art(get_tile_art_for_direction());
        }
        reset_action();
    }

    void step() {
        handle_action();
    }

    std::pair<int, int> get_xy_after_move(bool forward) {
        switch (direction_)
        {
        case Direction::Up:
            return std::make_pair(x_, y_+(forward ? -1 : 1));
        case Direction::Down:
            return std::make_pair(x_, y_+(forward ? 1 : -1));
        case Direction::Right:
            return std::make_pair(x_+(forward ? 1 : -1), y_);
        case Direction::Left:
            return std::make_pair(x_+(forward ? -1 : 1), y_);
        }
        throw std::runtime_error("Invalid direction");
    }

    Direction get_direction_after_turn(bool right) {
        switch (direction_)
        {
        case Direction::Up:
            return right ? Direction::Right : Direction::Left;
        case Direction::Down:
            return right ? Direction::Left : Direction::Right;
        case Direction::Right:
            return right ? Direction::Down : Direction::Up;
        case Direction::Left:
            return right ? Direction::Up : Direction::Down;
        }
        throw std::runtime_error("Invalid direction");
    }

    TileArt get_tile_art_for_direction() {
        switch (direction_)
        {
        case Direction::Up:
            return TileArt::RobotUp;
        case Direction::Down:
            return TileArt::RobotDown;
        case Direction::Right:
            return TileArt::RobotRight;
        case Direction::Left:
            return TileArt::RobotLeft;
        }
        throw std::runtime_error("Invalid direction");
    }
public:
    Robot(size_t x, size_t y, Direction direction, std::shared_ptr<Level> level) : x_(x), y_(y), direction_(direction), level_(std::move(level)) {
        level_->tiles_[y_][x_].change_art(get_tile_art_for_direction());
        refresh_visible_tiles();
    };

    RobotActionResult set_action(RobotAction new_action) {
        if (action_.has_value()) {
            return RobotActionResult::ActionAlreadyPending;
        }
        if (new_action == RobotAction::MoveForward || new_action == RobotAction::MoveBackward) {
            auto [new_x, new_y] = get_xy_after_move(new_action == RobotAction::MoveForward); 
            if (!level_->is_free(new_x, new_y)) {
                return RobotActionResult::Blocked;
            }
        }
        action_ = new_action;
        return RobotActionResult::Ok;
    }

    void reset_action() {
        action_ = std::nullopt;
    }
friend class Game;
};

class Game {
private:
    std::chrono::milliseconds step_speed_;
    size_t terminal_rows_ = 0;
    size_t terminal_cols_ = 0;
    std::shared_ptr<Level> level_;
    std::unique_ptr<Robot> robot_;
    size_t tick_ = 0;
    bool manual_control_ = false;
    bool canvas_too_small_ = false;
    size_t canvas_width_;
    size_t canvas_height_;
    bool running_ = true;

    /* updates terminal size variables, returns true if they changed */
    bool check_for_terminal_size_change() {
        size_t new_terminal_rows, new_terminal_cols;
        get_terminal_size(new_terminal_rows, new_terminal_cols);
        if (new_terminal_rows != terminal_rows_ || new_terminal_cols != terminal_cols_)  {
            terminal_rows_ = new_terminal_rows;
            terminal_cols_ = new_terminal_cols;
            return true;
        }
        return false;
    }

    void draw_border(size_t canvas_start_x, size_t canvas_start_y) {
        size_t border_start_x = canvas_start_x - 1;
        size_t border_start_y = canvas_start_y - 1;
        ansi_move_cursor(border_start_x, border_start_y);
        std::cout << LINE_WALLS[0];
        for (size_t i = 0; i < canvas_width_; i++)
        {
            std::cout << LINE_WALLS[4];
        }
        std::cout << LINE_WALLS[1];

        for (size_t i = 1; i <= canvas_height_; i++)
        {
            ansi_move_cursor(border_start_x, border_start_y+i);
            std::cout << LINE_WALLS[5];

            ansi_move_cursor(border_start_x+canvas_width_+1, border_start_y+i);
            std::cout << LINE_WALLS[5];
        }

        ansi_move_cursor(border_start_x, border_start_y+canvas_height_+1);
        std::cout << LINE_WALLS[2];
        for (size_t i = 0; i < canvas_width_; i++)
        {
            std::cout << LINE_WALLS[4];
        }
        std::cout << LINE_WALLS[3];

    }

    void draw_canvas_too_small() {
        std::cout << ANSI_CLEAR << "Please resize your window to make the canvas fit!";
        std::cout.flush();
    }

    void draw_controls_text() {
        ansi_move_cursor((terminal_cols_ - CONTROLS_TEXT.size()) / 2, terminal_rows_-2);
        std::cout << CONTROLS_TEXT;
    }

    void draw_tile(size_t canvas_start_x, size_t canvas_start_y, size_t x, size_t y, Tile& tile) {
        size_t offset_x = x*TILE_WIDTH;
        size_t offset_y = y*TILE_HEIGHT;
        bool has_flags = !tile.flags.empty();
        for (size_t inner_y = 0; inner_y < TILE_HEIGHT; inner_y++) {
            ansi_move_cursor(canvas_start_x+offset_x, canvas_start_y+offset_y+inner_y);
            for (size_t inner_x = 0; inner_x < TILE_WIDTH; inner_x++) {
                if (has_flags) {
                    size_t idx = inner_x % tile.flags.size();
                    FlagColor flag_color = *std::next(tile.flags.begin(), idx);
                    switch (flag_color)
                    {
                    case FlagColor::Red:
                        std::cout << ANSI_BG_RED;
                        break;
                    case FlagColor::Green:
                        std::cout << ANSI_BG_GREEN;
                        break;
                    case FlagColor::Blue:
                        std::cout << ANSI_BG_BLUE;
                        break;
                    case FlagColor::Yellow:
                        std::cout << ANSI_BG_YELLOW;
                        break;
                    };
                }
                if (!robot_->tile_visibility_[y][x]) {
                    srand((offset_x+inner_x)*canvas_width_+offset_y+inner_y);
                    std::cout << ((rand() % 2 == 0) ? "?" : "X");;
                } else {
                    switch (tile.art)
                    {
                    case TileArt::Empty:
                        srand((offset_x+inner_x)*canvas_width_+offset_y+inner_y);
                        std::cout << ((rand() % 17 == 0) ? 'v' : ' ');
                        break;
                    case TileArt::RobotUp:
                        std::cout << ROBOT_UP_ART[inner_y][inner_x];
                        break;
                    case TileArt::RobotDown:
                        std::cout << ROBOT_DOWN_ART[inner_y][inner_x];
                        break;
                    case TileArt::RobotLeft:
                        std::cout << ROBOT_LEFT_ART[inner_y][inner_x];
                        break;
                    case TileArt::RobotRight:
                        std::cout << ROBOT_RIGHT_ART[inner_y][inner_x];
                        break;
                    // case TileArt::Block:
                    //     std::cout << BLOCK_ART[tile_inner_y][tile_inner_x];
                    //     break;
                    case TileArt::Wall:
                        std::cout << (((x+y) % 2 == 0) ? WALL_ART_EVEN : WALL_ART_ODD)[inner_y][inner_x];
                        break;
                    }
                }
                
                
                if (has_flags) {
                    std::cout << ANSI_RESET;
                }
            }
        }
    }

    void draw(bool full_update) {
        size_t canvas_start_x = (terminal_cols_ - canvas_width_) / 2 - 3;
        size_t canvas_start_y = (terminal_rows_ - canvas_height_) / 2;
        if (full_update) {
            std::cout << ANSI_CLEAR;
            draw_border(canvas_start_x, canvas_start_y);
            draw_controls_text();
        }
        for (size_t y = 0; y < level_->height(); y++) {
            for (size_t x = 0; x < level_->width(); x++) {
                auto& tile = level_->tiles_[y][x];
                if (full_update || tile.changed) {
                    draw_tile(canvas_start_x, canvas_start_y, x, y, tile);
                    tile.changed = false;
                }
            }
        }
        std::cout.flush();
    }

    void handle_input() {
        char ch;
        if (read(STDIN_FILENO, &ch, 1) == 1) {
            if (ch == 'q') running_ = false;
            if (ch == ' ') {
                level_->tiles_[robot_->y_][robot_->x_].flags.emplace(FlagColor::Red);
                level_->tiles_[robot_->y_][robot_->x_].changed = true;
            }
            if (!manual_control_) return;
            if (ch == 'w' || ch == 'a' || ch == 's' || ch == 'd') {
                robot_->reset_action();
                switch (ch)
                {
                case 'w':
                    robot_->set_action(RobotAction::MoveForward);
                    break;
                case 's':
                    robot_->set_action(RobotAction::MoveBackward);
                    break;
                case 'a':
                    robot_->set_action(RobotAction::TurnLeft);
                    break;
                case 'd':
                    robot_->set_action(RobotAction::TurnRight);
                    break;
                }
            }
        }
    }

    void setup_terminal() {
        set_raw_mode(true);
        std::cout << ANSI_HIDE_CURSOR;
    }

    void cleanup_terminal() {
        std::cout << ANSI_SHOW_CURSOR;
        std::cout << ANSI_CLEAR;
        set_raw_mode(false);
    }
public:
    Game(size_t step_speed_ms = 100) : step_speed_(step_speed_ms) {}
    bool step() {
        if (!robot_ || !level_) {
            return false;
        }
        auto start_time = std::chrono::steady_clock::now();

        if (tick_ == 0) {
            setup_terminal();
        }
        
        handle_input();
        robot_->step();

        bool size_changed = check_for_terminal_size_change();
        if (size_changed) {
            if (terminal_rows_ < canvas_height_ + 5 || terminal_cols_ < canvas_width_ + 2) {
                canvas_too_small_ = true;
                draw_canvas_too_small();
            } else {
                canvas_too_small_ = false;
            }
        }

        if (!canvas_too_small_) {
            draw(size_changed);
        }

        if (!running_) {
            cleanup_terminal();
            return false;
        }
        
        auto elapsed_time = std::chrono::steady_clock::now() - start_time;
        if (elapsed_time < step_speed_) {
            std::this_thread::sleep_for(step_speed_ - elapsed_time);
        }
        return true;
    }

    void load_level(std::unique_ptr<Level> level) {
        level_ = std::move(level);
        canvas_width_ = level_->width()*TILE_WIDTH;
        canvas_height_ = level_->height()*TILE_HEIGHT;
        auto [initial_robot_x, initial_robot_y] = level_->initial_robot_xy();
        robot_ = std::make_unique<Robot>(initial_robot_x, initial_robot_y, level_->initial_robot_direction(), level_);
    }

    void run_with_manual_control() {
        manual_control_ = true;
        while (step()) {};
    }
};



int main() {
    auto g = Game();
    g.load_level(std::make_unique<Level1>());
    g.run_with_manual_control();
    
    return 0;
}