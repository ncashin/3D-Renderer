#include "window.h"

namespace render{
Window::Window(){};
Window::~Window(){}

void Window::Initialize(WindowInfo info){
    width = info.width;
    height = info.height;
    SDL_SetRelativeMouseMode(SDL_TRUE);
    window_ = SDL_CreateWindow(info.name, info.x, info.y, width, height,
                               SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
}
void Window::Terminate(){
    SDL_DestroyWindow(window_);
}

void Window::GetInstanceExtensions(unsigned int* pCount, const char** pNames){
    SDL_Vulkan_GetInstanceExtensions(window_, pCount, pNames);
}
VkSurfaceKHR Window::CreateVulkanSurface(VkInstance vk_instance){
    VkSurfaceKHR vk_surface;
    SDL_Vulkan_CreateSurface(window_, vk_instance, &vk_surface);
    return vk_surface;
}
}
