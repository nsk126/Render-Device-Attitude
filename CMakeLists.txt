# Minimum Version for CMake
cmake_minimum_required(VERSION 3.0.0)

# Project Name and C++ Standards
project(Render_Orientation 
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)

# Include directory libraries
include_directories(
    include/
)

add_library(
    RenderObj
    include/RenderObj.h
    src/RenderObj.cpp
)

find_package( glm REQUIRED )

# Executable
add_executable(Render_Orientation
    src/tRender.cpp
)


# Link to SDL2 and user made libs
target_link_libraries(Render_Orientation 
    # SDL2main 
    SDL2     
    RenderObj
    GL
    glfw
    GLEW
    glm
)
