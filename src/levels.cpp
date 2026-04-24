#include <random>
#include "levels.h"

std::random_device random_device;
std::mt19937 random_generator(random_device());

bool Level::check_bounds(int x, int y) const {
    return x >= 0 && y >= 0 && x < (int)width() && y < (int)height();
}
bool Level::is_free(int x, int y) const {
    return check_bounds(x, y) && tiles_[y][x].is_free();
}
void Level::increment_collected_coins() {
    coins_collected_++;
    if (coins_collected_ == coins_goal_) {
        won_ = true;
    }
}
Level::Level(size_t width, size_t height, size_t coins_goal, std::pair<size_t, size_t> initial_robot_xy, Direction initial_robot_direction) 
    : width_(width), height_(height), coins_goal_(coins_goal), initial_robot_xy_(initial_robot_xy), initial_robot_direction_(initial_robot_direction), tiles_{height, std::vector<Tile>(width)} {};

void Level::set_flag(size_t x, size_t y, FlagColor flag) {
    auto& tile = tiles_[y][x];
    tile.flags.emplace(flag);
    tile.changed = true;
}
void Level::unset_flag(size_t x, size_t y, FlagColor flag) {
    auto& tile = tiles_[y][x];
    tile.flags.erase(flag);
    tile.changed = true;
}
bool Level::has_flag(size_t x, size_t y, FlagColor flag) {
    return tiles_[y][x].flags.contains(flag);
}

int get_random_int(int min, int max) {
    std::uniform_int_distribution<> distribution(min, max);
    return distribution(random_generator);
}

Level0::Level0() : Level(20, 14, 1, std::make_pair(get_random_int(0, 19), get_random_int(0, 13)), Direction::Right) {
    int x, y;
    while (true) {
        x = get_random_int(0, 19);
        y = get_random_int(0, 13);
        if (x != initial_robot_xy_.first || y != initial_robot_xy_.second) break;
    }
    tiles_[y][x].art = TileArt::Coin;
};

Level1::Level1() : Level(20, 14, 1, std::make_pair(get_random_int(0, 9), get_random_int(5, 8)), Direction::Right) {
    int wall_start_y = get_random_int(1, 5);
    int wall_end_y = get_random_int(8, 12);
    for (size_t y = wall_start_y; y <= wall_end_y; y++) {
        tiles_[y][10].art = TileArt::Wall;
    }
    tiles_[get_random_int(5, 8)][get_random_int(11, 19)].art = TileArt::Coin;
};