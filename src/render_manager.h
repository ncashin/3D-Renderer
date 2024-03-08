#pragma once
#include "render_context.h"

#include <deque>
#include <functional>
#include <mutex>
#include <thread>

namespace engine{
enum class SubmissionType{
    Graphics,
    Compute,
    
    Present,
    
    Fence,
    
    ProcessEndSignal,
};
class RecordBuffer{
public:
    void Enqueue(std::function<void(VkCommandBuffer)> function);
    void Record(VkCommandBuffer vk_command_buffer);
    
    std::vector<std::function<void(VkCommandBuffer)>> function_vector;
};
struct SubmitInfo{
    std::vector<VkSemaphore> wait_semaphores;
    VkPipelineStageFlags wait_stage_flags;
    std::vector<VkSemaphore> signal_semaphores;
    VkFence fence;
    VkCommandBuffer* vk_command_buffer;
};
struct PresentInfo{
    std::vector<VkSemaphore> wait_semaphores;
    std::vector<VkSwapchainKHR> swapchains;
    std::vector<uint32_t> image_indices;
};
class RenderManager{
    struct RecordingInfo{
        VkCommandBuffer* vk_command_buffer;
        RecordBuffer record_buffer;
    };
    struct SubmissionInfo{
        SubmissionType type;
        void* pointer;
    };
public:
    RenderManager();
    ~RenderManager();
        
    void SubmitGraphics(SubmitInfo submission_info, RecordBuffer record_buffer);
    void SubmitCompute (SubmitInfo submission_info, RecordBuffer record_buffer);
    void Present(PresentInfo present_info);
    
    void ResetFence(bool& submission_fence);
    void InsertSubmissionFence(bool* fence);
    void WaitForSubmission(bool* fence);
    
    void HandleGraphicsRecording();
    void HandleSubmission();
    void Present();
        
    bool active = true;
    
    VkCommandPool primary_graphics_command_pool;
    std::vector<VkCommandBuffer> primary_graphics_command_buffers;
    std::condition_variable graphics_record_condition_variable;
    std::thread graphics_record_thread;
    std::deque<RecordingInfo> graphics_record_queue;
    
    std::mutex  submission_mutex;
    std::condition_variable submission_condition_variable;
    std::thread submission_thread;
    std::deque<SubmissionInfo> submission_queue;
        
    std::mutex fence_mutex;
    std::condition_variable fence_condition_variable;
};
}
