#include "render/descriptor.h"

namespace render{
void DescriptorSetLayout::Initialize(std::vector<DescriptorBinding> bindings){
    VkDescriptorSetLayoutCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.bindingCount = (uint32_t)bindings.size();
    create_info.pBindings    = (VkDescriptorSetLayoutBinding*)bindings.data();
    
    vkCreateDescriptorSetLayout(render::context.vk_device, &create_info, nullptr, &vk_descriptor_set_layout);
}
void DescriptorSetLayout::Terminate(){
    vkDestroyDescriptorSetLayout(render::context.vk_device, vk_descriptor_set_layout, nullptr);
}

DescriptorAllocator descriptor_allocator{};
void DescriptorAllocator::Initialize(){
    std::vector<std::pair<VkDescriptorType,float>> pool_sizes = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
    };
    std::vector<VkDescriptorPoolSize> sizes;
    sizes.reserve(pool_sizes.size());
    for (auto size : pool_sizes) {
        sizes.push_back({ size.first, uint32_t(size.second * pool_set_count) });
    }
    
    VkDescriptorPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.flags = 0;
    create_info.pNext = nullptr;
    create_info.maxSets = pool_set_count;
    create_info.poolSizeCount = (uint32_t)sizes.size();
    create_info.pPoolSizes    = sizes.data();
    VkDescriptorPool new_pool{};
    vkCreateDescriptorPool(render::context.vk_device,&create_info, nullptr, &new_pool);
    descriptor_pools.emplace_back(new_pool);
}
void DescriptorAllocator::Terminate(){
    for(VkDescriptorPool pool : descriptor_pools){
        vkDestroyDescriptorPool(render::context.vk_device, pool, nullptr);
    }
}

DescriptorSet DescriptorAllocator::Allocate(DescriptorSetLayout set_layout){
    VkDescriptorSetAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = nullptr;
    allocate_info.descriptorPool = descriptor_pools[active_pool_index];
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &set_layout.vk_descriptor_set_layout;
    DescriptorSet descriptor_set{};
    VkResult vk_result = vkAllocateDescriptorSets(render::context.vk_device, &allocate_info, &descriptor_set.vk_descriptor_set);
    if(vk_result == VK_SUCCESS){
        return descriptor_set;
    }
    while(++active_pool_index < descriptor_pools.size()){
        allocate_info.descriptorPool = descriptor_pools[active_pool_index];
        vk_result = vkAllocateDescriptorSets(render::context.vk_device, &allocate_info, &descriptor_set.vk_descriptor_set);
        if(vk_result == VK_SUCCESS){
            return descriptor_set;
        }
    }
    
    std::vector<std::pair<VkDescriptorType,float>> pool_sizes = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
    };
    std::vector<VkDescriptorPoolSize> sizes;
    sizes.reserve(pool_sizes.size());
    for (auto size : pool_sizes) {
        sizes.push_back({ size.first, uint32_t(size.second * 1000) });
    }
    
    VkDescriptorPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.flags = 0;
    create_info.pNext = nullptr;
    create_info.maxSets = 1000;
    create_info.poolSizeCount = (uint32_t)sizes.size();
    create_info.pPoolSizes    = sizes.data();
    VkDescriptorPool new_pool{};
    vkCreateDescriptorPool(render::context.vk_device,&create_info, nullptr, &new_pool);
    descriptor_pools.emplace_back(new_pool);
    
    allocate_info.descriptorPool = new_pool;
    vk_result = vkAllocateDescriptorSets(render::context.vk_device, &allocate_info, &descriptor_set.vk_descriptor_set);
    if(vk_result == VK_SUCCESS){
        return descriptor_set;
    }
}

void DescriptorAllocator::Flush(){
    active_pool_index = 0;
    for(VkDescriptorPool pool : descriptor_pools){
        vkResetDescriptorPool(render::context.vk_device, pool, 0);
    }
}
}
