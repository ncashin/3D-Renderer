#include "staging.h"

namespace ngfx{
namespace StagingManager{
char* mapped_pointer = nullptr;
uint32_t staging_buffer_offset = 0;
Buffer staging_buffer{};

VkCommandPool   vk_command_pool   = VK_NULL_HANDLE;
VkCommandBuffer vk_command_buffer = VK_NULL_HANDLE;

std::mutex upload_active_mutex;
bool upload_active = false;
bool upload_submission_flag = false;
VkFence upload_fence = VK_NULL_HANDLE;
}

void StagingManager::Initialize(){
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.pNext = nullptr;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    pool_create_info.queueFamilyIndex = Context::graphics_queue.vk_family_index;
    vkCreateCommandPool(Context::vk_device, &pool_create_info, nullptr, &vk_command_pool);
    
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.pNext = nullptr;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    allocate_info.commandPool = vk_command_pool;
    vkAllocateCommandBuffers(Context::vk_device, &allocate_info, &vk_command_buffer);
    
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
    
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = 0;
    fence_create_info.pNext = nullptr;
    vkCreateFence(Context::vk_device, &fence_create_info, nullptr, &upload_fence);
}
void StagingManager::Terminate(){
    vkDestroyFence(Context::vk_device, upload_fence, nullptr);
    vkDestroyCommandPool(Context::vk_device, vk_command_pool, nullptr);
    staging_buffer.Terminate();
}

char* StagingManager::UploadToBuffer(size_t upload_size, size_t offset, Buffer* buffer){
    char* buffer_pointer = mapped_pointer + staging_buffer_offset;
    VkBufferCopy buffer_copy{};
    buffer_copy.size = upload_size;
    buffer_copy.srcOffset = staging_buffer_offset;
    buffer_copy.dstOffset = offset;
    
    vkCmdCopyBuffer(vk_command_buffer, staging_buffer.vk_buffer, buffer->vk_buffer, 1, &buffer_copy);
    
    staging_buffer_offset += upload_size;
    return buffer_pointer;
}
char* StagingManager::UploadToImage(){
    
}

void StagingManager::SubmitUpload(SubmitInfo submit_info){
    vkEndCommandBuffer(vk_command_buffer);
    submit_info.fence = upload_fence;
    CommandManager::SubmitGraphics(submit_info, &upload_submission_flag, vk_command_buffer);
}
void StagingManager::AwaitUploadCompletion(){
    upload_active_mutex.lock();
    if(upload_active){
        CommandManager::WaitForSubmissionFence(&upload_submission_flag);
        vkWaitForFences(Context::vk_device, 1, &upload_fence, VK_TRUE, UINT64_MAX);
        upload_active = false;
    }
    upload_active_mutex.unlock();
}
}
