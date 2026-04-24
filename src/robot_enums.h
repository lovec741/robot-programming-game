#ifndef ROBOT_ENUMS_H_
#define ROBOT_ENUMS_H_

enum class Direction {
    Up, Right, Down, Left
};

enum class RobotAction {
    MoveForward, MoveBackward, TurnLeft, TurnRight
};

enum class RobotActionResult {
    Ok, Blocked, ActionAlreadyPending
};

#endif