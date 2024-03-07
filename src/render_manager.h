#pragma once
#include "render_context.h"

#include <deque>
#include <functional>
#include <mutex>

namespace engine{
enum class SubmissionQueue{
    Graphics,
    Compute,
};
class RecordBuffer{
public:
    void Enqueue(std::function<void(VkCommandBuffer)> function);
    void Record(VkCommandBuffer vk_command_buffer);
    
    VkCommandBuffer vk_command_buffer = VK_NULL_HANDLE;
    std::vector<std::function<void(VkCommandBuffer)>> function_vector;
};
struct SubmissionInfo{
    SubmissionQueue submission_queue;
    std::vector<VkSemaphore> wait_semaphores;
    VkPipelineStageFlags wait_stage_flags;
    std::vector<VkSemaphore> signal_semaphores;
    RecordBuffer* record_buffer;
    VkFence fence;
};
class RenderManager{
public:
    RenderManager();
    ~RenderManager();
        
    void Submit(SubmissionInfo submission_info);
    
    void Record();
    void Submit();
        
    VkCommandPool primary_graphics_command_pool;
    std::vector<VkCommandBuffer> primary_graphics_command_buffers;
    std::deque<RecordBuffer*> graphics_record_queue;
    
    std::mutex submission_mutex;
    std::deque<SubmissionInfo> submission_queue;
};
}
