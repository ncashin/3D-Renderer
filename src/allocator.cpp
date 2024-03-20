#include "allocator.h"

namespace ngfx{
RegionList::RegionList(){};
RegionList::RegionList(size_t offset, size_t size) : list_({{offset, size}}) {}
bool RegionList::GetRegion(size_t size, size_t alignment, Region* acquired_region){
    size_t padding = 0;
    for(Region& memory : list_){
        if(alignment != 0 && memory.offset % alignment){
            padding = alignment - (memory.offset % alignment);
        }
        if(memory.size < size + padding){
            continue;
        }
        *acquired_region = Region{ memory.offset, size + padding };
        memory.size   -= size + padding;
        memory.offset += size + padding;
        return true;
    }
    return false;
}
void RegionList::FreeRegion(Region free_memory){
    size_t index = list_.size() / 2;
    while(index / 2 != 0){
        if     (free_memory.offset < list_[index].offset){ index -= index / 2;}
        else if(free_memory.offset > list_[index].offset){ index += index / 2; }
    }
    if(free_memory.offset > list_[index].offset){ index++; }
    
    if(free_memory.offset + free_memory.size == list_[index].offset){
        list_[index].offset = free_memory.offset;
        list_[index].size += free_memory.size;
        
        if(index - 1 != UINT32_MAX && list_[index - 1].offset + list_[index - 1].size == free_memory.offset){
            list_[index - 1].size += list_[index].size;
            list_.erase(list_.begin() + index);
        } return;
    }
    else if(list_[index].offset + list_[index].size == free_memory.offset){
        list_[index].size += free_memory.size; return;
    }
    list_.insert(list_.begin() + index, free_memory);
}


std::vector<DeviceMemory> Allocator::device_allocations_{};
void Allocator::Initialize(){};
void Allocator::Terminate(){
    for(DeviceMemory memory : device_allocations_){
        vkFreeMemory(Context::vk_device, memory.vk_memory, nullptr);
    }
}

MemoryAllocation Allocator::Malloc(NGFX_MemoryType desired_memory_type,
                                   VkMemoryRequirements memory_requirements){
    return Malloc(desired_memory_type,
                  memory_requirements.size, memory_requirements.alignment, memory_requirements.memoryTypeBits);
}
MemoryAllocation Allocator::Malloc(NGFX_MemoryType desired_memory_type,
                                   size_t size, size_t alignment, uint32_t memory_type_bits){
    VkMemoryPropertyFlags required_properties{};
    switch(desired_memory_type){
        case NGFX_MemoryType::NGFX_MEMORY_TYPE_DEVICE_LOCAL:
            required_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;
        case NGFX_MemoryType::NGFX_MEMORY_TYPE_COHERENT:
            required_properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;
    }
    
    VkPhysicalDeviceMemoryProperties memory_properties{};
    vkGetPhysicalDeviceMemoryProperties(Context::vk_physical_device, &memory_properties);
    for(uint8_t i = 0; i < device_allocations_.size(); i++){
        DeviceMemory& memory = device_allocations_[i];
        if(!(memory_type_bits & (1 << memory.vk_memory_type_index)) ||
           (memory_properties.memoryTypes[memory.vk_memory_type_index].propertyFlags
            & required_properties) != required_properties){
            continue;
        }
        Region memory_region{};
        if(memory.region_list.GetRegion(size, alignment, &memory_region)){
            return MemoryAllocation{ i, memory_region.offset, memory_region.size };
        }
    }
    
    for(uint8_t i = 0; i < memory_properties.memoryTypeCount; i++){
        if(!(memory_type_bits & (1 << i)) ||
           (memory_properties.memoryTypes[i].propertyFlags & required_properties) != required_properties){
            continue;
        }
        uint8_t heap_index = memory_properties.memoryTypes[i].heapIndex;
        DeviceHeap& heap = Context::device_heaps[heap_index];
        if(heap.allocated_size + size > heap.maximum_allocated_size){
            continue;
        }
        VkDeviceSize new_allocation_size = size;
        if(new_allocation_size < heap.minimum_allocation &&
           heap.allocated_size + heap.minimum_allocation < heap.maximum_allocated_size){
            new_allocation_size = heap.minimum_allocation;
        }
        
        VkMemoryAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.pNext = nullptr;
        allocate_info.allocationSize = new_allocation_size;
        allocate_info.memoryTypeIndex = i;
        
        uint16_t index = device_allocations_.size();
        DeviceMemory new_memory{};
        new_memory.region_list = { 0, new_allocation_size };
        new_memory.vk_memory_type_index = i;
        vkAllocateMemory(Context::vk_device, &allocate_info, nullptr, &new_memory.vk_memory);
        if(desired_memory_type == NGFX_MemoryType::NGFX_MEMORY_TYPE_COHERENT){
            vkMapMemory(Context::vk_device,
                        new_memory.vk_memory, 0, VK_WHOLE_SIZE, 0, (void**)&new_memory.mapped_pointer);
        }
        uint8_t new_memory_index = (uint8_t)device_allocations_.size();
        device_allocations_.emplace_back(new_memory);
        return MemoryAllocation{ new_memory_index, 0, size };
    }
    return MemoryAllocation{};
}
void Allocator::Free(MemoryAllocation allocation){
    device_allocations_[allocation.resource_index].region_list.FreeRegion({
        allocation.offset, allocation.size,
    });
}

void Allocator::Allocate(NGFX_MemoryType memory_type, Buffer* buffer){
    VkMemoryRequirements memory_requirements;
    buffer->GetMemoryRequirements(&memory_requirements);
    auto allocation = Malloc(memory_type, memory_requirements);
    buffer->memory_allocation = allocation;
    buffer->BindMemory(device_allocations_[allocation.resource_index].vk_memory, allocation.offset);
}
void Allocator::Allocate(NGFX_MemoryType memory_type, Image* image){
    VkMemoryRequirements memory_requirements;
    image->GetMemoryRequirements(&memory_requirements);
    auto allocation = Malloc(memory_type, memory_requirements);
    image->memory_allocation = allocation;
    image->BindMemory(device_allocations_[allocation.resource_index].vk_memory,
                      allocation.offset + (allocation.offset % memory_requirements.alignment));
}

void Allocator::Free(Buffer* buffer){
    Free(buffer->memory_allocation);
}
void Allocator::Free(Image*  image){
    Free(image->memory_allocation);
}

void Allocator::Map(Buffer* buffer, char** pointer){
    auto index = buffer->memory_allocation.resource_index;
    *pointer = device_allocations_[index].mapped_pointer + buffer->memory_allocation.offset;
}
}
