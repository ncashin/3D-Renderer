#include "render_buffer.h"

namespace render{
Renderpass:: Renderpass(){};
Renderpass::~Renderpass(){};

void Renderpass::Initialize(RenderpassInfo info){
    std::vector<VkAttachmentDescription> vk_attachment_descriptions;
    for(Attachment attachment : info.attachments){
        VkAttachmentDescription vk_attachment{};
        vk_attachment.flags = 0;
        vk_attachment.initialLayout = attachment.initial_layout;
        vk_attachment.finalLayout   = attachment.final_layout;
        vk_attachment.format  = attachment.format;
        vk_attachment.loadOp  = (VkAttachmentLoadOp) attachment.load_op;
        vk_attachment.storeOp = (VkAttachmentStoreOp)attachment.store_op;
        vk_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        vk_attachment_descriptions.emplace_back(vk_attachment);
    }
    std::vector<VkSubpassDescription> vk_subpass_descriptions;
    for(Subpass subpass : info.subpasses){
        VkSubpassDescription vk_subpass{};
        vk_subpass.flags = 0;
        vk_subpass.colorAttachmentCount    = (uint32_t)subpass.color_attachments.size();
        vk_subpass.pColorAttachments       = (VkAttachmentReference*)subpass.color_attachments.data();
        vk_subpass.pDepthStencilAttachment = (VkAttachmentReference*)subpass.depth_stencil_attachment;
        vk_subpass_descriptions.emplace_back(vk_subpass);
    }
    
    VkRenderPassCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.attachmentCount = (uint32_t)vk_attachment_descriptions.size();
    create_info.pAttachments    = vk_attachment_descriptions.data();
    create_info.subpassCount = (uint32_t)vk_subpass_descriptions.size();
    create_info.pSubpasses   = vk_subpass_descriptions.data();
    create_info.dependencyCount = 0;
    create_info.pDependencies   = nullptr;
    vkCreateRenderPass(render::context.vk_device, &create_info, nullptr, &vk_render_pass);
}

RenderBuffer::RenderBuffer(Swapchain* swapchain) : swapchain_attachment(swapchain) {
    extent.width  = 1440;
    extent.height = 1080;
    extent.depth  = 1;
    CreateDepthImageAndView();
    std::vector<VkAttachmentDescription> attachment_descriptions{};
    
    VkAttachmentDescription depth_attachment_description{};
    depth_attachment_description.flags = 0;
    depth_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment_description.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment_description.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment_description.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    
    VkAttachmentReference depth_attachment_reference{};
    depth_attachment_reference.attachment = (uint32_t)attachment_descriptions.size();
    depth_attachment_reference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachment_descriptions.emplace_back(depth_attachment_description);
    
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
    swapchain_attachment_reference.attachment = (uint32_t)attachment_descriptions.size();
    swapchain_attachment_reference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_descriptions.emplace_back(swapchain_attachment_description);

    VkSubpassDescription forward_subpass_description{};
    forward_subpass_description.flags = 0;
    forward_subpass_description.colorAttachmentCount = 1;
    forward_subpass_description.pColorAttachments = &swapchain_attachment_reference;
    forward_subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    forward_subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
    
    VkSubpassDependency swapchain_present_dependency{};
    swapchain_present_dependency.dependencyFlags = 0;
    swapchain_present_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    swapchain_present_dependency.dstSubpass = 0;
    
    swapchain_present_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    swapchain_present_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    swapchain_present_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    
    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = nullptr;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = (uint32_t)attachment_descriptions.size();
    render_pass_create_info.pAttachments    = attachment_descriptions.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses   = &forward_subpass_description;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies   = &swapchain_present_dependency;
    
    vkCreateRenderPass(render::context.vk_device, &render_pass_create_info, nullptr, &vk_render_pass);
    
    std::vector<VkImageView> attachment_views{};
    attachment_views.emplace_back(vk_depth_image_view);
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
    for(uint32_t i = 0; i < swapchain->images_.size(); i++){
        attachment_views[swapchain_index] = swapchain_image_views[i];
        vkCreateFramebuffer(render::context.vk_device, &framebuffer_create_info, nullptr, &vk_framebuffers[i]);
    }
}
RenderBuffer::~RenderBuffer(){
    for(VkFramebuffer framebuffer : vk_framebuffers){
        vkDestroyFramebuffer(render::context.vk_device, framebuffer, nullptr);
    }
    vkDestroyRenderPass(render::context.vk_device, vk_render_pass, nullptr);
    
    if(vk_depth_image_view != VK_NULL_HANDLE){
        vkDestroyImageView(render::context.vk_device, vk_depth_image_view, nullptr);
        vk_depth_image_view = VK_NULL_HANDLE;
    }
    if(vk_depth_image != VK_NULL_HANDLE){
        vmaDestroyImage(render::context.allocator, vk_depth_image, vma_depth_allocation);
        vk_depth_image = VK_NULL_HANDLE;
    }
}

void RenderBuffer::Begin(VkCommandBuffer vk_command_buffer, Swapchain* swapchain, uint32_t swapchain_image_index){
    VkRenderPassBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.pNext = nullptr;
    
    begin_info.renderPass  = vk_render_pass;
    begin_info.framebuffer = vk_framebuffers[swapchain_image_index];
    begin_info.renderArea  = { 0, 0, swapchain->extent_ };
    
    VkClearValue clear_value[2] = {};
    clear_value[0] = {1.0f, 0.0f};
    clear_value[1] = {{0.0f, 0.0f, 0.0f, 0.0f}};
    begin_info.clearValueCount = 2;
    begin_info.pClearValues = clear_value;
    
    vkCmdBeginRenderPass(vk_command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderBuffer::CreateDepthImageAndView(){
    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.flags = 0;
    image_create_info.pNext = nullptr;
    
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    
    image_create_info.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    image_create_info.extent = *(VkExtent3D*)&extent;
    image_create_info.mipLevels = 1;
    
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 1;
    image_create_info.pQueueFamilyIndices = &render::context.graphics_queue.vk_family_index;
    
    image_create_info.tiling  = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.mipLevels   = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmaCreateImage(render::context.allocator, &image_create_info, &allocInfo, &vk_depth_image, &vma_depth_allocation, nullptr);
    
    VkImageSubresourceRange subresource_range{};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.layerCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.levelCount   = 1;
    subresource_range.baseMipLevel = 0;
    
    VkImageViewCreateInfo view_create_info{};
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image = vk_depth_image;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format   = VK_FORMAT_D32_SFLOAT_S8_UINT;
    view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount   = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount     = 1;
    
    vkCreateImageView(render::context.vk_device, &view_create_info, nullptr, &vk_depth_image_view);
}
}
