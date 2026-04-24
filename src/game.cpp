#include <thread>
#include "game.h"
#include "ansi_utils.h"

bool Game::check_for_terminal_size_change() {
    size_t new_terminal_rows, new_terminal_cols;
    get_terminal_size(new_terminal_rows, new_terminal_cols);
    if (new_terminal_rows != terminal_rows_ || new_terminal_cols != terminal_cols_)  {
        terminal_rows_ = new_terminal_rows;
        terminal_cols_ = new_terminal_cols;
        return true;
    }
    return false;
}

void Game::draw_border(size_t start_x, size_t start_y, size_t width, size_t height) {
    size_t border_start_x = start_x - 1;
    size_t border_start_y = start_y - 1;
    ansi_move_cursor(border_start_x, border_start_y);
    std::cout << LINE_WALLS[0];
    for (size_t i = 0; i < width; i++)
    {
        std::cout << LINE_WALLS[4];
    }
    std::cout << LINE_WALLS[1];

    for (size_t i = 1; i <= height; i++)
    {
        ansi_move_cursor(border_start_x, border_start_y+i);
        std::cout << LINE_WALLS[5];

        ansi_move_cursor(border_start_x+width+1, border_start_y+i);
        std::cout << LINE_WALLS[5];
    }

    ansi_move_cursor(border_start_x, border_start_y+height+1);
    std::cout << LINE_WALLS[2];
    for (size_t i = 0; i < width; i++)
    {
        std::cout << LINE_WALLS[4];
    }
    std::cout << LINE_WALLS[3];

}

void Game::draw_canvas_too_small() {
    std::cout << ANSI_CLEAR;
    ansi_move_cursor((terminal_cols_ - CANVAS_TOO_SMALL_TEXT.size()) / 2, terminal_rows_ / 2);
    std::cout << CANVAS_TOO_SMALL_TEXT;
    std::cout.flush();
}

void Game::draw_controls_text() {
    auto text = manual_control_ ? CONTROLS_WITH_MANUAL_TEXT : CONTROLS_TEXT;
    ansi_move_cursor((terminal_cols_ - text.size()) / 2, terminal_rows_-2);
    std::cout << text;
}

void Game::draw_won_text() {
    auto text = manual_control_ ? WON_WITH_MANUAL_TEXT : WON_TEXT;
    size_t start_x = (terminal_cols_ - text.size()) / 2;
    size_t start_y = terminal_rows_ / 2;
    draw_border(start_x-1, start_y, text.size()+2, 1);
    ansi_move_cursor(start_x-1, start_y);
    std::cout << " " << text << " ";
}

void Game::draw_tile(size_t canvas_start_x, size_t canvas_start_y, size_t x, size_t y, Tile& tile) {
    size_t offset_x = x*TILE_WIDTH;
    size_t offset_y = y*TILE_HEIGHT;
    bool has_flags = !tile.flags.empty();
    for (size_t inner_y = 0; inner_y < TILE_HEIGHT; inner_y++) {
        ansi_move_cursor(canvas_start_x+offset_x, canvas_start_y+offset_y+inner_y);
        for (size_t inner_x = 0; inner_x < TILE_WIDTH; inner_x++) {
            if (has_flags) {
                size_t idx = (inner_x/2) % tile.flags.size();
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
                case TileArt::Coin:
                    size_t animation_step = (tick_ % 24) / 6;
                    if (animation_step == 0) {
                        std::cout << COIN_ART_1[inner_y][inner_x];
                    } else if (animation_step == 1 || animation_step == 3) {
                        std::cout << COIN_ART_2[inner_y][inner_x];
                    } else {
                        std::cout << COIN_ART_3[inner_y][inner_x];
                    }
                    break;
                }
            }
            
            
            if (has_flags) {
                std::cout << ANSI_RESET;
            }
        }
    }
}

void Game::draw(bool full_update) {
    size_t canvas_start_x = (terminal_cols_ - canvas_width_) / 2 - 3;
    size_t canvas_start_y = (terminal_rows_ - canvas_height_) / 2;
    if (full_update) {
        std::cout << ANSI_CLEAR;
        draw_border(canvas_start_x, canvas_start_y, canvas_width_, canvas_height_);
        draw_controls_text();
    }
    for (size_t y = 0; y < level_->height(); y++) {
        for (size_t x = 0; x < level_->width(); x++) {
            auto& tile = level_->tiles_[y][x];
            if (full_update || tile.changed || (tile.art == TileArt::Coin && robot_->tile_visibility_[y][x])) {
                draw_tile(canvas_start_x, canvas_start_y, x, y, tile);
                tile.changed = false;
            }
        }
    }
    std::cout.flush();
}

void Game::handle_input() {
    char ch;
    if (read(STDIN_FILENO, &ch, 1) == 1) {
        if (ch == 'q') running_ = false;
        if (!manual_control_) return;
        if (ch == '1' || ch == '2' || ch == '3' || ch == '4') {
            FlagColor flag;
            switch (ch)
            {
            case '1':
                flag = FlagColor::Red;
                break;
            case '2':
                flag = FlagColor::Green;
                break;
            case '3':
                flag = FlagColor::Blue;
                break;
            case '4':
                flag = FlagColor::Yellow;
                break;
            default:
                throw std::runtime_error("Invalid flag color");
            }
            if (level_->has_flag(robot_->x_, robot_->y_, flag)) {
                level_->unset_flag(robot_->x_, robot_->y_, flag);
            } else {
                level_->set_flag(robot_->x_, robot_->y_, flag);
            }
        }
        if (ch == 'w' || ch == 'a' || ch == 's' || ch == 'd') {
            robot_->unset_action();
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

void Game::setup_terminal() {
    set_raw_mode(true);
    std::cout << ANSI_HIDE_CURSOR;
}

void Game::cleanup_terminal() {
    std::cout << ANSI_SHOW_CURSOR;
    std::cout << ANSI_CLEAR;
    set_raw_mode(false);
}

Game::Game(size_t step_time_ms) : step_speed_(step_time_ms) {}

bool Game::step() {
    if (!robot_ || !level_) {
        throw std::runtime_error("No level loaded");
    }

    auto start_time = std::chrono::steady_clock::now();

    if (tick_ == 0) {
        setup_terminal();
    }
    
    handle_input();
    if (!win_screen_shown_) {
        robot_->step();
    }

    bool size_changed = check_for_terminal_size_change();
    if (size_changed) {
        if (terminal_rows_ < canvas_height_ + 5 || terminal_cols_ < canvas_width_ + 2) {
            canvas_too_small_ = true;
            draw_canvas_too_small();
        } else {
            canvas_too_small_ = false;
        }
    }

    if (!canvas_too_small_ && !win_screen_shown_) {
        draw(size_changed);
    }
    if (level_->won_) {
        draw_won_text();
        win_screen_shown_ = true;
    }

    if (!running_) {
        cleanup_terminal();
        return false;
    }
    
    auto elapsed_time = std::chrono::steady_clock::now() - start_time;
    if (elapsed_time < step_speed_) {
        std::this_thread::sleep_for(step_speed_ - elapsed_time);
    }
    tick_++;
    return true;
}

void Game::load_level(size_t level_number) {
    switch (level_number)
    {
    case 0:
        level_ = std::make_shared<Level0>();
        break;
    case 1:
        level_ = std::make_shared<Level1>();
        break;
    
    default:
        throw std::runtime_error("Invalid level number");
    }
    canvas_width_ = level_->width()*TILE_WIDTH;
    canvas_height_ = level_->height()*TILE_HEIGHT;
    auto [initial_robot_x, initial_robot_y] = level_->initial_robot_xy_;
    robot_ = std::make_shared<Robot>(initial_robot_x, initial_robot_y, level_->initial_robot_direction_, level_);
}

void Game::run_with_manual_control() {
    manual_control_ = true;
    while (step()) {};
}
