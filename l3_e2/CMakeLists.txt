cmake_minimum_required(VERSION 3.20.0)                  # ensures the build fails if cmake version is too old
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})   # pulls in the Zephyr build systems, which creates a CMake target named app
project(hello_world)                                    # sets the name of the project.
target_sources(app PRIVATE src/main.c)                  # adds the source file to the project
#target_sources(app PRIVATE src/myfunction.c)            # The issue with including source code through target_sources() is that we will include this file in our build regardless of whether we will use it. 
                                                        # In the next paragraph, we will learn how to control the inclusion of source code using Kconfig symbols and target_sources_ifdef().
target_sources_ifdef(CONFIG_MYFUNCTION app PRIVATE src/myfunction.c) # The build will now only include the custom file myfunction.c if CONFIG_MYFUNCTION is enabled