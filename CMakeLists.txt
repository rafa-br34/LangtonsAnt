cmake_minimum_required(VERSION 3.20)

project(
	LangtonsAnt
	DESCRIPTION "A simple project that simulates the Langton's ant universal Turing machine"
	HOMEPAGE_URL "https://github.com/rafa-br34/LangtonsAnt"
	LANGUAGES CXX
)

# Setup constants
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_CXX_STANDARD 20)


include(CMake/lodepng.cmake)
include(CMake/argparse.cmake)

if(MSVC)
	add_compile_options(/W4)
else()
	add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(${PROJECT_NAME} SOURCE/Entry.cpp ${COMMON_SOURCES})