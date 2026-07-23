cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(l6_e1)

target_sources(app PRIVATE src/main.c src/app_i2c.c src/bmx280.c)