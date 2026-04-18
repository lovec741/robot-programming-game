#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

std::atomic<bool> running(true);

constexpr size_t HORIZONTAL_TILES_COUNT = 20;
constexpr size_t VERTICAL_TILES_COUNT = 14;
constexpr size_t TILE_WIDTH = 8;
constexpr size_t TILE_HEIGHT = 3;
constexpr char LINE_WALLS[][4] = {"┏", "┓", "┗", "┛", "━", "┃"};
constexpr char BLOCK_ART[TILE_HEIGHT][TILE_WIDTH][4] = {
    { " ", "╔", "═", "═", "═", "═", "╗", " " },
    { " ", "║", " ", " ", " ", " ", "║", " " },
    { " ", "╚", "═", "═", "═", "═", "╝", " " },
};
constexpr char ROBOT_UP_ART[TILE_HEIGHT][TILE_WIDTH][4] = {
    { " ", "┢", "━", "/", "\\", "━", "┪", " " },
    { " ", "┃", " ", " ", " ", " ", "┃", " " },
    { " ", "┗", "━", "━", "━", "━", "┛", " " },
};
constexpr char ROBOT_DOWN_ART[TILE_HEIGHT][TILE_WIDTH][4] = {
    { " ", "┏", "━", "━", "━", "━", "┓", " " },
    { " ", "┃", " ", " ", " ", " ", "┃", " " },
    { " ", "┡", "━", "\\", "/", "━", "┩", " " },
};
constexpr char ROBOT_RIGHT_ART[TILE_HEIGHT][TILE_WIDTH][4] = {
    { " ", "┏", "━", "━", "━", "━", "┱", "─" },
    { " ", "┃", " ", " ", " ", " ", ">", " " },
    { " ", "┗", "━", "━", "━", "━", "┹", "─" },
};
constexpr char ROBOT_LEFT_ART[TILE_HEIGHT][TILE_WIDTH][4] = {
    { "─", "┲", "━", "━", "━", "━", "┓", " " },
    { " ", "<", " ", " ", " ", " ", "┃", " " },
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

bool is_tile_visible(int radius, size_t x, size_t y, size_t robot_x, size_t robot_y, std::vector<std::vector<bool>> &visible_tiles, std::vector<std::vector<size_t>> &tiles) {
    int offset_x = x - robot_x;
    int offset_y = y - robot_y;
    int sign_offset_x = offset_x < 0 ? -1 : 1;
    int sign_offset_y = offset_y < 0 ? -1 : 1;

    auto is_cover = [&visible_tiles, &tiles](size_t x, size_t y) -> bool {
        return !visible_tiles[y][x] || tiles[y][x] != 0;
    };

    if (radius == 1) {
        if (offset_x*sign_offset_x == offset_y*sign_offset_y) { // corner
            return !is_cover(x-sign_offset_x, y) || !is_cover(x, y-sign_offset_y);
        }
        return true;
    }

    if (offset_x*sign_offset_x == offset_y*sign_offset_y) { // corner
        return !is_cover(x-sign_offset_x, y-sign_offset_y) && (!is_cover(x-sign_offset_x, y) || !is_cover(x, y-sign_offset_y));
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


bool check_xy_bounds(size_t x, size_t y) {
    return x >= 0 && y >= 0 && x < HORIZONTAL_TILES_COUNT && y < VERTICAL_TILES_COUNT;
}

void recalculate_visible_tiles(size_t robot_x, size_t robot_y, std::vector<std::vector<bool>> &visible_tiles, std::vector<std::vector<size_t>> &tiles) {
    for (size_t y = 0; y < VERTICAL_TILES_COUNT; ++y) {
        for (size_t x = 0; x < HORIZONTAL_TILES_COUNT; ++x) {
            visible_tiles[y][x] = true;
        }
    }
    
    visible_tiles[robot_y][robot_x] = true;
    int radius = 1;
    bool tile_in_radius_exists = true;
    size_t work_x, work_y;
    while (tile_in_radius_exists) {
        tile_in_radius_exists = false;
        for (int offset = -radius; offset < radius; ++offset) {
            work_y = robot_y+radius;
            work_x = robot_x+offset;
            if (check_xy_bounds(work_x, work_y)) {
                visible_tiles[work_y][work_x] = is_tile_visible(radius, work_x, work_y, robot_x, robot_y, visible_tiles, tiles);
                tile_in_radius_exists = true;
            }
            work_y = robot_y-radius;
            work_x = robot_x-offset;
            if (check_xy_bounds(work_x, work_y)) {
                visible_tiles[work_y][work_x] = is_tile_visible(radius, work_x, work_y, robot_x, robot_y, visible_tiles, tiles);
                tile_in_radius_exists = true;
            }
            work_y = robot_y+offset+1;
            work_x = robot_x+radius;
            if (check_xy_bounds(work_x, work_y)) {
                visible_tiles[work_y][work_x] = is_tile_visible(radius, work_x, work_y, robot_x, robot_y, visible_tiles, tiles);
                tile_in_radius_exists = true;
            }
            work_y = robot_y-offset-1;
            work_x = robot_x-radius;
            if (check_xy_bounds(work_x, work_y)) {
                visible_tiles[work_y][work_x] = is_tile_visible(radius, work_x, work_y, robot_x, robot_y, visible_tiles, tiles);
                tile_in_radius_exists = true;
            }
        }
        ++radius;
    }
}

int main() {
    set_raw_mode(true);
    std::cout << "\033[?25l"; // hide cursor

    std::vector<std::vector<size_t>> tiles(VERTICAL_TILES_COUNT, std::vector<size_t>(HORIZONTAL_TILES_COUNT, 0));
    tiles[4][4] = 6;
    tiles[4][5] = 6;
    tiles[5][5] = 6;
    tiles[6][5] = 6;
    tiles[4][7] = 6;
    tiles[5][7] = 6;
    tiles[6][7] = 6;
    tiles[6][10] = 6;
    tiles[6][11] = 6;
    tiles[6][12] = 6;
    tiles[6][13] = 6;
    size_t current_x = 6, current_y = 6;
    tiles[current_x][current_y] = 4;

    const size_t CANVAS_WIDTH = HORIZONTAL_TILES_COUNT * TILE_WIDTH;
    const size_t CANVAS_HEIGHT = VERTICAL_TILES_COUNT * TILE_HEIGHT;

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

            char ch;
            if (read(STDIN_FILENO, &ch, 1) == 1) {
                if (ch == 'q') break;
                if (ch == 'w' || ch == 'a' || ch == 's' || ch == 'd') {
                    tiles[current_y][current_x] = 0;
                    size_t tile_art_id = 0;
                    switch (ch)
                    {
                    case 'w':
                        if (current_y != 0) current_y--;
                        tile_art_id = 1;
                        break;
                    case 'a':
                        if (current_x != 0) current_x--;
                        tile_art_id = 3;
                        break;
                    case 's':
                        if (current_y != VERTICAL_TILES_COUNT-1) current_y++;
                        tile_art_id = 2;
                        break;
                    case 'd':
                        if (current_x != HORIZONTAL_TILES_COUNT-1) current_x++;
                        tile_art_id = 4;
                        break;
                    }
                    tiles[current_y][current_x] = tile_art_id;
                    recalculate_visible_tiles(current_x, current_y, visible_tiles, tiles);
                }
            }

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
                            if (!visible_tiles[tile_y][tile_x]) {
                                srand(x*HORIZONTAL_TILES_COUNT+y);
                                std::cout << ((rand() % 2 == 0) ? "?" : "X");;
                                continue;
                            }
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
                    }
                }
            }
            
        }
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ++i;
    }

    std::cout << "\033[?25h"; // restore cursor
    std::cout << "\033[2J\033[H"; // clear screen
    set_raw_mode(false);
    return 0;
}