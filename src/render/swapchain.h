#pragma once
#include "render/context.h"

#include <mutex>

namespace render{
class Swapchain{
public:
    Swapchain(Window* window);
    ~Swapchain();
    
    uint32_t AcquireImage(VkSemaphore semaphore, VkFence fence);
    
    void CreateImageViews();
    VkImageView* GetImageViews();
    
    std::mutex usage_mutex;
    
    VkSurfaceKHR vk_surface_;
    VkSurfaceFormatKHR surface_format_;
    VkExtent2D extent_;
    VkSwapchainKHR vk_swapchain_;
    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views;
};
}
