#include "render/pipeline.h"

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"

namespace render{
class Camera{
public:
    static render::PushConstantRange PushConstantRange(uint32_t offset);
    glm::mat4 GetViewProjection(float aspect_ratio);
    
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
}
