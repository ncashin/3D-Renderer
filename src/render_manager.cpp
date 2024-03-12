#include "render_manager.h"

namespace engine{
RenderManager::RenderManager(){
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.pNext = nullptr;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    pool_create_info.queueFamilyIndex = render_context->graphics_queue.vk_family_index;
    vkCreateCommandPool(render_context->vk_device, &pool_create_info, nullptr, &primary_graphics_command_pool);
    
    primary_graphics_command_buffers.resize(1);
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.pNext = nullptr;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    allocate_info.commandPool = primary_graphics_command_pool;
    vkAllocateCommandBuffers(render_context->vk_device, &allocate_info,
                             primary_graphics_command_buffers.data());
    
    graphics_record_thread = std::thread(&RenderManager::HandleGraphicsRecording, this);
    submission_thread      = std::thread(&RenderManager::HandleSubmission,        this);
}
RenderManager::~RenderManager(){
    active = false;
    submission_mutex.lock();
    
    VkCommandBuffer* vk_command_buffer = new VkCommandBuffer(VK_NULL_HANDLE);
    RecordingInfo end_record = { vk_command_buffer, [](VkCommandBuffer vk_command_buffer){} };
    graphics_record_queue.emplace_back(end_record);
    
    SubmissionInfo end_submission = { NGFX_SUBMISSION_TYPE_EXIT, nullptr };
    submission_queue.emplace_back(end_submission);
    
    submission_mutex.unlock();
    
    graphics_record_condition_variable.notify_one();
    submission_condition_variable.notify_one();

    graphics_record_thread.join();
    submission_thread.join();
    
    delete vk_command_buffer;
    
    vkDeviceWaitIdle(render_context->vk_device);
    vkDestroyCommandPool(render_context->vk_device, primary_graphics_command_pool, nullptr);
}

void RenderManager::SubmitGraphics(SubmitInfo submit_info,
                                   std::function<void(VkCommandBuffer)> record_function){
    VkCommandBuffer* vk_command_buffer = new VkCommandBuffer(VK_NULL_HANDLE);
    submit_info.vk_command_buffer  = vk_command_buffer;
    
    submission_mutex.lock();
    
    RecordingInfo recording_info = { vk_command_buffer, record_function };
    graphics_record_queue.emplace_back(recording_info);
    
    SubmissionInfo submission_info = { NGFX_SUBMISSION_TYPE_GRAPHICS, new SubmitInfo(submit_info) };
    submission_queue.emplace_back(submission_info);
    
    submission_mutex.unlock();
    
    graphics_record_condition_variable.notify_one();
}
void RenderManager::SubmitCompute(SubmitInfo submit_info,
                                  std::function<void(VkCommandBuffer)> record_function){
    VkCommandBuffer* vk_command_buffer = new VkCommandBuffer(VK_NULL_HANDLE);
    submit_info.vk_command_buffer  = vk_command_buffer;
    
    submission_mutex.lock();
    
    RecordingInfo recording_info = { vk_command_buffer, record_function };
    graphics_record_queue.emplace_back(recording_info);
    
    SubmissionInfo submission_info = { NGFX_SUBMISSION_TYPE_COMPUTE, new SubmitInfo(submit_info) };
    submission_queue.emplace_back(submission_info);
    
    submission_mutex.unlock();
}
void RenderManager::Present(PresentInfo present_info){
    submission_mutex.lock();
    SubmissionInfo submission_info = { NGFX_SUBMISSION_TYPE_PRESENT, new PresentInfo(present_info) };
    submission_queue.emplace_back(submission_info);
    submission_mutex.unlock();
    submission_condition_variable.notify_one();
}

void RenderManager::InsertSubmissionFence(bool* fence){
    submission_mutex.lock();
    SubmissionInfo submission_info = { NGFX_SUBMISSION_TYPE_FENCE, fence };
    submission_queue.emplace_back(submission_info);
    submission_mutex.unlock();
    submission_condition_variable.notify_one();
}
void RenderManager::WaitForFence(bool* submission_fence){
    std::unique_lock<std::mutex> lock(fence_mutex);
    fence_condition_variable.wait(lock, [submission_fence]{
        return *submission_fence;
    });
    *submission_fence = false;
}
void RenderManager::ResetFence(bool& submission_fence){
    fence_mutex.lock();
    submission_fence = false;
    fence_mutex.unlock();
}

void RenderManager::HandleGraphicsRecording(){
    while(active || graphics_record_queue.size() > 0){
        
        std::unique_lock<std::mutex> lock(submission_mutex);
        graphics_record_condition_variable.wait(lock, [this]{
            return graphics_record_queue.size() != 0;
        });
        auto record_info = graphics_record_queue.front();
        graphics_record_queue.pop_front();
        lock.unlock();
        
        vkResetCommandBuffer(primary_graphics_command_buffers[0], 0);
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.pNext = nullptr;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;
        
        vkBeginCommandBuffer (primary_graphics_command_buffers[0], &begin_info);
        record_info.function(primary_graphics_command_buffers[0]);
        vkEndCommandBuffer(primary_graphics_command_buffers[0]);
        
        lock.lock();
        *record_info.vk_command_buffer = primary_graphics_command_buffers[0];
        lock.unlock();
        submission_condition_variable.notify_one();
        
    }
}
void RenderManager::HandleSubmission(){
    while(active || submission_queue.size() > 0){
        std::unique_lock<std::mutex> lock(submission_mutex);
        submission_condition_variable.wait(lock, [this]{
            if(submission_queue.size() == 0) return false;
            if(submission_queue.front().type == NGFX_SUBMISSION_TYPE_GRAPHICS){
                return *((SubmitInfo*)(submission_queue.front().pointer))->vk_command_buffer != VK_NULL_HANDLE;
            }
            return true;
        });
        SubmissionInfo submission = submission_queue.front();
        submission_queue.pop_front();
        lock.unlock();
        
        SubmitInfo*  submit_info = (SubmitInfo*)submission.pointer;
        VkSubmitInfo vk_submit_info{};
        
        PresentInfo* present_info = (PresentInfo*)submission.pointer;
        VkPresentInfoKHR vk_present_info{};
        
        switch(submission.type){
            case NGFX_SUBMISSION_TYPE_GRAPHICS:
                vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                vk_submit_info.pNext = nullptr;
                
                vk_submit_info.waitSemaphoreCount = (uint32_t)submit_info->wait_semaphores.size();
                vk_submit_info.pWaitSemaphores    = submit_info->wait_semaphores.data();
                vk_submit_info.pWaitDstStageMask  = &submit_info->wait_stage_flags;
                
                vk_submit_info.signalSemaphoreCount = (uint32_t)submit_info->signal_semaphores.size();
                vk_submit_info.pSignalSemaphores    = submit_info->signal_semaphores.data();
                
                vk_submit_info.commandBufferCount = 1;
                vk_submit_info.pCommandBuffers    = submit_info->vk_command_buffer;
                
                vkQueueSubmit(render_context->graphics_queue.vk_queue, 1, &vk_submit_info, submit_info->fence);
                delete submit_info->vk_command_buffer;
                delete submit_info;
                break;
            case NGFX_SUBMISSION_TYPE_COMPUTE:
                vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                vk_submit_info.pNext = nullptr;
                
                vk_submit_info.waitSemaphoreCount = (uint32_t)submit_info->wait_semaphores.size();
                vk_submit_info.pWaitSemaphores    = submit_info->wait_semaphores.data();
                vk_submit_info.pWaitDstStageMask  = &submit_info->wait_stage_flags;
                
                vk_submit_info.signalSemaphoreCount = (uint32_t)submit_info->signal_semaphores.size();
                vk_submit_info.pSignalSemaphores    = submit_info->signal_semaphores.data();
                
                vk_submit_info.commandBufferCount = 1;
                vk_submit_info.pCommandBuffers    = submit_info->vk_command_buffer;
                
                vkQueueSubmit(render_context->graphics_queue.vk_queue, 1, &vk_submit_info, submit_info->fence);
                delete submit_info->vk_command_buffer;
                delete submit_info;
                break;
                
            case NGFX_SUBMISSION_TYPE_PRESENT:
                vk_present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                vk_present_info.pNext = nullptr;
                vk_present_info.swapchainCount = (uint32_t)present_info->swapchains.size();
                vk_present_info.pSwapchains    = present_info->swapchains.data();
                vk_present_info.pImageIndices  = present_info->image_indices.data();
                vk_present_info.waitSemaphoreCount = (uint32_t)present_info->wait_semaphores.size();
                vk_present_info.pWaitSemaphores    = present_info->wait_semaphores.data();
                vkQueuePresentKHR(render_context->graphics_queue.vk_queue, &vk_present_info);
                delete present_info;
                break;
                
            case NGFX_SUBMISSION_TYPE_FENCE:
                fence_mutex.lock();
                *(bool*)submission.pointer = true;
                fence_mutex.unlock();
                fence_condition_variable.notify_all();
                break;
                
            case NGFX_SUBMISSION_TYPE_EXIT:
                break;
        }
    }
}
}
