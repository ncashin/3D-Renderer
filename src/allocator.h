#pragma once
#include "render_context.h"

namespace engine{
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

enum class MemoryType{
    DeviceLocal,
    Coherent,
};
struct DeviceMemory{
    DeviceMemory(){};
    RegionList region_list;
    uint8_t vk_memory_type_index;
    VkDeviceMemory vk_memory;
    char* mapped_pointer;
};
struct MemoryAllocation{
    uint8_t resource_index;
    size_t offset;
    size_t size;
};
struct Buffer{
    Buffer(){};
    MemoryType memory_type;
    MemoryAllocation memory_allocation;
    RegionList region_list;
    VkBuffer vk_buffer;
};
struct BufferAllocation{
    uint8_t resource_index;
    size_t offset;
    size_t size;
};
struct BufferBarrier{
    VkAccessFlags source_access_flags;
    VkAccessFlags destination_access_flags;
    
    uint32_t source_queue_family;
    uint32_t destination_queue_family;
    
    BufferAllocation buffer_allocation;
};
class Allocator{
public:
    ~Allocator();
    
    const MemoryAllocation Malloc(MemoryType desired_memory_type,
                                  size_t size, size_t alignment, uint32_t memory_type_bits);
    void Free(const MemoryAllocation allocation);
    
    const BufferAllocation Balloc(MemoryType desired_memory_type, size_t size, size_t alignment);
    void Free(const BufferAllocation allocation);
    
    void InsertImageAcquisitionBarrier();
    void InsertImageReleaseBarrier();

    void InsertBufferAcquisitionBarrier();
    void InsertBufferReleaseBarrier();

    std::vector<DeviceMemory> device_allocations_;
    std::vector<Buffer> buffers_;
};
extern Allocator* allocator;
}
