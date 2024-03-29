#pragma once
#include "render/context.h"

namespace render{
struct DescriptorBinding{
    uint32_t              binding;
    VkDescriptorType      descriptorType;
    uint32_t              descriptorCount;
    VkShaderStageFlags    stageFlags;
    const VkSampler*      pImmutableSamplers;
};
class DescriptorSetLayout{
public:
    void Initialize(std::vector<DescriptorBinding> bindings);
    void Terminate();
    
    VkDescriptorSetLayout vk_descriptor_set_layout;
};
//VkDescriptorSetLayout CreateDescriptorSetLayout(std::vector<DescriptorBinding> bindings);

struct DescriptorSet{
    VkDescriptorSet vk_descriptor_set;
};
class DescriptorAllocator{
public:
    void Initialize();
    void Terminate();
    
    DescriptorSet Allocate(DescriptorSetLayout set_layout);
    
    void Flush();
    
    uint32_t pool_set_count = 3;
    uint32_t active_pool_index = 0;
    std::vector<VkDescriptorPool> descriptor_pools{};
};
extern DescriptorAllocator descriptor_allocator;
}
