cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_STANDARD 11)
project(engine)

# --- Source Files --- #
set(SOURCE_FILES src/main.cpp)
add_executable(runtime ${SOURCE_FILES})

# --- Package Libraries --- #
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(runtime PRIVATE SDL2::SDL2main SDL2::SDL2)

find_package(Vulkan REQUIRED)
target_include_directories(runtime PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(runtime INTERFACE Vulkan::Vulkan)

# --- Vendored Libraries --- #
add_subdirectory(vendor/glm)
include_directories(vendor/glm/glm)
target_link_libraries(runtime PUBLIC glm::glm)

include_directories(vendor/stb)