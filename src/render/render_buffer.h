#pragma once
#include <optional>

#include "render/swapchain.h"
#include "render/texture.h"

namespace render{
enum LoadOp{
    LOAD_OP_LOAD      = 0,
    LOAD_OP_CLEAR     = 1,
    LOAD_OP_DONT_CARE = 2,
};
enum StoreOp{
    STORE_OP_DONT_CARE,
};
struct Attachment{
    VkImageLayout initial_layout;
    VkImageLayout final_layout;
    VkFormat format;
    //depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    LoadOp  load_op;
    StoreOp store_op;

    //depth_attachment_description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
};
struct AttachmentReference{
    uint32_t index;
    VkImageLayout layout;
};
struct Subpass{
    std::vector<AttachmentReference> color_attachments;
    std::vector<AttachmentReference> input_attachments;
    AttachmentReference*             depth_stencil_attachment = nullptr;
};
struct RenderpassInfo{
    ImageExtent extent;
    std::vector<Attachment> attachments;
    std::vector<Subpass>    subpasses;
};
class Renderpass{
public:
     Renderpass();
    ~Renderpass();
    
    void Initialize(RenderpassInfo info);
    void Terminate();
        
    VkRenderPass vk_render_pass;
};
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
