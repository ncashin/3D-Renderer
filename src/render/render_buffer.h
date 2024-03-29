#pragma once
#include "render/swapchain.h"
#include "render/texture.h"

namespace render{
class RenderBuffer{
public:
    RenderBuffer(Swapchain* swapchain);
    ~RenderBuffer();
    
    void Begin(VkCommandBuffer vk_command_buffer, Swapchain* swapchain, uint32_t swapchain_image_index);
    
    void CreateDepthImageAndView();
    
    ImageExtent extent;
    VmaAllocation vma_depth_allocation;
    VkImage     vk_depth_image;
    VkImageView vk_depth_image_view;
    
    VkRenderPass vk_render_pass;
    std::vector<VkFramebuffer> vk_framebuffers;
    Swapchain* swapchain_attachment;
};
}
