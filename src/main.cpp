
#include "window.h"
#include "swapchain.h"

#include "render_context.h"
#include "allocator.h"

#include "descriptor.h"

#include "render_buffer.h"

#include "pipeline.h"

#include "render_manager.h"

using namespace engine;

#include <glm/glm.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include "glm/gtx/transform.hpp"

#include "input.h"

#include "asset.h"

struct Vertex {
    glm::vec3 position;
    //glm::vec4 color;
};
/*const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};*/

class Camera{
public:
    glm::mat4 GetViewProjection(float aspect_ratio){
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(direction);
        
        glm::mat4 view = glm::lookAt(position, position + front, up);
        
        if(orthographic){
            return glm::ortho(view_size * aspect_ratio, -view_size * aspect_ratio, view_size, -view_size,
                              z_far, z_near) * view;
        }
        return glm::perspective(-glm::radians(view_size), aspect_ratio, z_near, z_far) * view;
    }
    
    glm::vec3 position = glm::vec3(0.0f, 0.0f, -5.0f);
    glm::vec3 front    = glm::vec3(0.0f, 0.0f,  1.0f);
    glm::vec3 up       = glm::vec3(0.0f, 1.0f,  0.0f);
    
    bool orthographic = false;
    float yaw = 90.0f;
    float pitch = 0.0f;
    float view_size = 10.0f;
    float z_near = 1.0f;
    float z_far = 10000.0f;
    glm::mat4 projection;
};

bool running = true;
void HandleEvent(){
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_EventType::SDL_QUIT:{
                running = false;
                break;
            }
            case SDL_EventType::SDL_MOUSEWHEEL:{
                SDL_MouseWheelEvent mouse_wheel = event.wheel;
                Input::SetMouseScroll(mouse_wheel.y);
                break;
            }
            case SDL_EventType::SDL_MOUSEMOTION:{
                SDL_MouseMotionEvent mouse_motion = event.motion;
                Input::RegisterMouseMovement({mouse_motion.xrel, mouse_motion.yrel});
                break;
            }
            case SDL_EventType::SDL_KEYDOWN:{
                SDL_KeyboardEvent key_down = event.key;
                Input::SetKey((ScanCode)key_down.keysym.scancode, InputState::INPUT_STATE_DOWN);
                break;
            }
            case SDL_EventType::SDL_KEYUP:{
                SDL_KeyboardEvent key_up = event.key;
                Input::SetKey((ScanCode)key_up.keysym.scancode, InputState::INPUT_STATE_UP);
                break;
            }
        }
    }
}

Camera camera{};
float camera_speed = 0.1f;
const float camera_sensitivity = 0.1f;
const float camera_zoom_sensitivity = 0.4f;
void UpdateCamera(){
    if(Input::GetKey(ScanCode::W) == INPUT_STATE_DOWN) 
        camera.position += camera_speed * camera.front;
    if(Input::GetKey(ScanCode::S) == INPUT_STATE_DOWN) 
        camera.position -= camera_speed * camera.front;

    if(Input::GetKey(ScanCode::D) == INPUT_STATE_DOWN)
        camera.position -= camera_speed * glm::normalize(glm::cross(camera.front, camera.up));
    if(Input::GetKey(ScanCode::A) == INPUT_STATE_DOWN)
        camera.position += camera_speed * glm::normalize(glm::cross(camera.front, camera.up));
    
    if(Input::GetKey(ScanCode::SDL_SCANCODE_SPACE) == INPUT_STATE_DOWN){
        camera.position += camera.up * camera_speed;
    }
    if(Input::GetKey(ScanCode::SDL_SCANCODE_LSHIFT) == INPUT_STATE_DOWN){
        camera.position -= camera.up * camera_speed;
    }
    
    camera.view_size += Input::GetMouseScroll() * camera_zoom_sensitivity;

    camera.yaw   -= Input::GetMouseOffset().x * camera_sensitivity;
    camera.pitch -= Input::GetMouseOffset().y * camera_sensitivity;
    
    if(camera.pitch > 89.0f)
      camera.pitch =  89.0f;
    if(camera.pitch < -89.0f)
      camera.pitch = -89.0f;
}

void SyntaxTest(){
    ngfx::Context::vk_device;
    ngfx::Manager::Submit(submit_info,
                          [data, render_buffer, swapchain, image_index, pipeline,
                          buffer, vertex_count, index_count]
                          (VkCommandBuffer vk_command_buffer)
                          {
        ngfx::PipelineManager::Bind("example_pipeline");
        ngfx::VertexBuffer::EnsureBound();
        ngfx::IndexBuffer ::EnsureBound();
        ngfx::PipelineManager::BindDescriptorSet(descriptor_setx);
        
        ngfx::PipelineManager::PushConstant(vk_command_buffer, 0, sizeof(glm::mat4), data);
        delete (glm::mat4*)data;
        
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
        
        vkCmdDrawIndexed(vk_command_buffer, index_count, 1, 0, 0, 0);
        
        vkCmdEndRenderPass(vk_command_buffer);
    });
}

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
    pipeline_info.vertex_attributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex, position)},
        //{1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color)},
    };
    pipeline_info.vertex_bindings = {{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
    pipeline_info.shaders = { vertex_shader, fragment_shader };
    pipeline_info.front_face = NGFX_FRONT_FACE_CCW;
    pipeline_info.cull_mode  = NGFX_CULL_MODE_BACK_FACE;
    Pipeline* pipeline = new Pipeline(pipeline_info);
    delete vertex_shader;
    delete fragment_shader;
    
    auto set_layout = CreateDescriptorSetLayout({
        {0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 16, NGFX_SHADER_STAGE_FRAGMENT, nullptr},
    });
    
    Buffer* buffer = new Buffer(100000000);
    allocator->Allocate(NGFX_MEMORY_TYPE_COHERENT, buffer);
    char* mapped_pointer;
    allocator->Map(buffer, &mapped_pointer);
    uint32_t vertex_count = 0;
    uint32_t index_count  = 0;
    Asset::LoadMesh(mapped_pointer, &vertex_count, &index_count, "backpack/backpack.obj");
    //std::memcpy(mapped_pointer, vertices.data(), vertices.size() * sizeof(Vertex));

    Image* image = new Image({100, 100, 1});
    allocator->Allocate(NGFX_MEMORY_TYPE_DEVICE_LOCAL, image);
    
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
    while(running){
        Input::Flush();
        HandleEvent();
        UpdateCamera();
        
        if(Input::GetKey(ScanCode::P)){
            running = false;
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
        
        float aspect_ratio = (float)window->width / (float)window->height;
        void* data = new glm::mat4(camera.GetViewProjection(aspect_ratio));

        render_manager->SubmitGraphics(submit_info,
                                       [data, render_buffer, swapchain, image_index, pipeline, 
                                        buffer, vertex_count, index_count]
                                       (VkCommandBuffer vk_command_buffer){
            render_buffer->Begin(vk_command_buffer, swapchain, image_index);
            pipeline->Bind(vk_command_buffer);
            
            buffer->BindAsVertexBuffer(vk_command_buffer, 0);
            buffer->BindAsIndexBuffer (vk_command_buffer, vertex_count * sizeof(Vertex));
            
            pipeline->PushConstant(vk_command_buffer, 0, sizeof(glm::mat4), data);
            delete (glm::mat4*)data;
            
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
            
            //vkCmdDraw(vk_command_buffer, vertex_count, 1, 0, 0);
            vkCmdDrawIndexed(vk_command_buffer, index_count, 1, 0, 0, 0);
            
            vkCmdEndRenderPass(vk_command_buffer);
        });
        
        render_manager->InsertSubmissionFence(&submission_fence);
        
        PresentInfo present_info{};
        present_info.wait_semaphores = {render_finished_semaphore};
        present_info.swapchains      = {swapchain->vk_swapchain_};
        present_info.image_indices   = {image_index};
        
        render_manager->Present(present_info);
    }
        
    render_manager->WaitForSubmissionFence(&submission_fence);
    vkWaitForFences(render_context->vk_device, 1, &render_complete_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(render_context->vk_device, 1, &render_complete_fence);
    delete render_manager;
    
    allocator->Free(buffer);
    delete buffer;
    
    allocator->Free(image);
    delete image;
    
    vkDestroySemaphore(render_context->vk_device, swapchain_semaphore, nullptr);
    vkDestroySemaphore(render_context->vk_device, render_finished_semaphore, nullptr);
    vkDestroyFence(render_context->vk_device, render_complete_fence, nullptr);
    
    delete pipeline;
    delete render_buffer;
    delete swapchain;
    delete allocator;
    delete render_context;
}
