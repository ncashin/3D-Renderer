#pragma once
#include "render_context.h"
#include "swapchain.h"

namespace engine{
class RenderBuffer{
public:
    RenderBuffer(Swapchain* swapchain);
    ~RenderBuffer();
    
    VkRenderPass vk_render_pass;
    std::vector<VkFramebuffer> vk_framebuffers;
    Swapchain* swapchain_attachment;
};
}
