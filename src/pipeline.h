#pragma once
#include <fstream>

#include <deque>
#include <functional>
#include <mutex>
#include <thread>

#include "glm.hpp"

#include "render_context.h"
#include "render_buffer.h"

#include "thread_pool.h"


namespace ngfx{
enum NGFX_ShaderStage{
    NGFX_SHADER_STAGE_VERTEX   = 0x00000001,
    NGFX_SHADER_STAGE_FRAGMENT = 0x00000010,
};
enum NGFX_ShaderFormat{
    NGFX_SHADER_FORMAT_GLSL,
    NGFX_SHADER_FORMAT_SPIRV,
};
struct ShaderInfo{
    NGFX_ShaderStage  shader_stage;
    NGFX_ShaderFormat shader_code_format;
    const char* filepath;
    size_t buffer_size;
    char* buffer;
};
class Shader{
public:
    static VkShaderModule CompileGlsl(size_t buffer_size, char* buffer);
    static VkShaderModule CompileSpirv(size_t buffer_size, char* buffer);
    
public:
    Shader(NGFX_ShaderStage shader_stage, NGFX_ShaderFormat shader_code_format, size_t buffer_size, char* buffer);
    Shader(ShaderInfo info);
    ~Shader();
    
    NGFX_ShaderStage GetStage();
    VkShaderModule GetModule();
    
private:
    NGFX_ShaderStage shader_stage_;
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
    NGFX_CULL_MODE_NONE = 0,
    NGFX_CULL_MODE_FRONT_FACE = 1,
    NGFX_CULL_MODE_BACK_FACE = 2,
    NGFX_CULL_MODE_FRONT_AND_BACK_FACE = 3,
};
struct PipelineInfo{
    RenderBuffer* render_buffer;
    std::vector<Shader*> shaders;
    
    std::vector<VertexBinding>   vertex_bindings;
    std::vector<VertexAttribute> vertex_attributes;
    
    NGFX_FrontFace front_face = NGFX_FRONT_FACE_CW;
    NGFX_CullMode  cull_mode  = NGFX_CULL_MODE_NONE;
};
class Pipeline {
public:
    Pipeline();
    Pipeline(PipelineInfo info);
    ~Pipeline();
    
    void Initialize(PipelineInfo info);
    
    void Bind(VkCommandBuffer command_buffer);
    void PushConstant(VkCommandBuffer vk_command_buffer, VkDeviceSize size, VkDeviceSize offset, void* data);
    
    VkPipelineLayout vk_pipeline_layout;
    VkPipeline vk_pipeline;
};

struct ShaderCompilationInfo{
    Shader* shader;
    ShaderInfo shader_info{};
};
struct PipelineCompilationInfo{
    Pipeline* pipeline;
    PipelineInfo pipeline_info;
};

typedef uint32_t PipelineID;
namespace PipelineManager{
void Initialize();
void Terminate();

Pipeline* Compile(PipelineInfo info);
void AwaitCompilation(Pipeline* pipeline);

void Destroy(Pipeline* pipeline);


extern std::mutex compilation_mutex;
extern std::condition_variable compilation_condition_variable;
}
}
