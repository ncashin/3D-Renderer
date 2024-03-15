cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_STANDARD 14)
project(engine)

# --- Source Files --- #
set(SOURCE_FILES src/main.cpp 
src/window.h src/window.cpp 
src/vkutil.h src/vkutil.cpp
src/render_context.h src/render_context.cpp
Src/allocation.h
src/allocator.h src/allocator.cpp
src/buffer.h src/buffer.cpp
src/image.h src/image.cpp
src/swapchain.h src/swapchain.cpp
src/render_buffer.h src/render_buffer.cpp
src/pipeline.h src/pipeline.cpp
src/render_manager.h src/render_manager.cpp)
add_executable(runtime ${SOURCE_FILES})

# --- Package Libraries --- #
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(runtime PRIVATE SDL2::SDL2main SDL2::SDL2)

find_package(Vulkan REQUIRED)
target_include_directories(runtime PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(runtime PRIVATE Vulkan::Vulkan)

# --- Vendored Libraries --- #
add_subdirectory(vendor/glm)
include_directories(vendor/glm/glm)
target_link_libraries(runtime PRIVATE glm::glm)

include_directories(vendor/stb)
