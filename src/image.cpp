#include "image.h"

namespace ngfx{
Image::Image(ImageExtent extent) : extent_(extent) {
    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.flags = 0;
    image_create_info.pNext = nullptr;
    
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    
    image_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    image_create_info.extent = *(VkExtent3D*)&extent_;
    image_create_info.mipLevels = 1;
    
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 1;
    image_create_info.pQueueFamilyIndices = &ngfx::Context::graphics_queue.vk_family_index;
    
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    
    vkCreateImage(ngfx::Context::vk_device, &image_create_info, nullptr, &image_);
}
Image::~Image(){
    if(view_ != VK_NULL_HANDLE){
        vkDestroyImageView(ngfx::Context::vk_device, view_, nullptr);
    }
    vkDestroyImage(ngfx::Context::vk_device, image_, nullptr);
}

void Image::CreateImage(){
    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.flags = 0;
    image_create_info.pNext = nullptr;
    
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    
    image_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    image_create_info.extent = *(VkExtent3D*)&extent_;
    image_create_info.mipLevels = 1;
    
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 1;
    image_create_info.pQueueFamilyIndices = &ngfx::Context::graphics_queue.vk_family_index;
    
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    
    vkCreateImage(ngfx::Context::vk_device, &image_create_info, nullptr, &image_);
}
void Image::CreateView(){
    VkImageSubresourceRange subresource_range{};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.layerCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseMipLevel = 0;
    
    VkImageViewCreateInfo view_create_info{};
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image = image_;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = 1;
    
    vkCreateImageView(ngfx::Context::vk_device, &view_create_info, nullptr, &view_);
}
void Image::GetMemoryRequirements(VkMemoryRequirements* memory_requirements){
    vkGetImageMemoryRequirements(ngfx::Context::vk_device, image_, memory_requirements);
}
void Image::BindMemory(VkDeviceMemory vk_memory, VkDeviceSize offset){
    vkBindImageMemory(ngfx::Context::vk_device, image_, vk_memory, offset);
}

void Image::WriteDescriptor(VkDescriptorSet descriptor_set, uint32_t binding, uint32_t index){
    VkDescriptorImageInfo image_info{};
    image_info.imageView = view_;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.sampler = VK_NULL_HANDLE;
    
    VkWriteDescriptorSet set_write{};
    set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    set_write.descriptorCount = 1;
    set_write.dstBinding = binding;
    set_write.dstArrayElement = index;
    set_write.dstSet = descriptor_set;
    set_write.pImageInfo = &image_info;
    
    vkUpdateDescriptorSets(Context::vk_device, 1, &set_write, 0, nullptr);
}

// --- TEXTURE SET --- //
VkDescriptorSetLayout texture_set_layout = VK_NULL_HANDLE;
}
