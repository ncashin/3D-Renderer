#include "camera.h"

namespace render{
render::PushConstantRange Camera::PushConstantRange(uint32_t offset){
    return {
        render::SHADER_STAGE_VERTEX, offset, sizeof(glm::mat4),
    };
}
glm::mat4 Camera::GetViewProjection(float aspect_ratio){
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
}
