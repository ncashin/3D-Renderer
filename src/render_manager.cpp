#include "render_manager.h"

namespace engine{

void RecordBuffer::Enqueue(std::function<void(VkCommandBuffer)> function){
    function_vector.emplace_back(function);
}
void RecordBuffer::Record(VkCommandBuffer command_buffer){
    for(std::function<void(VkCommandBuffer)> function : function_vector){
        function(command_buffer);
    }
}

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
}
RenderManager::~RenderManager(){
    vkDestroyCommandPool(render_context->vk_device, primary_graphics_command_pool, nullptr);
}

void RenderManager::Submit(SubmissionInfo submission_info){
    submission_mutex.lock();
    switch(submission_info.submission_queue){
        case SubmissionQueue::Graphics:
            graphics_record_queue.emplace_back(submission_info.record_buffer);
            break;
        case SubmissionQueue::Compute:
            graphics_record_queue.emplace_back(submission_info.record_buffer);
            break;
    }
    submission_queue.emplace_back(submission_info);
    submission_mutex.unlock();
}


void RenderManager::Record(){
    submission_mutex.lock();
    RecordBuffer* record_buffer = graphics_record_queue.front();
    graphics_record_queue.pop_front();
    submission_mutex.unlock();

    vkResetCommandBuffer(primary_graphics_command_buffers[0], 0);
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;
    
    vkBeginCommandBuffer (primary_graphics_command_buffers[0], &begin_info);
    record_buffer->Record(primary_graphics_command_buffers[0]);
    vkEndCommandBuffer(primary_graphics_command_buffers[0]);
    
    submission_mutex.lock();
    record_buffer->vk_command_buffer = primary_graphics_command_buffers[0];
    submission_mutex.unlock();
    // Signal Condition Variable
}
void RenderManager::Submit(){
    submission_mutex.lock();
    SubmissionInfo submission_info = submission_queue.front();
    submission_queue.pop_front();
    submission_mutex.unlock();
    
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    
    submit_info.waitSemaphoreCount = (uint32_t)submission_info.wait_semaphores.size();
    submit_info.pWaitSemaphores    = submission_info.wait_semaphores.data();
    submit_info.pWaitDstStageMask  = &submission_info.wait_stage_flags;

    submit_info.signalSemaphoreCount = (uint32_t)submission_info.signal_semaphores.size();
    submit_info.pSignalSemaphores    = submission_info.signal_semaphores.data();
    
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &submission_info.record_buffer->vk_command_buffer;
    
    VkQueue submission_queue;
    switch(submission_info.submission_queue){
        case SubmissionQueue::Graphics:
            submission_queue = render_context->graphics_queue.vk_queue;
            break;
        case SubmissionQueue::Compute:
            submission_queue = render_context->graphics_queue.vk_queue;
            break;
    }
    vkQueueSubmit(submission_queue, 1, &submit_info, submission_info.fence);
}
}
