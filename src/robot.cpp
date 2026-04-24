#include "robot.h"

bool Robot::is_tile_visible(int radius, size_t x, size_t y) {
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

void Robot::refresh_visible_tiles() {
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

void Robot::handle_action() {
    if (action_ == RobotAction::MoveForward || action_ == RobotAction::MoveBackward) {
        auto [new_x, new_y] = get_xy_after_move(action_ == RobotAction::MoveForward);
        level_->tiles_[y_][x_].change_art(TileArt::Empty);
        x_ = new_x;
        y_ = new_y;
        if (level_->tiles_[y_][x_].art == TileArt::Coin) {
            level_->increment_collected_coins();
        }
        level_->tiles_[y_][x_].change_art(get_tile_art_for_direction());
        refresh_visible_tiles();
    } else if (action_ == RobotAction::TurnLeft || action_ == RobotAction::TurnRight) {
        auto new_direction = get_direction_after_turn(action_ == RobotAction::TurnRight);
        direction_ = new_direction;
        level_->tiles_[y_][x_].change_art(get_tile_art_for_direction());
    }
    unset_action();
}

void Robot::step() {
    handle_action();
}

std::pair<int, int> Robot::get_xy_after_move(bool forward) {
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

Direction Robot::get_direction_after_turn(bool right) {
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

TileArt Robot::get_tile_art_for_direction() {
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

Robot::Robot(size_t x, size_t y, Direction direction, std::shared_ptr<Level> level) : x_(x), y_(y), direction_(direction), level_(std::move(level)) {
    level_->tiles_[y_][x_].change_art(get_tile_art_for_direction());
    refresh_visible_tiles();
};

RobotActionResult Robot::set_action(RobotAction new_action) {
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

void Robot::unset_action() {
    action_ = std::nullopt;
}

std::vector<std::vector<TileInfo>> Robot::get_tiles() {
    // this is suboptimal, however I don't want to make it change incrementaly based on level changes
    // that is because that would make multiple robots quite hard to implement and I don't want to close that door
    std::vector<std::vector<TileInfo>> tiles_info{level_->width(), std::vector<TileInfo>(level_->height(), TileInfo::Unknown)};
    for (size_t y = 0; y < level_->height(); y++) {
        for (size_t x = 0; x < level_->width(); x++) {
            if (tile_visibility_[y][x]) {
                tiles_info[x][y] = level_->tiles_[y][x].get_tile_info();
            }
        }
    }
    return tiles_info;
}