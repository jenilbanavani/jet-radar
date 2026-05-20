cmake_minimum_required(VERSION 3.16)

project(RadarSimulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

add_executable(radar_sim
    src/Enemy.cpp
    src/RadarRenderer.cpp
    src/Simulation.cpp
    src/main.cpp
)

target_include_directories(radar_sim PRIVATE include)

target_link_libraries(radar_sim PRIVATE sfml-graphics sfml-window sfml-system)

if (MSVC)
    target_compile_options(radar_sim PRIVATE /W4 /permissive-)
else()
    target_compile_options(radar_sim PRIVATE -Wall -Wextra -Wpedantic)
endif()
