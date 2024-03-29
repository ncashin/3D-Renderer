#include "buffer.h"

namespace render{
Buffer::Buffer(){};
Buffer::~Buffer(){}

char* Buffer::Initialize(BufferInfo buffer_info){
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.flags = 0;
    create_info.pNext = nullptr;
    
    create_info.size = buffer_info.size;
    create_info.usage = buffer_info.usage_flags;
    
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &render::context.graphics_queue.vk_family_index;
    
    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.usage = buffer_info.memory_usage;
    alloc_create_info.flags = buffer_info.create_flags;
    VmaAllocationInfo alloc_info{};
    vmaCreateBuffer(render::context.allocator, &create_info, &alloc_create_info,
                    &vk_buffer, &vma_allocation, &alloc_info);
    
    return (char*)alloc_info.pMappedData;
}
void Buffer::Terminate(){
    vmaDestroyBuffer(render::context.allocator, vk_buffer, vma_allocation);
    vk_buffer = VK_NULL_HANDLE;
}

void Buffer::BindAsVertexBuffer(VkCommandBuffer vk_command_buffer, VkDeviceSize offset){
    vkCmdBindVertexBuffers(vk_command_buffer, 0, 1, &vk_buffer, &offset);
}
void Buffer::BindAsIndexBuffer(VkCommandBuffer vk_command_buffer, VkDeviceSize offset){
    vkCmdBindIndexBuffer(vk_command_buffer, vk_buffer, offset, VK_INDEX_TYPE_UINT32);
}

// --- Region List --- //
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

// --- Vertex Buffer --- //
TemplateAllocatedBuffer device_local_buffer;
TemplateAllocatedBuffer:: TemplateAllocatedBuffer(){};
TemplateAllocatedBuffer::~TemplateAllocatedBuffer(){};

void TemplateAllocatedBuffer::Initialize(BufferInfo buffer_info){
    buffer.Initialize(buffer_info);
    region_list = RegionList(0, buffer_info.size);
}
void TemplateAllocatedBuffer::Terminate(){
    buffer.Terminate();
}
}
