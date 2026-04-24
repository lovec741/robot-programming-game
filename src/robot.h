#ifndef ROBOT_H_
#define ROBOT_H_

#include <vector>
#include <memory>
#include <optional>
#include "tile_and_art.h"
#include "robot_enums.h"
#include "levels.h"

class Robot {
private:
    size_t x_;
    size_t y_;
    Direction direction_;
    std::shared_ptr<Level> level_;
    std::vector<std::vector<bool>> tile_visibility_{level_->height(), std::vector<bool>(level_->width(), true)};
    std::optional<RobotAction> action_;

    bool is_tile_visible(int radius, size_t x, size_t y);
    void refresh_visible_tiles();
    void handle_action();
    void step();
    std::pair<int, int> get_xy_after_move(bool forward);
    Direction get_direction_after_turn(bool right);
    TileArt get_tile_art_for_direction();
public:
    Robot(size_t x, size_t y, Direction direction, std::shared_ptr<Level> level);
    RobotActionResult set_action(RobotAction new_action);
    void unset_action();
    size_t x() {
        return x_;
    }
    size_t y() {
        return y_;
    }
    Direction direction() {
        return direction_;
    }
    std::vector<std::vector<TileInfo>> get_tiles();
friend class Game;
};

#endif
