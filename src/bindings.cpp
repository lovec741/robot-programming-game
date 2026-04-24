#include <pybind11/pybind11.h>
#include <pybind11/native_enum.h>
#include <pybind11/stl.h>
#include "game.h"

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(robot_programming_game, m, py::mod_gil_not_used()) {
    m.doc() = "A simple robot programming game.\nProgram your robot to collect all the coins and progress to the next level. The levels are randomized each time."; // optional module docstring

    py::class_<Game>(m, "Game")
        .def(py::init<size_t>(), "step_time_ms"_a = 100)
        .def("load_level", &Game::load_level, "level_number"_a)
        .def("step", &Game::step)
        .def("run_with_manual_control", &Game::run_with_manual_control)
        .def_property_readonly("robot", &Game::robot)
        .def_property_readonly("level", &Game::level);

    py::native_enum<RobotAction>(m, "RobotAction", "enum.Enum")
        .value("MoveForward", RobotAction::MoveForward)
        .value("MoveBackward", RobotAction::MoveBackward)
        .value("TurnLeft", RobotAction::TurnLeft)
        .value("TurnRight", RobotAction::TurnRight)
        .finalize();
    
    py::native_enum<RobotActionResult>(m, "RobotActionResult", "enum.Enum")
        .value("Ok", RobotActionResult::Ok)
        .value("Blocked", RobotActionResult::Blocked)
        .value("ActionAlreadyPending", RobotActionResult::ActionAlreadyPending)
        .finalize();

    py::native_enum<Direction>(m, "Direction", "enum.Enum")
        .value("Up", Direction::Up)
        .value("Right", Direction::Right)
        .value("Down", Direction::Down)
        .value("Left", Direction::Left)
        .finalize();
    
    py::native_enum<TileInfo>(m, "TileInfo", "enum.Enum")
        .value("Empty", TileInfo::Empty)
        .value("Robot", TileInfo::Robot)
        .value("Wall", TileInfo::Wall)
        .value("Coin", TileInfo::Coin)
        .value("Unknown", TileInfo::Unknown)
        .finalize();
    
    py::native_enum<FlagColor>(m, "FlagColor", "enum.Enum")
        .value("Red", FlagColor::Red)
        .value("Green", FlagColor::Green)
        .value("Blue", FlagColor::Blue)
        .value("Yellow", FlagColor::Yellow)
        .finalize();
    
    py::class_<Robot>(m, "Robot")
        .def("set_action", &Robot::set_action, "new_action"_a)
        .def("reset_action", &Robot::unset_action)
        .def("get_tiles", &Robot::get_tiles)
        .def_property_readonly("x", &Robot::x)
        .def_property_readonly("y", &Robot::y)
        .def_property_readonly("direction", &Robot::direction);

    py::class_<Level>(m, "Level")
        .def("set_flag", &Level::set_flag, "x"_a, "y"_a, "flag"_a)
        .def("unset_flag", &Level::unset_flag, "x"_a, "y"_a, "flag"_a)
        .def("has_flag", &Level::has_flag, "x"_a, "y"_a, "flag"_a)
        .def_property_readonly("width", &Level::width)
        .def_property_readonly("height", &Level::height)
        .def_property_readonly("coins_collected", &Level::coins_collected)
        .def_property_readonly("coins_goal", &Level::coins_goal);

}
