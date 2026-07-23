cmake_minimum_required(VERSION 3.20.0)                  # ensures the build fails if cmake version is too old
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})   # pulls in the Zephyr build systems, which creates a CMake target named app
project(hello_world)                                    # sets the name of the project.
target_sources(app PRIVATE src/main.c)                  # adds the source file to the project