#include "buffer.h"

namespace engine{
Buffer::Buffer(size_t size){
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.flags = 0;
    create_info.pNext = nullptr;
    
    create_info.size = size;
    create_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT   | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &render_context->graphics_queue.vk_family_index;
    
    vkCreateBuffer(render_context->vk_device, &create_info, nullptr, &vk_buffer);
}
Buffer::~Buffer(){
    vkDestroyBuffer(render_context->vk_device, vk_buffer, nullptr);
}

void Buffer::GetMemoryRequirements(VkMemoryRequirements* memory_requirements){
    vkGetBufferMemoryRequirements(render_context->vk_device, vk_buffer, memory_requirements);
}
void Buffer::BindMemory(VkDeviceMemory vk_memory, VkDeviceSize offset){
    vkBindBufferMemory(render_context->vk_device, vk_buffer, vk_memory, offset);
}

void Buffer::BindAsVertexBuffer(VkCommandBuffer vk_command_buffer, VkDeviceSize offset){
    vkCmdBindVertexBuffers(vk_command_buffer, 0, 1, &vk_buffer, &offset);
}
void Buffer::BindAsIndexBuffer(VkCommandBuffer vk_command_buffer, VkDeviceSize offset){
    vkCmdBindIndexBuffer(vk_command_buffer, vk_buffer, offset, VK_INDEX_TYPE_UINT32);
}
}
