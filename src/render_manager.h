#pragma once
#include "render_context.h"

#include <deque>
#include <functional>
#include <mutex>
#include <thread>

#include "thread_pool.h"

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


void SubmitGraphics(SubmitInfo submission_info, bool* submission_flag,
                    VkCommandBuffer vk_command_buffer);
void SubmitGraphics(SubmitInfo submission_info, bool* submission_flag,
                    std::function<void(VkCommandBuffer)> record_function);
void SubmitCompute (SubmitInfo submission_info, std::function<void(VkCommandBuffer)> record_function);
void Present(PresentInfo present_info);

void ResetSubmissionFence(bool& submission_fence);
void WaitForSubmissionFence(bool* fence);

void Present();

extern std::mutex  submission_mutex;
extern std::condition_variable submission_condition_variable;

extern uint32_t to_submit_id;
extern uint32_t submit_id;

extern VkCommandPool primary_graphics_command_pool;
extern std::vector<VkCommandBuffer> primary_graphics_command_buffers;
}
}
