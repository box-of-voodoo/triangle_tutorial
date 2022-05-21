cmake_minimum_required(VERSION 3.16)
project(triangle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_<LANG>_FLAGS_RELEASE "-O2")
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(Vulkan REQUIRED)
find_package(GLFW REQUIRED)

add_executable(triangle main.cpp)
target_link_libraries(triangle PRIVATE
    Vulkan::Vulkan
    GLFW::GLFW
)

target_compile_features(triangle PRIVATE cxx_std_17)