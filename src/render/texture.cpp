#include "render/texture.h"

namespace render{
Texture::Texture(){};
Texture::~Texture(){}

void Texture::Initialize(TextureInfo info){
    image_extent = info.extent;
    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.flags = 0;
    image_create_info.pNext = nullptr;
    
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    
    image_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    image_create_info.extent = *(VkExtent3D*)&image_extent;
    image_create_info.mipLevels = 1;
    
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 1;
    image_create_info.pQueueFamilyIndices = &render::context.graphics_queue.vk_family_index;
    
    image_create_info.tiling  = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.mipLevels   = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmaCreateImage(render::context.allocator, &image_create_info, &allocInfo, &vk_image, &vma_allocation, nullptr);
    
    VkImageSubresourceRange subresource_range{};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.layerCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseMipLevel = 0;
    
    VkImageViewCreateInfo view_create_info{};
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image = vk_image;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format   = VK_FORMAT_R8G8B8A8_SRGB;
    view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount   = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount     = 1;
    
    vkCreateImageView(render::context.vk_device, &view_create_info, nullptr, &vk_view);
}
void Texture::Terminate(){
    if(vk_view != VK_NULL_HANDLE){
        vkDestroyImageView(render::context.vk_device, vk_view, nullptr);
        vk_view = VK_NULL_HANDLE;
    }
    if(vk_image != VK_NULL_HANDLE){
        vmaDestroyImage(render::context.allocator, vk_image, vma_allocation);
        vk_image = VK_NULL_HANDLE;
    }
}

void Texture::WriteDescriptor(VkDescriptorSet descriptor_set, uint32_t binding, uint32_t index){
    VkDescriptorImageInfo image_info{};
    image_info.imageView = vk_view;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.sampler = VK_NULL_HANDLE;
    
    VkWriteDescriptorSet set_write{};
    set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    set_write.descriptorCount = 1;
    set_write.dstBinding      = binding;
    set_write.dstArrayElement = index;
    set_write.dstSet     = descriptor_set;
    set_write.pImageInfo = &image_info;
    
    vkUpdateDescriptorSets(render::context.vk_device, 1, &set_write, 0, nullptr);
}


void Sampler::Initialize(){
    VkSamplerCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.flags = 0;
    create_info.pNext = nullptr;
    //create_info.borderColor = {};
    //create_info.
    vkCreateSampler(render::context.vk_device, &create_info, nullptr, &vk_sampler);
}
void Sampler::Terminate(){
    vkDestroySampler(render::context.vk_device, vk_sampler, nullptr);
}

void Sampler::WriteDescriptor(VkDescriptorSet descriptor_set, uint32_t binding, uint32_t index){
    VkDescriptorImageInfo image_info{};
    image_info.imageView   = VK_NULL_HANDLE;
    image_info.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.sampler = vk_sampler;
    
    VkWriteDescriptorSet set_write{};
    set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    set_write.descriptorCount = 1;
    set_write.dstBinding      = binding;
    set_write.dstArrayElement = index;
    set_write.dstSet     = descriptor_set;
    set_write.pImageInfo = &image_info;
    
    vkUpdateDescriptorSets(render::context.vk_device, 1, &set_write, 0, nullptr);
}
}
