#include "buffer.h"

namespace ngfx{
Buffer::Buffer(){};
Buffer::Buffer(BufferInfo buffer_info){
    Initialize(buffer_info);
}
Buffer::~Buffer(){
    if(vk_buffer != VK_NULL_HANDLE){
        Terminate();
    }
}

char* Buffer::Initialize(BufferInfo buffer_info){
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.flags = 0;
    create_info.pNext = nullptr;
    
    create_info.size = buffer_info.size;
    create_info.usage = buffer_info.usage_flags;
    
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &ngfx::Context::graphics_queue.vk_family_index;
    
    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.usage = buffer_info.memory_usage;
    alloc_create_info.flags = buffer_info.create_flags;
    VmaAllocationInfo alloc_info{};
    vmaCreateBuffer(Context::allocator, &create_info, &alloc_create_info,
                    &vk_buffer, &vma_allocation, &alloc_info);
    
    return (char*)alloc_info.pMappedData;
}
void Buffer::Terminate(){
    vmaDestroyBuffer(Context::allocator, vk_buffer, vma_allocation);
    vk_buffer = VK_NULL_HANDLE;
}

void Buffer::BindAsVertexBuffer(VkCommandBuffer vk_command_buffer, VkDeviceSize offset){
    vkCmdBindVertexBuffers(vk_command_buffer, 0, 1, &vk_buffer, &offset);
}
void Buffer::BindAsIndexBuffer(VkCommandBuffer vk_command_buffer, VkDeviceSize offset){
    vkCmdBindIndexBuffer(vk_command_buffer, vk_buffer, offset, VK_INDEX_TYPE_UINT32);
}
}
