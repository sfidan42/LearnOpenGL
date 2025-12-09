cmake_minimum_required(VERSION 3.10)
project(OpenGLTriangle)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

add_executable(triangle main.cpp)

file(GLOB_RECURSE PROJECT_SOURCES ${CMAKE_SOURCE_DIR}/source/*.cpp)
target_sources(triangle PRIVATE ${PROJECT_SOURCES})

find_package(OpenGL REQUIRED)

add_library(glad STATIC vendored/glad/src/glad.c)
target_include_directories(glad PUBLIC vendored/glad/include)

target_include_directories(triangle
        PUBLIC
            vendored/glfw-3.4/include
            vendored/glm
            vendored/stb
            ${CMAKE_SOURCE_DIR}/source
)

target_link_libraries(triangle
        OpenGL::GL
        glad
        ${CMAKE_DL_LIBS}
        ${CMAKE_SOURCE_DIR}/vendored/glfw-3.4/libglfw3.a
)

add_definitions(-DDATA_DIR="${CMAKE_SOURCE_DIR}/data")
