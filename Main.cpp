#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

std::atomic<bool> running(true);

constexpr size_t HORIZONTAL_TILES_COUNT = 10;
constexpr size_t VERTICAL_TILES_COUNT = 6;
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

void setRawMode(bool enable) {
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

void getTerminalSize(size_t &rows, size_t &cols) {
    winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    rows = w.ws_row;
    cols = w.ws_col;
}

int main() {
    setRawMode(true);
    std::cout << "\033[?25l"; // hide cursor

    std::vector<std::vector<size_t>> tiles(VERTICAL_TILES_COUNT, std::vector<size_t>(HORIZONTAL_TILES_COUNT, false));
    tiles[0][0] = 0;
    size_t current_x = 0, current_y = 0;

    const size_t CANVAS_WIDTH = HORIZONTAL_TILES_COUNT * TILE_WIDTH;
    const size_t CANVAS_HEIGHT = VERTICAL_TILES_COUNT * TILE_HEIGHT;

    while (running) {
        std::cout << "\033[H"; // move cursor to top-left
        size_t rows, cols;
        getTerminalSize(rows, cols);
        if (rows < CANVAS_HEIGHT + 2 || cols < CANVAS_WIDTH + 2) {
            std::cout << "Please resize your window to make the canvas fit!";
        } else {
            size_t canvas_start_x = (cols - CANVAS_WIDTH) / 2 - 1;
            size_t canvas_start_y = (rows - CANVAS_HEIGHT) / 2 - 1;

            char ch;
            if (read(STDIN_FILENO, &ch, 1) == 1) {
                if (ch == 'q') break;
                if (ch == 'w' || ch == 'a' || ch == 's' || ch == 'd') {
                    tiles[current_y][current_x] = false;
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
                }
            }


            for (size_t r = 0; r < rows; ++r) {
                for (size_t c = 0; c < cols; ++c) {
                    // std::cout << (((r+c) % 2 == 0) ? fill1 : fill2);
                    if (c < canvas_start_x || r < canvas_start_y || c > canvas_start_x + CANVAS_WIDTH + 1 || r > canvas_start_y + CANVAS_HEIGHT + 1) {
                        std::cout << ' ';
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
                        switch (tiles[tile_y][tile_x])
                        {
                        case 0:
                            std::cout << ' ';
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
                        }
                        
                    }
                    
                }
            }
        }
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\033[?25h"; // restore cursor
    std::cout << "\033[2J\033[H"; // clear screen
    setRawMode(false);
    return 0;
}