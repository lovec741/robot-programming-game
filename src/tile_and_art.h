#ifndef TILE_AND_ART_H_
#define TILE_AND_ART_H_

#include <set>

constexpr size_t TILE_WIDTH = 8;
constexpr size_t TILE_HEIGHT = 3;

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

constexpr char COIN_ART_1[TILE_HEIGHT][TILE_WIDTH][5] = {
    { " ", " ", "▗", "▄", "▄", "▖", " ", " " },
    { " ", "█", "▌", " ", " ", "▐", "█", " " },
    { " ", " ", "▝", "▀", "▀", "▘", " ", " " },
};

constexpr char COIN_ART_2[TILE_HEIGHT][TILE_WIDTH][5] = {
    { " ", " ", " ", "▄", "▄", " ", " ", " " },
    { " ", " ", "█", " ", " ", "█", " ", " " },
    { " ", " ", " ", "▀", "▀", " ", " ", " " },
};

constexpr char COIN_ART_3[TILE_HEIGHT][TILE_WIDTH][5] = {
    { " ", " ", " ", "▗", "▖", " ", " ", " " },
    { " ", " ", " ", "▌", "▐", " ", " ", " " },
    { " ", " ", " ", "▝", "▘", " ", " ", " " },
};

enum class FlagColor {
    Red, Green, Blue, Yellow
};

enum class TileArt {
    Empty, RobotUp, RobotDown, RobotLeft, RobotRight, Wall, Coin
};

enum class TileInfo {
    Empty, Robot, Wall, Coin, Unknown
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
        default:
            throw std::runtime_error("Invalid tile art");
        }
    };
    void change_art(TileArt new_art) {
        art = new_art;
        changed = true;
    }
    bool is_free() const {
        return art == TileArt::Empty || art == TileArt::Coin;
    };
};

#endif