# Available at setup time due to pyproject.toml
import os

from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup
from setuptools.command.build_ext import build_ext
import subprocess
import sys


__version__ = "0.0.1"

class BuildExtWithStubs(build_ext):
    """build the stubs and move them to the extension directory"""
    def run(self):
        super().run()
        for ext in self.extensions:
            ext_path = self.get_ext_fullpath(ext.name)
            ext_dir = os.path.dirname(os.path.abspath(ext_path))
            
            env = os.environ.copy()
            env["PYTHONPATH"] = ext_dir + os.pathsep + env.get("PYTHONPATH", "")
            
            subprocess.run([
                sys.executable, "-m", "pybind11_stubgen",
                ext.name,
                "--output-dir", ext_dir
            ], env=env, check=True)

ext_modules = [
    Pybind11Extension(
        "robot_programming_game",
        ["src/bindings.cpp"],
        define_macros=[("VERSION_INFO", __version__)],
    ),
]

setup(
    name="robot_programming_game",
    version=__version__,
    author="Šimon Kala",
    author_email="simon.kala@email.cz",
    # url="https://github.com/pybind/python_example",
    description="A test project using pybind11",
    long_description="",
    ext_modules=ext_modules,
    cmdclass={"build_ext": BuildExtWithStubs},
    zip_safe=False,
    python_requires=">=3.9",
)