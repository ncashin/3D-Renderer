#include <chrono>
#include <iostream>

#include "thread_pool.h"
#include "window.h"
#include "input.h"
#include "asset.h"

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"

#include "render/render.h"

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

render::Camera camera{};
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

render::Window window{};
void Initialize(){
    const uint32_t thread_count = 1;
    core::threadpool.Initialize(thread_count);
    
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);

    window.Initialize({"engine", display_mode.w, display_mode.h});

    render::ContextInfo context_info{};
    context_info.window = &window;
    context_info.enable_validation_layers = true;
    context_info.applcation_name = "engine";
    context_info.engine_name = "engine";
    render::context.Initalize(context_info);
    
    render::descriptor_allocator.Initialize();
    render::pipeline_manager.Initialize();
    render::command_manager.Initialize();
    render::staging_manager.Initialize();
    render::gpu_buffer.Initialize({
        1000000000,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT  |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY, 0
    });
}

MESH_VERTEX_STRUCT Vertex {
    MVS_POSITION(pos);
    MVS_TEXTURE_COORDINATE_2D(tc2d);
};

int main(int argc, char** argv){
    Initialize();
    auto swapchain      = new render::Swapchain(&window);
    auto render_buffer  = new render::RenderBuffer(swapchain);
    
    auto vertex_shader   = new render::Shader({
        render::SHADER_STAGE_VERTEX,   render::SHADER_FORMAT_SPIRV, "vert.spv"
    });
    auto fragment_shader = new render::Shader({
        render::SHADER_STAGE_FRAGMENT, render::SHADER_FORMAT_SPIRV, "frag.spv"
    });
    
    auto set_layout = render::DescriptorSetLayout{};
    set_layout.Initialize({
        {0, VK_DESCRIPTOR_TYPE_SAMPLER,       1, render::SHADER_STAGE_FRAGMENT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, render::SHADER_STAGE_FRAGMENT, nullptr},
    });
    
    render::PipelineInfo pipeline_info{};
    pipeline_info.push_constant_ranges   = { render::Camera::PushConstantRange(0), };
    pipeline_info.descriptor_set_layouts = { set_layout, };
    pipeline_info.vertex_attributes = {
        render::MVS::PositionAttribute<Vertex>(0, 0),
        render::MVS::TextureCoordinate2DAttribute<Vertex>(1, 0),
    };
    pipeline_info.vertex_bindings = {
        render::MVS::Binding<Vertex>(0),
    };
    pipeline_info.render_buffer = render_buffer;
    pipeline_info.shaders = { vertex_shader, fragment_shader };
    pipeline_info.front_face = render::NGFX_FRONT_FACE_CW;
    pipeline_info.cull_mode  = render::NGFX_CULL_MODE_BACK_FACE;
    pipeline_info.depth_test_enabled = true;
    pipeline_info.depth_write_enabled = true;
    render::Pipeline* pipeline = render::pipeline_manager.Compile(pipeline_info);
    
    uint32_t vertex_count = 0;
    uint32_t index_count  = 0;

    auto mesh    = asset::GetMesh<Vertex>("backpack/backpack.obj");
    auto texture = asset::GetTexture("backpack/diffuse.jpg");
    
    render::Sampler sampler{};
    sampler.Initialize();
    
    /*render::Texture texture{};
    texture.Initialize({100, 100, 1});

    const VkDeviceSize image_bytesize = 100 * 100 * sizeof(glm::vec4);
    char* staging_pointer = (char*)render::staging_manager.UploadToImage(image_bytesize, &texture);
    for(uint32_t i = 0; i < image_bytesize; i++){
        ((uint8_t*)staging_pointer)[i] = rand() % UINT8_MAX;
    }*/
    
    auto descriptor_set = render::descriptor_allocator.Allocate(set_layout);
    sampler.WriteDescriptor(descriptor_set.vk_descriptor_set, 0, 0);
    texture.WriteDescriptor(descriptor_set.vk_descriptor_set, 1, 0);
    
    render::staging_manager.SubmitUpload({});
    
    render::Fence fence{};
    fence.Initialize(render::Fence::InitializeSignaled);
    
    render::Semaphore render_finished_semaphore;
    render_finished_semaphore.Initialize();
    render::Semaphore swapchain_semaphore;
    swapchain_semaphore.Initialize();
    
    render::pipeline_manager.AwaitCompilation(pipeline);
    delete vertex_shader;
    delete fragment_shader;
    
    render::staging_manager.AwaitUploadCompletion();
    
    auto  start = std::chrono::high_resolution_clock::now();
    bool submission_fence = true;
    while(running){
        Input::Flush();
        HandleEvent();
        UpdateCamera();
        
        if(Input::GetKey(ScanCode::P)){
            running = false;
        }
        
        render::command_manager.WaitForFence(&fence);
        render::command_manager.ResetFence(&fence);
        {
            auto  finish = std::chrono::high_resolution_clock::now();
            std::cout << "Frame Time: "
            << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count()
            << " microseconds\n";
            start = std::chrono::high_resolution_clock::now();
        }
        
        /*render::descriptor_allocator.Flush();
        auto descriptor_set = render::descriptor_allocator.Allocate(set_layout);
        auto a = render::descriptor_allocator.Allocate(set_layout);
        auto b = render::descriptor_allocator.Allocate(set_layout);
        auto c = render::descriptor_allocator.Allocate(set_layout);
        auto d = render::descriptor_allocator.Allocate(set_layout);*/
	
        uint32_t image_index = swapchain->AcquireImage(swapchain_semaphore.vk_semaphore, VK_NULL_HANDLE);
        
        render::SubmitInfo submit_info{};
        submit_info.fence             = &fence;
        submit_info.wait_semaphores   = {swapchain_semaphore};
        submit_info.signal_semaphores = {render_finished_semaphore};
        submit_info.wait_stage_flags  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        
        float aspect_ratio = (float)window.width / (float)window.height;
        glm::mat4 view_projection(camera.GetViewProjection(aspect_ratio));

        render::command_manager.SubmitGraphics(submit_info,
                                              [render_buffer, swapchain, image_index, pipeline,
                                               view_projection, descriptor_set, &mesh]
                                               (VkCommandBuffer vk_command_buffer){
            render_buffer->Begin(vk_command_buffer, swapchain, image_index);
            pipeline->Bind(vk_command_buffer);
            
            render::gpu_buffer.buffer.BindAsVertexBuffer(vk_command_buffer, 0);
            render::gpu_buffer.buffer.BindAsIndexBuffer (vk_command_buffer, 0);
            
            pipeline->PushConstant(vk_command_buffer, 0, sizeof(glm::mat4), (void*)&view_projection);
            pipeline->BindDescriptorSet(vk_command_buffer, descriptor_set, 0);
            
            VkViewport viewport{};
            viewport.width  = swapchain->extent_.width;
            viewport.height = swapchain->extent_.height;
            viewport.x = 0;
            viewport.y = 0;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(vk_command_buffer, 0, 1, &viewport);
            
            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = swapchain->extent_;
            vkCmdSetScissor(vk_command_buffer, 0, 1, &scissor);
            
            mesh.Draw(vk_command_buffer, 1, 0);
            
            vkCmdEndRenderPass(vk_command_buffer);
        });
        
        
        render::PresentInfo present_info{};
        present_info.wait_semaphores = {render_finished_semaphore};
        present_info.swapchains      = {swapchain};
        present_info.image_indices   = {image_index};
        
        render::command_manager.Present(present_info);
    }
    
    render::command_manager.WaitForFence(&fence);
    fence.Terminate();
    
    swapchain_semaphore.Terminate();
    render_finished_semaphore.Terminate();

    render::gpu_buffer.Terminate();
    render::pipeline_manager.Destroy(pipeline);
    set_layout.Terminate();

    texture.Terminate();
    sampler.Terminate();
    
    render::staging_manager.Terminate();
    render::command_manager.Terminate();
    render::pipeline_manager.Terminate();
    render::descriptor_allocator.Terminate();
    
    delete render_buffer;
    delete swapchain;

    render::context.Terminate();
    
    window.Terminate();
    
    core::threadpool.Terminate();
}
