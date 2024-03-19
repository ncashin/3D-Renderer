#include "descriptor.h"

namespace engine{
VkDescriptorSetLayout CreateDescriptorSetLayout(std::vector<DescriptorBinding> bindings){
    VkDescriptorSetLayoutCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    printf("%lu\n", bindings.size());
    create_info.bindingCount = (uint32_t)bindings.size();
    create_info.pBindings    = (VkDescriptorSetLayoutBinding*)bindings.data();
    
    VkDescriptorSetLayout layout;
    vkCreateDescriptorSetLayout(render_context->vk_device, &create_info, nullptr, &layout);
    return layout;
}
}
