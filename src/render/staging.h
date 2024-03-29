#pragma once
#include "render/buffer.h"
#include "render/texture.h"
#include "render/command.h"

namespace render{
class StagingManager{
public:
    void Initialize();
    void Terminate();
    
    template<typename T>
    char* UploadToBAllocation(TemplateAllocatedBuffer template_buffer, BAllocation<T> allocation){
        return UploadToBuffer(allocation.count * sizeof(T), allocation.offset * sizeof(T), &template_buffer.buffer);
    }
    char* UploadToBuffer(size_t upload_size, size_t offset, Buffer*  buffer);
    char* UploadToImage (size_t upload_size, Texture* texture);
    
    void SubmitUpload(SubmitInfo submit_info);
    void AwaitUploadCompletion();
    
    char* mapped_pointer = nullptr;
    uint32_t staging_buffer_offset = 0;
    Buffer staging_buffer{};

    VkCommandPool   vk_command_pool   = VK_NULL_HANDLE;
    VkCommandBuffer vk_command_buffer = VK_NULL_HANDLE;

    std::mutex upload_active_mutex;
    bool upload_active = false;
    bool upload_submission_flag = false;
    Fence upload_fence{};
};
extern StagingManager staging_manager;
}
