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
    bool record_submission_complete = false;
    VkCommandBuffer vk_command_buffer = VK_NULL_HANDLE;
};

class CommandManager{
public:
    void Initialize();
    void Terminate();
    
    void Free(CommandBuffer* command_buffer);
    
    CommandBuffer* RecordAsync(std::function<void(VkCommandBuffer)> record_function);
    void           RecordAsync(std::function<void(VkCommandBuffer)> record_function,
                               CommandBuffer* command_buffer);
    void SignalRecordCompletion(CommandBuffer* command_buffer);
    
    void SubmitAsync(SubmitInfo submit_info, CommandBuffer* command_buffer);

    void PresentAsync(PresentInfo present_info);
    
    void ResetFence  (Fence* fence);
    void WaitForFence(Fence* fence);
    
    
    void WTRecord();
    
    std::mutex  submission_mutex{};
    std::condition_variable submission_condition_variable{};

    std::atomic<uint32_t> to_record_id = 0;
    std::atomic<uint32_t> record_id = 0;
    
    std::atomic<uint32_t> to_submit_id = 0;
    std::atomic<uint32_t> submit_id = 0;

    std::mutex command_buffer_mutex{};
    VkCommandPool primary_graphics_command_pool{};
    std::vector<VkCommandBuffer> primary_graphics_command_buffers{};
    
    bool wt_active = true;
    std::thread wt_record;
    std::mutex wt_record_mutex;
    std::condition_variable wt_record_condition_variable;
    std::vector<std::function<void()>> wt_record_queue;
    
    uint8_t frame = 0;
};
extern CommandManager command_manager;
}
