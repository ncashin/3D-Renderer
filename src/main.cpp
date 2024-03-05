
#include "window.h"
#include "swapchain.h"

#include "render_context.h"
#include "allocator.h"
#include "render_buffer.h"

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
    }
    
    vkDeviceWaitIdle(render_context->vk_device);
    delete render_buffer;
    delete swapchain;
    delete allocator;
    delete render_context;
    delete window;
}
