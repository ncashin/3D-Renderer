#include "render/command.h"

namespace render{
void Semaphore::Initialize(){
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;
    vkCreateSemaphore(render::context.vk_device, &semaphore_create_info, nullptr, &vk_semaphore);
}
void Semaphore::Terminate(){
    vkDestroySemaphore(render::context.vk_device, vk_semaphore, nullptr);
}

void Fence::Initialize(FenceInitializationState initialization_state){
    submission_flag = initialization_state;
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = initialization_state;
    vkCreateFence(render::context.vk_device, &fence_create_info, nullptr, &vk_fence);
}
void Fence::Terminate(){
    vkDestroyFence(render::context.vk_device, vk_fence, nullptr);
}

CommandManager command_manager{};
void CommandManager::Initialize(){
    submit_id = 0;
    to_submit_id = 0;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.pNext = nullptr;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    pool_create_info.queueFamilyIndex = render::context.graphics_queue.vk_family_index;
    vkCreateCommandPool(render::context.vk_device, &pool_create_info, nullptr, &primary_graphics_command_pool);
    
    primary_graphics_command_buffers.resize(1);
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.pNext = nullptr;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    allocate_info.commandPool = primary_graphics_command_pool;
    vkAllocateCommandBuffers(render::context.vk_device, &allocate_info,
                             primary_graphics_command_buffers.data());
}
void CommandManager::Terminate(){
    vkDeviceWaitIdle(render::context.vk_device);
    vkDestroyCommandPool(render::context.vk_device, primary_graphics_command_pool, nullptr);
}

CommandBuffer* CommandManager::RecordAsync(std::function<void(VkCommandBuffer)> record_function){
    CommandBuffer* command_buffer = new CommandBuffer{};
    command_buffer->vk_command_buffer = primary_graphics_command_buffers[0];
    core::threadpool.Dispatch([command_buffer, record_function]{
        vkResetCommandBuffer(command_buffer->vk_command_buffer, 0);
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.pNext = nullptr;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;
        vkBeginCommandBuffer(command_buffer->vk_command_buffer, &begin_info);
        record_function(command_buffer->vk_command_buffer);
        
        command_buffer->record_complete_count++;
        return core::Threadpool::TASK_COMPLETE;
    });
    return command_buffer;
}
void CommandManager::SubmitAsync(SubmitInfo submit_info, CommandBuffer* command_buffer){
    core::threadpool.Dispatch([this, submit_info, command_buffer]{
        if(command_buffer->record_count != command_buffer->record_complete_count){
            return core::Threadpool::TASK_NOT_READY;
        }
        vkEndCommandBuffer(command_buffer->vk_command_buffer);

        VkSubmitInfo vk_submit_info{};
        vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        vk_submit_info.pNext = nullptr;
        
        vk_submit_info.waitSemaphoreCount = (uint32_t)submit_info.wait_semaphores.size();
        vk_submit_info.pWaitSemaphores    = (VkSemaphore*)submit_info.wait_semaphores.data();
        vk_submit_info.pWaitDstStageMask  = &submit_info.wait_stage_flags;
        
        vk_submit_info.signalSemaphoreCount = (uint32_t)submit_info.signal_semaphores.size();
        vk_submit_info.pSignalSemaphores    = (VkSemaphore*)submit_info.signal_semaphores.data();
        
        vk_submit_info.commandBufferCount = 1;
        vk_submit_info.pCommandBuffers    = &command_buffer->vk_command_buffer;
        
        vkQueueSubmit(render::context.graphics_queue.vk_queue, 1, &vk_submit_info, submit_info.fence->vk_fence);

        return core::Threadpool::TASK_COMPLETE;
    });
}


void CommandManager::SubmitGraphics(SubmitInfo submit_info, VkCommandBuffer vk_command_buffer){
    uint32_t id = submit_id++;
    core::threadpool.Dispatch([this, submit_info, vk_command_buffer, id]{
        if(id != to_submit_id){
            return core::Threadpool::TASK_NOT_READY;
        }
        //vkEndCommandBuffer(command_buffer->vk_command_buffer);

        VkSubmitInfo vk_submit_info{};
        vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        vk_submit_info.pNext = nullptr;
        
        vk_submit_info.waitSemaphoreCount = (uint32_t)submit_info.wait_semaphores.size();
        vk_submit_info.pWaitSemaphores    = (VkSemaphore*)submit_info.wait_semaphores.data();
        vk_submit_info.pWaitDstStageMask  = &submit_info.wait_stage_flags;
        
        vk_submit_info.signalSemaphoreCount = (uint32_t)submit_info.signal_semaphores.size();
        vk_submit_info.pSignalSemaphores    = (VkSemaphore*)submit_info.signal_semaphores.data();
        
        vk_submit_info.commandBufferCount = 1;
        vk_submit_info.pCommandBuffers    = &vk_command_buffer;
        
        vkQueueSubmit(render::context.graphics_queue.vk_queue, 1, &vk_submit_info, submit_info.fence->vk_fence);
        
        ++to_submit_id;
        if(submit_info.fence != nullptr){
            submit_info.fence->submission_flag = true;
            submission_condition_variable.notify_all();
        }
        
        return core::Threadpool::TASK_COMPLETE;
    });
}
void CommandManager::SubmitGraphics(SubmitInfo submit_info,
                                   std::function<void(VkCommandBuffer)> record_function){
    VkCommandBuffer vk_command_buffer = primary_graphics_command_buffers[0];
    uint32_t id = submit_id++;
    
    core::threadpool.Dispatch([this, vk_command_buffer, record_function, submit_info, id]{
        vkResetCommandBuffer(vk_command_buffer, 0);
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.pNext = nullptr;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;
        
        vkBeginCommandBuffer(vk_command_buffer, &begin_info);
        record_function(vk_command_buffer);
        vkEndCommandBuffer(vk_command_buffer);

        core::threadpool.Dispatch([this, submit_info, vk_command_buffer, id]{
            if(id != to_submit_id){
                return core::Threadpool::TASK_NOT_READY;
            }
            
            VkSubmitInfo vk_submit_info{};
            vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            vk_submit_info.pNext = nullptr;
            
            vk_submit_info.waitSemaphoreCount = (uint32_t)submit_info.wait_semaphores.size();
            vk_submit_info.pWaitSemaphores    = (VkSemaphore*)submit_info.wait_semaphores.data();
            vk_submit_info.pWaitDstStageMask  = &submit_info.wait_stage_flags;
            
            vk_submit_info.signalSemaphoreCount = (uint32_t)submit_info.signal_semaphores.size();
            vk_submit_info.pSignalSemaphores    = (VkSemaphore*)submit_info.signal_semaphores.data();
            
            vk_submit_info.commandBufferCount = 1;
            vk_submit_info.pCommandBuffers    = &vk_command_buffer;
            
            if(submit_info.fence != nullptr){
                vkQueueSubmit(render::context.graphics_queue.vk_queue, 1,
                              &vk_submit_info, submit_info.fence->vk_fence);
                
                to_submit_id++;
                
                submit_info.fence->submission_flag = true;
                submission_condition_variable.notify_all();
                return core::Threadpool::TASK_COMPLETE;
            }
            vkQueueSubmit(render::context.graphics_queue.vk_queue, 1,
                          &vk_submit_info, VK_NULL_HANDLE);
            to_submit_id++;

            return core::Threadpool::TASK_COMPLETE;
        });
        
        return core::Threadpool::TASK_COMPLETE;
    });
}
void CommandManager::Present(PresentInfo present_info){
    uint32_t id = submit_id++;
    core::threadpool.Dispatch([this, present_info, id]{
        if(id != to_submit_id){
            return core::Threadpool::TASK_NOT_READY;
        }
        
        VkPresentInfoKHR vk_present_info{};
        vk_present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        vk_present_info.pNext = nullptr;
        
        vk_present_info.waitSemaphoreCount = (uint32_t)present_info.wait_semaphores.size();
        vk_present_info.pWaitSemaphores    = (VkSemaphore*)present_info.wait_semaphores.data();

        vk_present_info.swapchainCount = (uint32_t)present_info.swapchains.size();
        vk_present_info.pSwapchains    = &present_info.swapchains[0]->vk_swapchain_;
        vk_present_info.pImageIndices  = present_info.image_indices.data();
        
        present_info.swapchains[0]->usage_mutex.lock();
        vkQueuePresentKHR(render::context.graphics_queue.vk_queue, &vk_present_info);
        present_info.swapchains[0]->usage_mutex.unlock();
        
        to_submit_id++;
        
        return core::Threadpool::TASK_COMPLETE;
    });
}

void CommandManager::WaitForFence(Fence* fence){
    std::unique_lock<std::mutex> lock(submission_mutex);
    submission_condition_variable.wait(lock, [fence]{
        return fence->submission_flag;
    });
    lock.unlock();
    vkWaitForFences(render::context.vk_device, 1, &fence->vk_fence, VK_TRUE, UINT64_MAX);
}
void CommandManager::ResetFence(Fence* fence){
    submission_mutex.lock();
    fence->submission_flag = false;
    vkResetFences(render::context.vk_device, 1, &fence->vk_fence);
    submission_mutex.unlock();
}
}
