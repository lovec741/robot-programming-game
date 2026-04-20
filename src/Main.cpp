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

void ansi_move_cursor(size_t row, size_t column) {
    std::cout << "\033[" << row << ";" << column << "H";
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
    bool is_free() const {
        return art == TileArt::Empty;
    };
};


class Level {
private:
    size_t height_;
    size_t width_;
    std::vector<std::vector<Tile>> tiles_{height_, std::vector<Tile>(width_)};
    size_t coins_goal_;
    size_t coins_collected_;
    bool check_bounds(int x, int y) const {
        return x >= 0 && y >= 0 && x < width_ && y < height_;
    }
    bool is_free(int x, int y) const {
        return check_bounds(x, y) && tiles_[y][x].is_free();
    }
public:
    size_t height() const { return height_; };
    size_t width() const { return width_; };
    std::vector<std::vector<Tile>> tiles() { return tiles_; };
    size_t coins_goal() const { return coins_goal_; };
    size_t coins_collected() const { return coins_collected_; };
    
friend class Robot;
};


class Robot {
private:
    size_t x_;
    size_t y_;
    Direction direction_;
    std::unique_ptr<Level> level_;
    std::vector<std::vector<bool>> tile_visibility_{level_->height_, std::vector<bool>(level_->width_, true)};
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
        size_t work_x, work_y;
        while (tile_in_radius_exists) {
            tile_in_radius_exists = false;
            for (int offset = -radius; offset < radius; ++offset) {
                work_y = y_+radius;
                work_x = x_+offset;
                if (level_->check_bounds(work_x, work_y)) {
                    tile_visibility_[work_y][work_x] = is_tile_visible(radius, work_x, work_y);
                    tile_in_radius_exists = true;
                }
                work_y = y_-radius;
                work_x = x_-offset;
                if (level_->check_bounds(work_x, work_y)) {
                    tile_visibility_[work_y][work_x] = is_tile_visible(radius, work_x, work_y);
                    tile_in_radius_exists = true;
                }
                work_y = y_+offset+1;
                work_x = x_+radius;
                if (level_->check_bounds(work_x, work_y)) {
                    tile_visibility_[work_y][work_x] = is_tile_visible(radius, work_x, work_y);
                    tile_in_radius_exists = true;
                }
                work_y = y_-offset-1;
                work_x = x_-radius;
                if (level_->check_bounds(work_x, work_y)) {
                    tile_visibility_[work_y][work_x] = is_tile_visible(radius, work_x, work_y);
                    tile_in_radius_exists = true;
                }
            }
            ++radius;
        }
    }

    void handle_action() {
        if (action_ == RobotAction::MoveForward || action_ == RobotAction::MoveBackward) {
            auto [new_x, new_y] = get_xy_after_move(action_ == RobotAction::MoveForward);
            level_->tiles()[y_][x_].changed = true;
            level_->tiles()[y_][x_].art = TileArt::Empty;
            x_ = new_x;
            y_ = new_y;
            level_->tiles()[y_][x_].changed = true;
            level_->tiles()[y_][x_].art = get_tile_art_for_direction();
            refresh_visible_tiles();
        } else if (action_ == RobotAction::TurnLeft || action_ == RobotAction::TurnRight) {
            auto new_direction = get_direction_after_turn(action_ == RobotAction::TurnRight);
            direction_ = new_direction;
            level_->tiles()[y_][x_].changed = true;
            level_->tiles()[y_][x_].art = get_tile_art_for_direction();
        }
    }

    void step() {
        // handle movement
        // recalculate visible tiles
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
    }
public:
    RobotActionResult set_action(RobotAction new_action) {
        if (action_ != std::nullopt) {
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
    std::unique_ptr<Robot> robot_;
    size_t tick_ = 0;
    bool manual_control_ = false;
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

    void draw_gui() {
        
    }

    void draw(bool do_patial_updates = true) {

    }

    void handle_input() {
        char ch;
        if (read(STDIN_FILENO, &ch, 1) == 1) {
            if (ch == 'q') running_ = false;
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
        auto start_time = std::chrono::steady_clock::now();
        if (tick_ == 0) {
            setup_terminal();
        }
        handle_input();
        robot_->step();
        bool size_changed = check_for_terminal_size_change();
        draw(!size_changed);
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
};



int main() {
    

    std::vector<std::vector<size_t>> tiles(VERTICAL_TILES_COUNT, std::vector<size_t>(HORIZONTAL_TILES_COUNT, 0));
    // tiles[4][4] = 6;
    tiles[4][5] = 6;
    // tiles[5][5] = 6;
    tiles[6][5] = 6;
    tiles[4][7] = 6;
    // tiles[5][7] = 6;
    tiles[6][7] = 6;
    tiles[6][10] = 6;
    tiles[6][11] = 6;
    tiles[6][12] = 6;
    tiles[6][13] = 6;
    size_t current_x = 6, current_y = 6;
    tiles[current_x][current_y] = 4;

    std::vector<std::vector<bool>> visible_tiles(VERTICAL_TILES_COUNT, std::vector<bool>(HORIZONTAL_TILES_COUNT, true));
    
    
    recalculate_visible_tiles(current_x, current_y, visible_tiles, tiles);
    size_t i = 0;
    while (running) {
        std::cout << "\033[H"; // move cursor to top-left
        size_t rows, cols;
        get_terminal_size(rows, cols);
        if (rows < CANVAS_HEIGHT + 2 || cols < CANVAS_WIDTH + 2) {
            std::cout << "Please resize your window to make the canvas fit!";
        } else {
            size_t canvas_start_x = (cols - CANVAS_WIDTH) / 2 - 1;
            size_t canvas_start_y = (rows - CANVAS_HEIGHT) / 2 - 1;

            if (i % 10 == 0) {
                for (size_t r = 0; r < rows; ++r) {
                    for (size_t c = 0; c < cols; ++c) {
                        // std::cout << (((r+c) % 2 == 0) ? fill1 : fill2);
                        if (c < canvas_start_x || r < canvas_start_y || c > canvas_start_x + CANVAS_WIDTH + 1 || r > canvas_start_y + CANVAS_HEIGHT + 1) {
                            std::cout << ".";
                            continue;
                        }
                        size_t x = c - canvas_start_x;
                        size_t y = r - canvas_start_y;
                        if (y == 0 && x == 0) {
                            std::cout << LINE_WALLS[0];
                        } else if (y == 0 && x == CANVAS_WIDTH+1) {
                            std::cout << LINE_WALLS[1];
                        } else if (y == CANVAS_HEIGHT+1 && x == 0) {
                            std::cout << LINE_WALLS[2];
                        } else if (y == CANVAS_HEIGHT+1 && x == CANVAS_WIDTH+1) {
                            std::cout << LINE_WALLS[3];
                        } else if ((y == 0 || y == CANVAS_HEIGHT+1) && x < CANVAS_WIDTH+1) {
                            std::cout << LINE_WALLS[4];
                        } else if ((x == 0 || x == CANVAS_WIDTH+1) && y < CANVAS_HEIGHT+1) {
                            std::cout << LINE_WALLS[5];
                        } else {
                            x--;
                            y--;
                            size_t tile_x = x / TILE_WIDTH;
                            size_t tile_y = y / TILE_HEIGHT;
                            size_t tile_inner_x = x % TILE_WIDTH;
                            size_t tile_inner_y = y % TILE_HEIGHT;
                            std::set<FlagColor> flag_colors = {FlagColor::Red, FlagColor::Blue, FlagColor::Green, FlagColor::Yellow};
                            if (tile_x == 3 && tile_y == 3) {
                                // if (tile_inner_y == 0 || tile_inner_y == TILE_HEIGHT-1) {
                                    // if (tile_inner_x == 0) {
                                    size_t idx = tile_inner_x % flag_colors.size();
                                    FlagColor flag_color = *std::next(flag_colors.begin(), idx);
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
                                    // }
                                // } else if (tile_inner_x == 0 || tile_inner_x == TILE_WIDTH-1) {
                                    // std::cout << ANSI_BG_RED;
                                // }
                            }
                            if (!visible_tiles[tile_y][tile_x]) {
                                srand(x*HORIZONTAL_TILES_COUNT+y);
                                std::cout << ((rand() % 2 == 0) ? "?" : "X");;
                            } else {
                                switch (tiles[tile_y][tile_x])
                                {
                                case 0:
                                    srand(x*HORIZONTAL_TILES_COUNT+y);
                                    std::cout << ((rand() % 17 == 0) ? 'v' : ' ');
                                    break;
                                case 1:
                                    std::cout << ROBOT_UP_ART[tile_inner_y][tile_inner_x];
                                    break;
                                case 2:
                                    std::cout << ROBOT_DOWN_ART[tile_inner_y][tile_inner_x];
                                    break;
                                case 3:
                                    std::cout << ROBOT_LEFT_ART[tile_inner_y][tile_inner_x];
                                    break;
                                case 4:
                                    std::cout << ROBOT_RIGHT_ART[tile_inner_y][tile_inner_x];
                                    break;
                                case 5:
                                    std::cout << BLOCK_ART[tile_inner_y][tile_inner_x];
                                    break;
                                case 6:
                                    std::cout << (((tile_x+tile_y) % 2 == 0) ? WALL_ART_EVEN : WALL_ART_ODD)[tile_inner_y][tile_inner_x];
                                    break;
                                }
                            }
                            
                            
                            if (tile_x == 3 && tile_y == 3) {
                                // if (tile_inner_y == 0 || tile_inner_y == TILE_HEIGHT-1) {
                                    // if (tile_inner_x == TILE_WIDTH-1) {
                                        std::cout << ANSI_RESET;
                                    // }
                                // } else if (tile_inner_x == 0 || tile_inner_x == TILE_WIDTH-1) {
                                    // std::cout << ANSI_RESET;
                                // }
                            }
                        }
                    }
                }
            }
            
        }
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ++i;
    }

    
    return 0;
}