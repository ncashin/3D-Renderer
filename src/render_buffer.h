#pragma once
#include "render_context.h"
#include "swapchain.h"

namespace ngfx{
class RenderBuffer{
public:
    RenderBuffer(Swapchain* swapchain);
    ~RenderBuffer();
    
    void Begin(VkCommandBuffer vk_command_buffer, Swapchain* swapchain, uint32_t swapchain_image_index);
    
    VkRenderPass vk_render_pass;
    std::vector<VkFramebuffer> vk_framebuffers;
    Swapchain* swapchain_attachment;
};
}
