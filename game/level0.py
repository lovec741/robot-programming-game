from robot_programming_game import Game, RobotAction, FlagColor, TileInfo, Direction

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
# we can just move to the coin straigh away

# at the start the robot is facing to the right
# print(robot.direction)

i = 0
while game.step():
    # you can use flags to mark the tiles we have already visited
    level.set_flag(robot.x, robot.y, FlagColor.Blue)

    # if the coin is to the right from us, move forward (since we are already facing right)
    if robot.direction == Direction.Right and robot.x < coin_position[0]:
        robot.set_action(RobotAction.MoveForward)
    # if the coin is to the left from us, we need to turn, when we are facing down we need to turn again to turn 180deg
    elif (robot.direction == Direction.Right or robot.direction == Direction.Down) and robot.x > coin_position[0]:
        robot.set_action(RobotAction.TurnRight)
    # if the coin is to the left from us and we are facing left, move forward
    elif robot.direction == Direction.Left and robot.x > coin_position[0]:
        robot.set_action(RobotAction.MoveForward)
    # if the coin is directly above or below us 
    elif (robot.direction == Direction.Right or robot.direction == Direction.Left) and robot.x == coin_position[0]:
        # turn towards the coin
        if robot.y < coin_position[1]:
            robot.set_action(RobotAction.TurnRight if robot.direction == Direction.Right else RobotAction.TurnLeft)
        elif robot.y > coin_position[1]:
            robot.set_action(RobotAction.TurnLeft if robot.direction == Direction.Right else RobotAction.TurnRight)
    # we are facing towards the coin, move forward
    else:
        robot.set_action(RobotAction.MoveForward)

# this one was very simple, since we could see the whole board from the start
# what if there was a wall that obstructed the robots vision?