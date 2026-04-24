#ifndef ANSI_UTILS_H_
#define ANSI_UTILS_H_

#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

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

void ansi_move_cursor(size_t x, size_t y) {
    std::cout << "\033[" << y+1 << ";" << x+1 << "H";
}

#endif