#pragma once
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

#include <atomic>
#include "thread_pool.h"

#include "render/context.h"
#include "render/swapchain.h"

namespace render{
struct Semaphore{
    void Initialize();
    void Terminate();
    
    VkSemaphore vk_semaphore;
};
struct Fence{
    enum FenceInitializationState{
        InitializeUnsignaled = 0,
        InitializeSignaled = 1,
    };
    void Initialize(FenceInitializationState initialization_state);
    void Terminate();
    
    bool    submission_flag = true;
    VkFence vk_fence        = VK_NULL_HANDLE;
};

struct SubmitInfo{
    std::vector<Semaphore> wait_semaphores;
    VkPipelineStageFlags   wait_stage_flags;
    std::vector<Semaphore> signal_semaphores;
    Fence*                 fence;
    VkCommandBuffer* vk_command_buffer;
};
struct PresentInfo{
    std::vector<Semaphore>  wait_semaphores;
    std::vector<Swapchain*> swapchains;
    std::vector<uint32_t>   image_indices;
};

class CommandBuffer{
public:
    uint8_t record_count = 1;
    uint8_t record_complete_count = 0;
    VkCommandBuffer vk_command_buffer;
};

class CommandManager{
public:
    void Initialize();
    void Terminate();
    
    CommandBuffer* RecordAsync(std::function<void(VkCommandBuffer)> record_function);
    void SubmitAsync(SubmitInfo submit_info, CommandBuffer* command_buffer);
    
    void SubmitGraphics(SubmitInfo submission_info,
                        VkCommandBuffer vk_command_buffer);
    void SubmitGraphics(SubmitInfo submission_info,
                        std::function<void(VkCommandBuffer)> record_function);
    void SubmitCompute (SubmitInfo submission_info, std::function<void(VkCommandBuffer)> record_function);
    void Present(PresentInfo present_info);
    
    void ResetFence  (Fence* fence);
    void WaitForFence(Fence* fence);
    
    void Present();
    
    std::mutex  submission_mutex{};
    std::condition_variable submission_condition_variable{};

    std::atomic<uint32_t> to_submit_id;
    std::atomic<uint32_t> submit_id;

    VkCommandPool primary_graphics_command_pool{};
    std::vector<VkCommandBuffer> primary_graphics_command_buffers{};
};
extern CommandManager command_manager;
}
