#pragma once
#include "render/context.h"

namespace render{
struct MemoryAllocation;
struct ImageOffset{
    int32_t x;
    int32_t y;
    int32_t z;
};
struct ImageExtent{
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};
struct ImageRegion{
    ImageOffset offset;
    ImageExtent extent;
};
struct TextureInfo{
    ImageExtent extent;
};
class Texture{
public:
    Texture();
    ~Texture();
    
    void Initialize(TextureInfo info);
    void Terminate();
    
    void WriteDescriptor(VkDescriptorSet descriptor_set, uint32_t binding, uint32_t index);
    
    ImageExtent   image_extent;
    VmaAllocation vma_allocation;
    VkImage       vk_image;
    VkImageView   vk_view;
};

class Sampler{
public:
    void Initialize();
    void Terminate();
    
    void WriteDescriptor(VkDescriptorSet descriptor_set, uint32_t binding, uint32_t index);
    
    VkSampler vk_sampler;
};
}
