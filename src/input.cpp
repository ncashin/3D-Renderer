#include "input.h"

#include "stdio.h"
#include <memory>
float     Input::mouse_scroll = 0.0f;
glm::vec2 Input::mouse_position;
glm::vec2 Input::mouse_offset;

InputState Input::scancode_state[SCANCODE_COUNT] = {};

void Input::Flush(){
    for(InputState& state : scancode_state){
        state = (InputState)((int)state & 1);
    }
    mouse_scroll = 0.0f;
    mouse_offset = {};
}


void       Input::SetKey(ScanCode code, InputState state){
    scancode_state[(uint32_t)code] = state;
}

InputState Input::GetKey(ScanCode code){
    return scancode_state[(int)code];
}

void  Input::SetMouseScroll(float value){
    mouse_scroll = value;
}
float Input::GetMouseScroll(){
    return mouse_scroll;
}

void Input::RegisterMouseMovement(glm::vec2 movement){
    mouse_offset   += movement;
    mouse_position += movement;
}
glm::vec2 Input::GetMousePosition(){
    return mouse_position;
}
glm::vec2 Input::GetMouseOffset(){
    return mouse_offset;
}
