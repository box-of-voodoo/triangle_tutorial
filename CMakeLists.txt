cmake_minimum_required(VERSION 3.16)
project(triangle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_<LANG>_FLAGS_RELEASE "-O2")

# Folder for generating
set(GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)

# Folder for shaders
set(SHADERS_BINS ${CMAKE_BINARY_DIR}/shaders_bin)
file(MAKE_DIRECTORY ${SHADERS_BINS})

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(Vulkan REQUIRED)
find_package(GLFW REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan GLFW::GLFW)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# Compile shaders
add_custom_command(
  TARGET ${PROJECT_NAME}
  PRE_BUILD
  COMMAND /bin/sh ${CMAKE_SOURCE_DIR}/shaders/compile.sh ${SHADERS_BINS}
  COMMENT "Building shaders into SPIR-V")

# Config file
configure_file(config.h.in ${GENERATED_DIR}/config.h)
target_include_directories(${PROJECT_NAME} PUBLIC ${GENERATED_DIR})
