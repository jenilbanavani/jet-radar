cmake_minimum_required(VERSION 3.16)

project(RadarSimulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Point at the local SFML 3.1.0 install
set(SFML_DIR "C:/SFML/SFML-3.1.0/lib/cmake/SFML" CACHE PATH "Path to SFMLConfig.cmake")

find_package(SFML 3 COMPONENTS Graphics REQUIRED)

add_executable(radar_sim
    src/Enemy.cpp
    src/RadarRenderer.cpp
    src/Simulation.cpp
    src/main.cpp
)

target_include_directories(radar_sim PRIVATE include)
target_link_libraries(radar_sim PRIVATE SFML::Graphics)

if (MSVC)
    target_compile_options(radar_sim PRIVATE /W4 /permissive-)
else()
    target_compile_options(radar_sim PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Automatically copy SFML DLLs next to the executable after building
set(SFML_BIN_DIR "C:/SFML/SFML-3.1.0/bin")
add_custom_command(TARGET radar_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_BIN_DIR}/sfml-graphics-3.dll"
        "${SFML_BIN_DIR}/sfml-window-3.dll"
        "${SFML_BIN_DIR}/sfml-system-3.dll"
        $<TARGET_FILE_DIR:radar_sim>
    COMMENT "Copying SFML runtime DLLs..."
)
