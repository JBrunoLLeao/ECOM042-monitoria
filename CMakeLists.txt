cmake_minimum_required(VERSION 3.20.0)
 
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(command_pattern)
 
target_include_directories(app PRIVATE include)
target_sources(app PRIVATE
	src/main.c
	src/command.c
	src/commands.c
)
 