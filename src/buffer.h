#pragma once
#include "render_context.h"

namespace ngfx{
struct BufferInfo{
    size_t size;
    VkBufferUsageFlags usage_flags;
    VmaMemoryUsage memory_usage;
    VmaAllocationCreateFlags create_flags;
};
class Buffer{
public:
    Buffer();
    Buffer(BufferInfo buffer_info);
    ~Buffer();
    
    char* Initialize(BufferInfo buffer_info);
    void  Terminate();
    
    void BindAsVertexBuffer(VkCommandBuffer vk_command_buffer, VkDeviceSize offset);
    void BindAsIndexBuffer (VkCommandBuffer vk_command_buffer, VkDeviceSize offset);
    
    VmaAllocation vma_allocation;
    VkBuffer vk_buffer = VK_NULL_HANDLE;
};

class VertexBuffer{
    
};
extern VertexBuffer* global_vertex_buffer;
}
