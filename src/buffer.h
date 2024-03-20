#pragma once
#include "render_context.h"
#include "allocation.h"

namespace ngfx{
class Buffer{
public:
    Buffer(size_t size);
    ~Buffer();
    
    void GetMemoryRequirements(VkMemoryRequirements* memory_requirements);
    void BindMemory(VkDeviceMemory vk_memory, VkDeviceSize offset);
    
    void BindAsVertexBuffer(VkCommandBuffer vk_command_buffer, VkDeviceSize offset);
    void BindAsIndexBuffer (VkCommandBuffer vk_command_buffer, VkDeviceSize offset);

    MemoryAllocation memory_allocation;
    VkBuffer vk_buffer;
};

class VertexBuffer{
    
};
extern VertexBuffer* global_vertex_buffer;
}
