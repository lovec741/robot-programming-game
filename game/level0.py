from robot_programming_game import Game, Robot, RobotAction, FlagColor, TileInfo, Direction

# Level 0 - Tutorial

# Game(step_time_ms=100)
# - load levels by level number - `load_level(level_number)`
# - run a step of the game loop - `step()`
#   - the time the step takes is decided by `step_time_ms`
# - play the level with manual controls - `run_with_manual_control()`
game = Game(step_time_ms=500) # slow down from 100ms to 500ms per step, to better see what the robot is doing
game.load_level(0)

# Robot
# - get the robots position and direction - `x`, `y`, `direction`
# - allows you to take actions as the robot  - `set_action(new_action)`
#   - the action will be performed during the next game loop iteration
#   - to unset the action  - `unset_action()`
# - see the tiles the robot can see - `get_tiles()`
robot = game.robot

# Level
# - get the size of the board - `width`, `height`
# - get the coins you need and the amount you have already collected - `coins_goal`, `coins_collected`
# - manage flags - `has_flag(x, y, flag)`, `set_flag(x, y, flag)`, `unset_flag(x, y, flag)`
#   - flags allow you to mark tiles on the board with a certain color
level = game.level

# this level requires us to collect one coin and there are no walls, run the next command to explore the level (you will probably want to decrease `step_time_ms` to 100)
# game.run_with_manual_control()

# we can check if there is really only one coin with
# print(level.coins_goal)

# we find the coin
tiles = robot.get_tiles()
coin_position = None
for x in range(level.width):
    for y in range(level.height):
        if tiles[x][y] == TileInfo.Coin:
            coin_position = (x, y)
if coin_position is None:
    raise Exception("Coin is missing!")

# since there are no walls and we can see the coin directly
# we can just move to the coin straight away

# at the start the robot is facing to the right
# print(robot.direction)

def get_next_action(robot: Robot, coin_position: tuple[int, int]):
    dx = coin_position[0] - robot.x
    dy = coin_position[1] - robot.y

    # get desired direction
    if dx > 0:
        target = Direction.Right
    elif dx < 0:
        target = Direction.Left
    elif dy > 0:
        target = Direction.Down
    else:
        target = Direction.Up

    if robot.direction == target:
        return RobotAction.MoveForward
    else: # turn until we are at the desired position
        return RobotAction.TurnLeft

while game.step():
    # you can use flags to mark the tiles we have already visited
    level.set_flag(robot.x, robot.y, FlagColor.Blue)

    # decide the next action and give it to the robot
    robot.set_action(get_next_action(robot, coin_position))

# this one was very simple, since we could see the whole board from the start
# what if there was a wall that obstructed the robots vision?