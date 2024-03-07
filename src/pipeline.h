#pragma once
#include <fstream>

#include "glm.hpp"

#include "render_context.h"
#include "render_buffer.h"

namespace engine{
enum class ShaderStage{
    Vertex   = 0x00000001,
    Fragment = 0x00000010,
};
enum class ShaderCodeFormat{
    Spirv,
    Glsl,
};
struct ShaderInfo{
    ShaderStage shader_type;
    ShaderCodeFormat shader_code_format;
    size_t buffer_size;
    char* buffer;
};
class Shader{
public:
    static VkShaderModule CompileGlsl(size_t buffer_size, char* buffer);
    static VkShaderModule CompileSpirv(size_t buffer_size, char* buffer);
    
public:
    Shader(ShaderStage shader_stage, ShaderCodeFormat shader_code_format, size_t buffer_size, char* buffer);
    Shader(ShaderStage shader_stage, ShaderCodeFormat shader_code_format, const char* filepath);
    ~Shader();
    
    ShaderStage GetStage();
    VkShaderModule GetModule();
    
private:
    ShaderStage shader_stage_;
    VkShaderModule vk_shader_module_;
};

struct VertexBinding{
    uint32_t binding;
    uint32_t stride;
    bool per_instance = false;
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
    AttributeFormat format;
    uint32_t offset;
};
enum class FrontFace{
    CounterClockwise = 0,
    Clockwise = 1,
};
enum class CullMode{
    None = 0,
    FrontFace = 1,
    BackFace = 2,
    FrontAndBackFace = 3,
};
struct PipelineInfo{
    RenderBuffer* render_buffer;
    std::vector<Shader*> shaders;
    
    std::vector<VertexBinding>   vertex_bindings;
    std::vector<VertexAttribute> vertex_attributes;
    
    FrontFace front_face = FrontFace::Clockwise;
    CullMode cull_mode = CullMode::None;
};
class Pipeline {
public:
    Pipeline();
    Pipeline(PipelineInfo info);
    ~Pipeline();
    
    void Initialize(PipelineInfo info);
    
    void Bind(VkCommandBuffer command_buffer);
        
private:
    VkPipelineLayout vk_pipeline_layout_;
    VkPipeline vk_pipeline_;
};

struct ShaderFileCompilationInfo{
    Shader* shader;
    ShaderCodeFormat shader_code_format;
    const char* filepath;
};
struct PipelineCompilationInfo{
    Pipeline* pipeline;
    PipelineInfo pipeline_info;
};

}
