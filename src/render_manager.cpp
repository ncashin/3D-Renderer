#include "render_manager.h"

namespace ngfx{

namespace CommandManager{
bool active = false;

VkCommandPool primary_graphics_command_pool{};
std::vector<VkCommandBuffer> primary_graphics_command_buffers{};
std::condition_variable graphics_record_condition_variable{};
std::thread graphics_record_thread{};
std::deque<RecordingInfo> graphics_record_queue{};

std::mutex  submission_mutex{};
std::condition_variable submission_condition_variable{};
std::thread submission_thread{};
std::deque<SubmissionInfo> submission_queue{};

std::mutex fence_mutex{};
std::condition_variable fence_condition_variable{};
}

void CommandManager::Initialize(){
    active = true;
    
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.pNext = nullptr;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    pool_create_info.queueFamilyIndex = Context::graphics_queue.vk_family_index;
    vkCreateCommandPool(Context::vk_device, &pool_create_info, nullptr, &primary_graphics_command_pool);
    
    primary_graphics_command_buffers.resize(1);
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.pNext = nullptr;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    allocate_info.commandPool = primary_graphics_command_pool;
    vkAllocateCommandBuffers(Context::vk_device, &allocate_info,
                             primary_graphics_command_buffers.data());
    
    graphics_record_thread = std::thread(&CommandManager::HandleGraphicsRecording);
    submission_thread      = std::thread(&CommandManager::HandleSubmission);
}
void CommandManager::Terminate(){
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
    
    vkDeviceWaitIdle(Context::vk_device);
    vkDestroyCommandPool(Context::vk_device, primary_graphics_command_pool, nullptr);
}

void CommandManager::SubmitGraphics(SubmitInfo submit_info,
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
void CommandManager::SubmitCompute(SubmitInfo submit_info,
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
void CommandManager::Present(PresentInfo present_info){
    submission_mutex.lock();
    SubmissionInfo submission_info = { NGFX_SUBMISSION_TYPE_PRESENT, new PresentInfo(present_info) };
    submission_queue.emplace_back(submission_info);
    submission_mutex.unlock();
    submission_condition_variable.notify_one();
}

void CommandManager::InsertSubmissionFence(bool* fence){
    submission_mutex.lock();
    SubmissionInfo submission_info = { NGFX_SUBMISSION_TYPE_FENCE, fence };
    submission_queue.emplace_back(submission_info);
    submission_mutex.unlock();
    submission_condition_variable.notify_one();
}
void CommandManager::WaitForSubmissionFence(bool* submission_fence){
    std::unique_lock<std::mutex> lock(fence_mutex);
    fence_condition_variable.wait(lock, [submission_fence]{
        return *submission_fence;
    });
    *submission_fence = false;
}
void CommandManager::ResetSubmissionFence(bool& submission_fence){
    fence_mutex.lock();
    submission_fence = false;
    fence_mutex.unlock();
}

void CommandManager::HandleGraphicsRecording(){
    while(active || graphics_record_queue.size() > 0){
        std::unique_lock<std::mutex> lock(submission_mutex);
        graphics_record_condition_variable.wait(lock, []{
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
void CommandManager::HandleSubmission(){
    while(active || submission_queue.size() > 0){
        std::unique_lock<std::mutex> lock(submission_mutex);
        submission_condition_variable.wait(lock, []{
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
                
                vkQueueSubmit(Context::graphics_queue.vk_queue, 1, &vk_submit_info, submit_info->fence);
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
                
                vkQueueSubmit(Context::graphics_queue.vk_queue, 1, &vk_submit_info, submit_info->fence);
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
                vkQueuePresentKHR(Context::graphics_queue.vk_queue, &vk_present_info);
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
