#pragma once
#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"

namespace render{
struct WindowInfo{
    const char* name;
    int width;
    int height;
    int x = 0;
    int y = 0;
};
class Window{
public:
    Window();
    ~Window();
    
    void Initialize(WindowInfo info);
    void Terminate();
    
    void GetInstanceExtensions(unsigned int* pCount, const char** pNames);
    VkSurfaceKHR CreateVulkanSurface(VkInstance vk_instance);
    
    int width;
    int height;
private:
    SDL_Window* window_;
};
}
