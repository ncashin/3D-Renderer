#pragma once
#include "render_context.h"

#include <deque>
#include <functional>
#include <mutex>

namespace engine{
enum class SubmissionQueue{
    Graphics,
    Compute,
    
    Present,
};
class RecordBuffer{
public:
    void Enqueue(std::function<void(VkCommandBuffer)> function);
    void Record(VkCommandBuffer vk_command_buffer);
    
    VkCommandBuffer vk_command_buffer = VK_NULL_HANDLE;
    std::vector<std::function<void(VkCommandBuffer)>> function_vector;
};
struct SubmissionInfo{
    std::vector<VkSemaphore> wait_semaphores;
    VkPipelineStageFlags wait_stage_flags;
    std::vector<VkSemaphore> signal_semaphores;
    RecordBuffer* record_buffer;
    VkFence fence;
};
struct PresentInfo{
    std::vector<VkSemaphore> wait_semaphores;
    std::vector<VkSwapchainKHR> swapchains;
    std::vector<uint32_t> image_indices;
};
struct Submission{
    SubmissionQueue queue;
    void* pointer;
};
class RenderManager{
public:
    RenderManager();
    ~RenderManager();
        
    void Submit(SubmissionQueue queue, SubmissionInfo submission_info);
    void Present(PresentInfo present_info);
    
    void Record();
    void Submit();
    void Present();
        
    VkCommandPool primary_graphics_command_pool;
    std::vector<VkCommandBuffer> primary_graphics_command_buffers;
    std::deque<RecordBuffer*> graphics_record_queue;
    
    std::mutex submission_mutex;
    std::deque<Submission> submission_queue;
};
}
