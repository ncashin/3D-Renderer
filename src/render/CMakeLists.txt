cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_STANDARD 14)

set(RENDER_SOURCE_FILES 
${CMAKE_CURRENT_LIST_DIR}/vkutil.h  ${CMAKE_CURRENT_LIST_DIR}/vkutil.cpp
${CMAKE_CURRENT_LIST_DIR}/render.h  ${CMAKE_CURRENT_LIST_DIR}/render.cpp
${CMAKE_CURRENT_LIST_DIR}/context.h ${CMAKE_CURRENT_LIST_DIR}/context.cpp
${CMAKE_CURRENT_LIST_DIR}/buffer.h  ${CMAKE_CURRENT_LIST_DIR}/buffer.cpp
${CMAKE_CURRENT_LIST_DIR}/texture.h ${CMAKE_CURRENT_LIST_DIR}/texture.cpp
${CMAKE_CURRENT_LIST_DIR}/descriptor.h ${CMAKE_CURRENT_LIST_DIR}/descriptor.cpp
${CMAKE_CURRENT_LIST_DIR}/swapchain.h  ${CMAKE_CURRENT_LIST_DIR}/swapchain.cpp
${CMAKE_CURRENT_LIST_DIR}/render_buffer.h ${CMAKE_CURRENT_LIST_DIR}/render_buffer.cpp
${CMAKE_CURRENT_LIST_DIR}/pipeline.h   ${CMAKE_CURRENT_LIST_DIR}/pipeline.cpp
${CMAKE_CURRENT_LIST_DIR}/command.h    ${CMAKE_CURRENT_LIST_DIR}/command.cpp
${CMAKE_CURRENT_LIST_DIR}/staging.h    ${CMAKE_CURRENT_LIST_DIR}/staging.cpp)