#pragma once
#include "render_context.h"

#include <deque>
#include <functional>
#include <mutex>
#include <thread>

namespace ngfx{
enum NGFX_SubmissionType{
    NGFX_SUBMISSION_TYPE_GRAPHICS,
    NGFX_SUBMISSION_TYPE_COMPUTE,
    
    NGFX_SUBMISSION_TYPE_PRESENT,
    
    NGFX_SUBMISSION_TYPE_FENCE,
    
    NGFX_SUBMISSION_TYPE_EXIT,
};
struct SubmitInfo{
    std::vector<VkSemaphore> wait_semaphores;
    VkPipelineStageFlags     wait_stage_flags;
    std::vector<VkSemaphore> signal_semaphores;
    VkFence                  fence;
    VkCommandBuffer* vk_command_buffer;
};
struct PresentInfo{
    std::vector<VkSemaphore>    wait_semaphores;
    std::vector<VkSwapchainKHR> swapchains;
    std::vector<uint32_t>       image_indices;
};

namespace CommandManager{
struct RecordingInfo{
    VkCommandBuffer* vk_command_buffer;
    std::function<void(VkCommandBuffer)> function;
};
struct SubmissionInfo{
    NGFX_SubmissionType type;
    void* pointer;
};

void Initialize();
void Terminate();

void SubmitGraphics(SubmitInfo submission_info, std::function<void(VkCommandBuffer)> record_function);
void SubmitCompute (SubmitInfo submission_info, std::function<void(VkCommandBuffer)> record_function);
void Present(PresentInfo present_info);

void ResetSubmissionFence(bool& submission_fence);
void InsertSubmissionFence(bool* fence);
void WaitForSubmissionFence(bool* fence);

void HandleGraphicsRecording();
void HandleSubmission();
void Present();

extern bool active;

extern VkCommandPool primary_graphics_command_pool;
extern std::vector<VkCommandBuffer> primary_graphics_command_buffers;
extern std::condition_variable graphics_record_condition_variable;
extern std::thread graphics_record_thread;
extern std::deque<RecordingInfo> graphics_record_queue;

extern std::mutex  submission_mutex;
extern std::condition_variable submission_condition_variable;
extern std::thread submission_thread;
extern std::deque<SubmissionInfo> submission_queue;

extern std::mutex fence_mutex;
extern std::condition_variable fence_condition_variable;
}
}
