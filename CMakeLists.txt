cmake_minimum_required(VERSION 3.0.0)
project(Render_Orientation VERSION 0.1.0)

#include(CTest)
#enable_testing()

add_executable(Render_Orientation src/main.cpp)

target_link_libraries(Render_Orientation 
    SDL2main 
    SDL2     
    GL
)

#set(CPACK_PROJECT_NAME ${PROJECT_NAME})
#set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
#include(CPack)
