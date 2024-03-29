cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_STANDARD 14)
project(engine)

# --- Source Files --- #
set(ENGINE_SOURCE_FILES 
src/main.cpp 
src/window.h src/window.cpp 
src/thread_pool.h src/thread_pool.cpp 
src/asset.h src/asset.cpp 
src/input.h src/input.cpp)

include(src/render/CMakeLists.txt)

add_executable(runtime ${ENGINE_SOURCE_FILES} ${RENDER_SOURCE_FILES})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

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

set(BUILD_STATIC_LIB ON)
set(BUILD_ASSIMP_TOOLS  ON)
set(ASSIMP_BUILD_STATIC_LIB ON)

add_subdirectory(vendor/assimp)
include_directories(vendor/assimp/include)
target_link_libraries(runtime PRIVATE assimp)

add_subdirectory(vendor/VulkanMemoryAllocator)
target_link_libraries(runtime PRIVATE VulkanMemoryAllocator)

