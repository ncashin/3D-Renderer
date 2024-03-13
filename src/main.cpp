
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
    auto window = std::make_unique<Window>("engine", display_mode.w, display_mode.h);
    
    render_context = new RenderContext(window, true, "engine", "engine");
    allocator      = new Allocator();
    auto render_manager = new RenderManager();
    auto swapchain      = new Swapchain(window);
    auto render_buffer  = new RenderBuffer(swapchain);
    
    Shader* vertex_shader   = new Shader({NGFX_SHADER_STAGE_VERTEX,   NGFX_SHADER_FORMAT_SPIRV, "vert.spv"});
    Shader* fragment_shader = new Shader({NGFX_SHADER_STAGE_FRAGMENT, NGFX_SHADER_FORMAT_SPIRV, "frag.spv"});
        
    PipelineInfo pipeline_info{};
    pipeline_info.render_buffer = render_buffer;
    pipeline_info.vertex_attributes = {};
    pipeline_info.vertex_bindings   = {};
    pipeline_info.shaders = { vertex_shader, fragment_shader };
    pipeline_info.front_face = NGFX_FRONT_FACE_CCW;
    pipeline_info.cull_mode  = NGFX_CULL_MODE_BACK_FACE;
    Pipeline* pipeline = new Pipeline(pipeline_info);
    delete vertex_shader;
    delete fragment_shader;
    
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;
    VkSemaphore swapchain_semaphore;
    vkCreateSemaphore(render_context->vk_device, &semaphore_create_info, nullptr, &swapchain_semaphore);
    
    VkSemaphore render_finished_semaphore;
    vkCreateSemaphore(render_context->vk_device, &semaphore_create_info, nullptr, &render_finished_semaphore);
    
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkFence render_complete_fence;
    vkCreateFence(render_context->vk_device, &fence_create_info, nullptr, &render_complete_fence);
        
    bool submission_fence = true;
    bool running = true;
    while(running){
        SDL_Event event;	
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_EventType::SDL_QUIT:
                    running = false;
                    break;
            }
        }
        
        render_manager->WaitForSubmissionFence(&submission_fence);

        vkWaitForFences(render_context->vk_device, 1, &render_complete_fence, VK_TRUE, UINT64_MAX);
        vkResetFences(render_context->vk_device, 1, &render_complete_fence);
        
        uint32_t image_index = swapchain->AcquireImage(swapchain_semaphore, VK_NULL_HANDLE);
        
        SubmitInfo submit_info{};
        submit_info.fence             = render_complete_fence;
        submit_info.wait_semaphores   = {swapchain_semaphore};
        submit_info.signal_semaphores = {render_finished_semaphore};
        submit_info.wait_stage_flags  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        
        render_manager->SubmitGraphics(submit_info,
                                       [render_buffer, swapchain, image_index, pipeline]
                                       (VkCommandBuffer vk_command_buffer){
            render_buffer->Begin(vk_command_buffer, swapchain, image_index);
            pipeline->Bind(vk_command_buffer);
            
            VkViewport viewport{};
            viewport.width  = swapchain->extent_.width;
            viewport.height = swapchain->extent_.height;
            viewport.x = 0;
            viewport.y = 0;
            vkCmdSetViewport(vk_command_buffer, 0, 1, &viewport);
            
            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = swapchain->extent_;
            vkCmdSetScissor(vk_command_buffer, 0, 1, &scissor);
            
            vkCmdDraw(vk_command_buffer, 3, 1, 0, 0);
            
            vkCmdEndRenderPass(vk_command_buffer);
        });
        
        render_manager->InsertSubmissionFence(&submission_fence);
        
        PresentInfo present_info{};
        present_info.wait_semaphores = {render_finished_semaphore};
        present_info.swapchains      = {swapchain->vk_swapchain_};
        present_info.image_indices   = {image_index};
        
        render_manager->Present(present_info);
    }
    
    delete render_manager;
    
    vkDestroySemaphore(render_context->vk_device, swapchain_semaphore, nullptr);
    vkDestroySemaphore(render_context->vk_device, render_finished_semaphore, nullptr);
    vkDestroyFence(render_context->vk_device, render_complete_fence, nullptr);
    
    delete pipeline;
    delete render_buffer;
    delete swapchain;
    delete allocator;
    delete render_context;
}
