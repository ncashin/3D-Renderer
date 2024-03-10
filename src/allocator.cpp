#include "allocator.h"

namespace engine{
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


Allocator* allocator = nullptr;
Allocator::~Allocator(){
    for(Buffer buffer : buffers_){
        vkDestroyBuffer(render_context->vk_device, buffer.vk_buffer, nullptr);
    }
    for(DeviceMemory memory : device_allocations_){
        vkFreeMemory(render_context->vk_device, memory.vk_memory, nullptr);
    }
}

const MemoryAllocation Allocator::Malloc(MemoryType desired_memory_type,
                                         size_t size, size_t alignment, uint32_t memory_type_bits){
    VkMemoryPropertyFlags required_properties{};
    switch(desired_memory_type){
        case MemoryType::DeviceLocal: required_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;  break;
        case MemoryType::Coherent:    required_properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; break;
    }
    
    VkPhysicalDeviceMemoryProperties memory_properties{};
    vkGetPhysicalDeviceMemoryProperties(render_context->vk_physical_device, &memory_properties);
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
        DeviceHeap& heap = render_context->device_heaps[heap_index];
        if(heap.allocated_size + size > heap.maximum_allocated_size){
            return;
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
        vkAllocateMemory(render_context->vk_device, &allocate_info, nullptr, &new_memory.vk_memory);
        if(desired_memory_type == MemoryType::Coherent){
            vkMapMemory(render_context->vk_device,
                        new_memory.vk_memory, 0, VK_WHOLE_SIZE, 0, (void**)&new_memory.mapped_pointer);
        }
        uint8_t new_memory_index = (uint8_t)device_allocations_.size();
        device_allocations_.emplace_back(new_memory);
        return MemoryAllocation{ new_memory_index, 0, size };
    }
    return MemoryAllocation{};
}
void Allocator::Free(const MemoryAllocation allocation){
    device_allocations_[allocation.resource_index].region_list.FreeRegion({
        allocation.offset, allocation.size,
    });
}

const BufferAllocation Allocator::Balloc(MemoryType desired_memory_type, size_t size, size_t alignment){
    for(uint8_t i = 0; i < buffers_.size(); i++){
        Buffer& buffer = buffers_[i];
        if(buffer.memory_type != desired_memory_type){
            continue;
        }
        Region region{};
        if(buffer.region_list.GetRegion(size, alignment, &region)){
            continue;
        }
        return BufferAllocation{
            i,
            region.offset,
            region.size,
        };
    }
    
    VkDeviceSize buffer_size = 8000000;
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.flags = 0;
    create_info.pNext = nullptr;
    
    create_info.size = buffer_size;
    create_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &render_context->graphics_queue.vk_family_index;
    
    Buffer buffer{};
    vkCreateBuffer(render_context->vk_device, &create_info, nullptr, &buffer.vk_buffer);
    
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(render_context->vk_device, buffer.vk_buffer, &requirements);
    auto memory_allocation = Malloc(desired_memory_type,
                                    requirements.size, requirements.alignment, requirements.memoryTypeBits);
    vkBindBufferMemory(render_context->vk_device, buffer.vk_buffer,
                       device_allocations_[memory_allocation.resource_index].vk_memory,
                       memory_allocation.offset + (memory_allocation.offset % requirements.alignment));
    buffer.memory_allocation = memory_allocation;
    buffer.memory_type = desired_memory_type;
    
    uint8_t index = buffers_.size();
    buffers_.emplace_back(buffer);
    return BufferAllocation{
        index,
        0,
        size,
    };
}
void Allocator::Free(const BufferAllocation allocation){
    buffers_[allocation.resource_index].region_list.FreeRegion({
        allocation.offset, allocation.size,
    });
}
}
