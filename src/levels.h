#ifndef LEVELS_H_
#define LEVELS_H_

#include <vector>
#include <memory>
#include <set>
#include "tile_and_art.h"
#include "robot_enums.h"

class Level {
protected:
    size_t width_;
    size_t height_;
    size_t coins_goal_;
    size_t coins_collected_ = 0;
    std::pair<size_t, size_t> initial_robot_xy_;
    Direction initial_robot_direction_;
    std::vector<std::vector<Tile>> tiles_;
    bool won_ = false;

    bool check_bounds(int x, int y) const;
    bool is_free(int x, int y) const;
    void increment_collected_coins();
public:
    Level(size_t width, size_t height, size_t coins_goal, std::pair<size_t, size_t> initial_robot_xy, Direction initial_robot_direction);
    virtual ~Level() = default;
    size_t width() const {return width_;};
    size_t height() const {return height_;};
    size_t coins_collected() const {return coins_collected_;};
    size_t coins_goal() const {return coins_goal_;};
    void set_flag(size_t x, size_t y, FlagColor flag);
    void unset_flag(size_t x, size_t y, FlagColor flag);
    bool has_flag(size_t x, size_t y, FlagColor flag);
friend class Robot;
friend class Game;
};

class Level0 : public Level {
public:
    Level0();
};

class Level1 : public Level {
public:
    Level1();
};

#endif
