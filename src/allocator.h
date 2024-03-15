#pragma once
#include "render_context.h"

#include "allocation.h"

#include "buffer.h"
#include "image.h"

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
struct DeviceMemory{
    DeviceMemory(){};
    RegionList region_list;
    uint8_t vk_memory_type_index;
    VkDeviceMemory vk_memory;
    char* mapped_pointer;
};
class Allocator{
public:
    ~Allocator();
    
    MemoryAllocation Malloc(NGFX_MemoryType desired_memory_type,
                            VkMemoryRequirements memory_requirements);
    MemoryAllocation Malloc(NGFX_MemoryType desired_memory_type,
                            size_t size, size_t alignment, uint32_t memory_type_bits);
    void Free(MemoryAllocation allocation);
    
    void Allocate(NGFX_MemoryType memory_type, Buffer* buffer);
    void Allocate(NGFX_MemoryType memory_type, Image*  image);

    void Free(Buffer* buffer);
    void Free(Image*  image);
    
    void Map(Buffer* buffer, char** pointer);
    
    std::vector<DeviceMemory> device_allocations_;
};
extern Allocator* allocator;
}
