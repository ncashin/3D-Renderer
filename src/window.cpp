#include "window.h"

namespace engine{
void Window::Init(){
    SDL_Init(SDL_INIT_EVERYTHING);
}


Window::Window(const char* name, int width, int height, int x, int y){
    window_ = SDL_CreateWindow(name, x, y, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
}
Window::~Window(){
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
