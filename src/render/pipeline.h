#pragma once
#include <fstream>

#include <deque>
#include <functional>
#include <mutex>
#include <thread>

#include "glm.hpp"

#include "thread_pool.h"

#include "render/context.h"
#include "render/render_buffer.h"
#include "render/descriptor.h"


namespace render{
enum ShaderStage{
    SHADER_STAGE_VERTEX   = 0x00000001,
    SHADER_STAGE_FRAGMENT = 0x00000010,
};
enum ShaderFormat{
    SHADER_FORMAT_GLSL,
    SHADER_FORMAT_SPIRV,
};
struct ShaderInfo{
    ShaderStage  shader_stage;
    ShaderFormat shader_code_format;
    const char* filepath;
    size_t buffer_size;
    char* buffer;
};
class Shader{
public:
    static VkShaderModule CompileGlsl(size_t buffer_size, char* buffer);
    static VkShaderModule CompileSpirv(size_t buffer_size, char* buffer);
    
public:
    Shader(ShaderStage shader_stage, ShaderFormat shader_code_format, size_t buffer_size, char* buffer);
    Shader(ShaderInfo info);
    ~Shader();
    
    ShaderStage GetStage();
    VkShaderModule GetModule();
    
private:
    ShaderStage shader_stage_;
    VkShaderModule   vk_shader_module_;
};

struct VertexBinding{
    uint32_t binding;
    uint32_t stride;
    VkVertexInputRate input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
};
enum class AttributeFormat{
    Float,
    Float1,
    Float2,
    Float3,
    Float4,
    
    Int,
    Int1,
    Int2,
    Int3,
    Int4,
    
    Uint,
    Uint1,
    Uint2,
    Uint3,
    Uint4,
};
struct VertexAttribute{
    uint32_t location;
    uint32_t binding;
    VkFormat format;
    uint32_t offset;
};
enum NGFX_FrontFace{
    NGFX_FRONT_FACE_CCW = 0,
    NGFX_FRONT_FACE_CW  = 1,
};
enum NGFX_CullMode{
    NGFX_CULL_MODE_NONE                = 0,
    NGFX_CULL_MODE_FRONT_FACE          = 1,
    NGFX_CULL_MODE_BACK_FACE           = 2,
    NGFX_CULL_MODE_FRONT_AND_BACK_FACE = 3,
};
struct PushConstantRange{
    ShaderStage stageFlags;
    uint32_t         offset;
    uint32_t         size;
};
struct PipelineInfo{
    std::vector<PushConstantRange>   push_constant_ranges;
    std::vector<DescriptorSetLayout> descriptor_set_layouts;
    
    std::vector<VertexBinding>   vertex_bindings;
    std::vector<VertexAttribute> vertex_attributes;
    
    RenderBuffer* render_buffer;
    std::vector<Shader*> shaders;
    
    NGFX_FrontFace front_face = NGFX_FRONT_FACE_CW;
    NGFX_CullMode  cull_mode  = NGFX_CULL_MODE_NONE;
    
    bool depth_test_enabled = false;
    bool depth_write_enabled = false;
};
class Pipeline {
public:
    Pipeline();
    ~Pipeline();
    
    void Initialize(PipelineInfo info);
    void Terminate();
    
    void Bind(VkCommandBuffer command_buffer);
    void PushConstant(VkCommandBuffer vk_command_buffer, VkDeviceSize size, VkDeviceSize offset, void* data);
    void BindDescriptorSet(VkCommandBuffer vk_command_buffer, DescriptorSet descriptor_set, uint32_t binding);
    
    VkPipelineLayout vk_pipeline_layout;
    VkPipeline vk_pipeline;
};

class PipelineManager{
public:
    void Initialize();
    void Terminate();
    
    Pipeline* Compile(PipelineInfo info);
    void AwaitCompilation(Pipeline* pipeline);
    
    void Destroy(Pipeline* pipeline);
    
    std::mutex compilation_mutex;
    std::condition_variable compilation_condition_variable;
};
extern PipelineManager pipeline_manager;
}
