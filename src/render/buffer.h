#pragma once
#include "render/context.h"

namespace render{
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

struct Region{
    size_t offset;
    size_t size;
};
class RegionList{
public:
    RegionList();
    RegionList(size_t offset, size_t size);
    
    bool GetRegion(size_t size, size_t alignment, Region* acquired_region);
    void FreeRegion(Region free_memory);
    
private:
    std::vector<Region> list_;
};


template<typename T>
struct BAllocation{
    uint32_t offset;
    uint32_t count;
};
class TemplateAllocatedBuffer{
public:
     TemplateAllocatedBuffer();
    ~TemplateAllocatedBuffer();
    
    void Initialize(BufferInfo buffer_info);
    void Terminate();
    
    template<typename T>
    BAllocation<T> Allocate(uint32_t count){
        Region region{};
        region_list.GetRegion(sizeof(T) * count, sizeof(T), &region);
        BAllocation<T> allocation;
        allocation.offset = region.offset / sizeof(T);
        allocation.count  = count;
        return allocation;
    }
    template<typename T>
    void Free(BAllocation<T> allocation){
        Region region{sizeof(T) * allocation.offset, sizeof(T) * allocation.count};
        region_list.FreeRegion(region);
    }
    
    RegionList region_list;
    Buffer buffer;
};
extern TemplateAllocatedBuffer device_local_buffer;
}
