
#include "window.h"
#include "swapchain.h"

#include "render_context.h"
#include "allocator.h"
#include "render_buffer.h"

#include "pipeline.h"

#include "render_manager.h"

using namespace engine;

int main(int argc, char** argv){
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);
    auto window = new Window("engine", display_mode.w, display_mode.h);
    render_context = new RenderContext(window, true, "engine", "engine");
    allocator      = new Allocator();
    auto swapchain = new Swapchain(window);
    auto render_buffer = new RenderBuffer(swapchain);
    
    auto render_manager = new RenderManager();
    
    PipelineInfo pipeline_create_info{};
    Pipeline* = new Pipeline();
    
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;
    
    VkSemaphore swapchain_semaphore;
    vkCreateSemaphore(render_context->vk_device, &semaphore_create_info, nullptr, &swapchain_semaphore);
    
    auto record_buffer  = new RecordBuffer();
    uint32_t image_index = swapchain->AcquireImage(swapchain_semaphore, VK_NULL_HANDLE);
    record_buffer->Enqueue([render_buffer, swapchain, image_index](VkCommandBuffer vk_command_buffer){
        render_buffer->Begin(vk_command_buffer, swapchain, image_index);
        pipeline->Bind();
        vkCmdEndRenderPass  (vk_command_buffer);
    });
    
    SubmissionInfo submission_info{};
    submission_info.submission_queue = SubmissionQueue::Graphics;
    submission_info.record_buffer = record_buffer;
    submission_info.fence = VK_NULL_HANDLE;
    submission_info.wait_semaphores = {swapchain_semaphore};
    submission_info.signal_semaphores = {};
    submission_info.wait_stage_flags  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    render_manager->Submit(submission_info);
    
    render_manager->Record();
    render_manager->Submit();
    
    bool  running = true;
    while(running){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_EventType::SDL_QUIT:
                    running = false;
                    break;
            }
        }
    }
    
    vkDeviceWaitIdle(render_context->vk_device);
    vkDestroySemaphore(render_context->vk_device, swapchain_semaphore, nullptr);
    delete render_manager;
    delete render_buffer;
    delete swapchain;
    delete allocator;
    delete render_context;
    delete window;
}
