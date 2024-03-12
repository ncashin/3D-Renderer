#pragma once
#include "render_context.h"

namespace engine{
class Swapchain{
public:
    Swapchain(std::unique_ptr<Window>& window);
    ~Swapchain();
    
    uint32_t AcquireImage(VkSemaphore semaphore, VkFence fence);
    
    void CreateImageViews();
    VkImageView* GetImageViews();
    
    VkSurfaceKHR vk_surface_;
    VkSurfaceFormatKHR surface_format_;
    VkExtent2D extent_;
    VkSwapchainKHR vk_swapchain_;
    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views;
};
}
