#pragma once
#include "render_context.h"

namespace ngfx{
struct DescriptorBinding{
    uint32_t              binding;
    VkDescriptorType      descriptorType;
    uint32_t              descriptorCount;
    VkShaderStageFlags    stageFlags;
    const VkSampler*      pImmutableSamplers;
};
VkDescriptorSetLayout CreateDescriptorSetLayout(std::vector<DescriptorBinding> bindings);

class DescriptorAllocator{
    
};
}
