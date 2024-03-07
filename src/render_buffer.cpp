#include "render_buffer.h"

namespace engine{
RenderBuffer::RenderBuffer(Swapchain* swapchain) : swapchain_attachment(swapchain) {
    VkAttachmentDescription swapchain_attachment_description{};
    swapchain_attachment_description.flags = 0;
    swapchain_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    swapchain_attachment_description.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    swapchain_attachment_description.format = swapchain->surface_format_.format;
    swapchain_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    swapchain_attachment_description.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    swapchain_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    swapchain_attachment_description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    swapchain_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    
    VkAttachmentReference swapchain_attachment_reference{};
    swapchain_attachment_reference.attachment = 0;
    swapchain_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription forward_subpass_description{};
    forward_subpass_description.flags = 0;
    forward_subpass_description.colorAttachmentCount = 1;
    forward_subpass_description.pColorAttachments = &swapchain_attachment_reference;
    forward_subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    
    VkSubpassDependency swapchain_present_dependency{};
    swapchain_present_dependency.dependencyFlags = 0;
    swapchain_present_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    swapchain_present_dependency.dstSubpass = 0;
    
    swapchain_present_dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    swapchain_present_dependency.srcAccessMask = 0;
    
    swapchain_present_dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    swapchain_present_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = nullptr;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments    = &swapchain_attachment_description;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses   = &forward_subpass_description;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies   = &swapchain_present_dependency;
    
    vkCreateRenderPass(render_context->vk_device, &render_pass_create_info, nullptr, &vk_render_pass);
    
    std::vector<VkImageView> attachment_views{};
    attachment_views.emplace_back(VkImageView{});
    
    VkFramebufferCreateInfo framebuffer_create_info{};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.pNext = nullptr;
    framebuffer_create_info.flags = 0;
    framebuffer_create_info.width  = swapchain->extent_.width;
    framebuffer_create_info.height = swapchain->extent_.height;
    framebuffer_create_info.layers = 1;
    framebuffer_create_info.renderPass = vk_render_pass;
    framebuffer_create_info.attachmentCount = (uint32_t)attachment_views.size();
    framebuffer_create_info.pAttachments    = attachment_views.data();
    
    vk_framebuffers.resize(swapchain->images_.size());
    uint32_t swapchain_index = (uint32_t)attachment_views.size() - 1;
    VkImageView* swapchain_image_views = swapchain->GetImageViews();
    for(int i = 0; i < swapchain->images_.size(); i++){
        attachment_views[swapchain_index] = swapchain_image_views[i];
        vkCreateFramebuffer(render_context->vk_device, &framebuffer_create_info, nullptr, &vk_framebuffers[i]);
    }
}
RenderBuffer::~RenderBuffer(){
    for(VkFramebuffer framebuffer : vk_framebuffers){
        vkDestroyFramebuffer(render_context->vk_device, framebuffer, nullptr);
    }
    vkDestroyRenderPass(render_context->vk_device, vk_render_pass, nullptr);
}

void RenderBuffer::Begin(VkCommandBuffer vk_command_buffer, Swapchain* swapchain, uint32_t swapchain_image_index){
    VkRenderPassBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.pNext = nullptr;
    
    begin_info.renderPass  = vk_render_pass;
    begin_info.framebuffer = vk_framebuffers[swapchain_image_index];
    begin_info.renderArea  = { 0, 0, swapchain->extent_ };
    
    VkClearValue clear_value = {};
    begin_info.clearValueCount = 1;
    begin_info.pClearValues = &clear_value;
    
    vkCmdBeginRenderPass(vk_command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

VkRenderPass vk_render_pass;
std::vector<VkFramebuffer> vk_framebuffers;
Swapchain* swapchain_attachment;
}
