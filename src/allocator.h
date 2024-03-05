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
class Allocator{
public:
    const MemoryAllocation Malloc(MemoryType desired_memory_type,
                                  size_t size, size_t alignment, uint32_t memory_type_bits);
    void Free(const MemoryAllocation allocation);
    
    std::vector<DeviceMemory> device_allocations_;
};
extern Allocator* allocator;
}
