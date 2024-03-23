#include "render_manager.h"

namespace ngfx{

namespace CommandManager{
std::mutex  submission_mutex{};
std::condition_variable submission_condition_variable{};

uint32_t to_submit_id = 0;
uint32_t submit_id = 0;

VkCommandPool primary_graphics_command_pool{};
std::vector<VkCommandBuffer> primary_graphics_command_buffers{};
}

void CommandManager::Initialize(){
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
}
void CommandManager::Terminate(){
    vkDeviceWaitIdle(Context::vk_device);
    vkDestroyCommandPool(Context::vk_device, primary_graphics_command_pool, nullptr);
}

void CommandManager::SubmitGraphics(SubmitInfo submit_info, bool* submission_flag,
                    VkCommandBuffer vk_command_buffer){
    submission_mutex.lock();
    uint32_t id = submit_id++;
    submission_mutex.unlock();    
    ThreadPool::Dispatch([submit_info, vk_command_buffer, submission_flag, id]{
        submission_mutex.lock();
        if(id != to_submit_id){
            submission_mutex.unlock();
            return ThreadPool::ReturnState::REDISPATCH;
        }
        submission_mutex.unlock();
        
        VkSubmitInfo vk_submit_info{};
        vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        vk_submit_info.pNext = nullptr;
        
        vk_submit_info.waitSemaphoreCount = (uint32_t)submit_info.wait_semaphores.size();
        vk_submit_info.pWaitSemaphores    = submit_info.wait_semaphores.data();
        vk_submit_info.pWaitDstStageMask  = &submit_info.wait_stage_flags;
        
        vk_submit_info.signalSemaphoreCount = (uint32_t)submit_info.signal_semaphores.size();
        vk_submit_info.pSignalSemaphores    = submit_info.signal_semaphores.data();
        
        vk_submit_info.commandBufferCount = 1;
        vk_submit_info.pCommandBuffers    = &vk_command_buffer;
        
        vkQueueSubmit(Context::graphics_queue.vk_queue, 1, &vk_submit_info, submit_info.fence);
        
        submission_mutex.lock();
        ++to_submit_id;
        if(submission_flag != nullptr){
            *submission_flag = true;
            submission_condition_variable.notify_all();
        }
        submission_mutex.unlock();
        
        return ThreadPool::ReturnState::COMPLETE;
    });
}
void CommandManager::SubmitGraphics(SubmitInfo submit_info, bool* submission_flag,
                                   std::function<void(VkCommandBuffer)> record_function){
    VkCommandBuffer vk_command_buffer = primary_graphics_command_buffers[0];
    submission_mutex.lock();
    uint32_t id = submit_id++;
    submission_mutex.unlock();
    
    ThreadPool::Dispatch([vk_command_buffer, record_function, submit_info, submission_flag, id]{
        vkResetCommandBuffer(vk_command_buffer, 0);
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.pNext = nullptr;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;
        
        vkBeginCommandBuffer(vk_command_buffer, &begin_info);
        record_function(vk_command_buffer);
        vkEndCommandBuffer(vk_command_buffer);
        
        ThreadPool::Dispatch([submit_info, vk_command_buffer, submission_flag, id]{
            submission_mutex.lock();
            if(id != to_submit_id){
                submission_mutex.unlock();
                return ThreadPool::ReturnState::REDISPATCH;
            }
            submission_mutex.unlock();
            
            VkSubmitInfo vk_submit_info{};
            vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            vk_submit_info.pNext = nullptr;
            
            vk_submit_info.waitSemaphoreCount = (uint32_t)submit_info.wait_semaphores.size();
            vk_submit_info.pWaitSemaphores    = submit_info.wait_semaphores.data();
            vk_submit_info.pWaitDstStageMask  = &submit_info.wait_stage_flags;
            
            vk_submit_info.signalSemaphoreCount = (uint32_t)submit_info.signal_semaphores.size();
            vk_submit_info.pSignalSemaphores    = submit_info.signal_semaphores.data();
            
            vk_submit_info.commandBufferCount = 1;
            vk_submit_info.pCommandBuffers    = &vk_command_buffer;
            
            vkQueueSubmit(Context::graphics_queue.vk_queue, 1, &vk_submit_info, submit_info.fence);
            
            submission_mutex.lock();
            ++to_submit_id;
            if(submission_flag != nullptr){
                *submission_flag = true;
                submission_condition_variable.notify_all();
            }
            submission_mutex.unlock();
            
            return ThreadPool::ReturnState::COMPLETE;
        });
        
        return ThreadPool::ReturnState::COMPLETE;
    });
}
void CommandManager::Present(PresentInfo present_info){
    uint32_t id = submit_id++;
    ThreadPool::Dispatch([present_info, id]{
        submission_mutex.lock();
        if(id != to_submit_id){
            submission_mutex.unlock();
            return ThreadPool::ReturnState::REDISPATCH;
        }
        submission_mutex.unlock();
        
        VkPresentInfoKHR vk_present_info{};
        vk_present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        vk_present_info.pNext = nullptr;
        
        vk_present_info.swapchainCount = (uint32_t)present_info.swapchains.size();
        vk_present_info.pSwapchains    = present_info.swapchains.data();
        vk_present_info.pImageIndices  = present_info.image_indices.data();
        
        vk_present_info.waitSemaphoreCount = (uint32_t)present_info.wait_semaphores.size();
        vk_present_info.pWaitSemaphores    = present_info.wait_semaphores.data();
        
        vkQueuePresentKHR(Context::graphics_queue.vk_queue, &vk_present_info);
        
        submission_mutex.lock();
        ++to_submit_id;
        submission_mutex.unlock();
        
        return ThreadPool::ReturnState::COMPLETE;
    });
}

void CommandManager::WaitForSubmissionFence(bool* submission_fence){
    std::unique_lock<std::mutex> lock(submission_mutex);
    submission_condition_variable.wait(lock, [submission_fence]{
        return *submission_fence;
    });
    *submission_fence = false;
}
}
