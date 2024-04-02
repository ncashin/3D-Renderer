#include "staging.h"

namespace render{
StagingManager staging_manager{};
void StagingManager::Initialize(){
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.pNext = nullptr;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    pool_create_info.queueFamilyIndex = render::context.graphics_queue.vk_family_index;
    vkCreateCommandPool(render::context.vk_device, &pool_create_info, nullptr, &vk_command_pool);
    
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.pNext = nullptr;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    allocate_info.commandPool = vk_command_pool;
    vkAllocateCommandBuffers(render::context.vk_device, &allocate_info, &vk_command_buffer);
    
    mapped_pointer = staging_buffer.Initialize({
        1000000000,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
        VMA_ALLOCATION_CREATE_MAPPED_BIT});
    
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pNext = nullptr;
    begin_info.pInheritanceInfo = nullptr;
    
    vkBeginCommandBuffer(vk_command_buffer, &begin_info);
    
    upload_fence.Initialize(Fence::InitializeUnsignaled);
}
void StagingManager::Terminate(){
    upload_fence.Terminate();
    vkDestroyCommandPool(render::context.vk_device, vk_command_pool, nullptr);
    staging_buffer.Terminate();
}

void* StagingManager::UploadToBuffer(size_t upload_size, size_t offset, Buffer* buffer){
    char* buffer_pointer = mapped_pointer + staging_buffer_offset;
    VkBufferCopy buffer_copy{};
    buffer_copy.size = upload_size;
    buffer_copy.srcOffset = staging_buffer_offset;
    buffer_copy.dstOffset = offset;
    
    vkCmdCopyBuffer(vk_command_buffer, staging_buffer.vk_buffer, buffer->vk_buffer, 1, &buffer_copy);
    
    staging_buffer_offset += upload_size;
    return buffer_pointer;
}
void* StagingManager::UploadToImage (size_t upload_size, Texture* texture){
    char* buffer_pointer = mapped_pointer + staging_buffer_offset;

    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.srcQueueFamilyIndex = 0;
        barrier.dstQueueFamilyIndex = 0;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = texture->vk_image;
        vkCmdPipelineBarrier(vk_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);
    }
    
    VkBufferImageCopy copy{};
    copy.bufferOffset = staging_buffer_offset;
    copy.bufferRowLength   = 100;
    copy.bufferImageHeight = 100;
    
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.layerCount = 1;
    copy.imageSubresource.mipLevel = 0;
    copy.imageOffset = {0, 0, 0};
    copy.imageExtent = {100, 100, 1};
    
    vkCmdCopyBufferToImage(vk_command_buffer, staging_buffer.vk_buffer, 
                           texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
    
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = 0;
        barrier.dstQueueFamilyIndex = 0;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.image = texture->vk_image;
        vkCmdPipelineBarrier(vk_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);
    }
    
    staging_buffer_offset += upload_size;
    return buffer_pointer;
}


void StagingManager::SubmitUpload(SubmitInfo submit_info){
    vkEndCommandBuffer(vk_command_buffer);
    submit_info.fence = &upload_fence;
    render::command_manager.SubmitGraphics(submit_info, vk_command_buffer);
}
void StagingManager::AwaitUploadCompletion(){
    upload_active_mutex.lock();
    if(upload_active){
        render::command_manager.WaitForFence(&upload_fence);
        upload_active = false;
    }
    upload_active_mutex.unlock();
}
}
