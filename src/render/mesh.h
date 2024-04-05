#pragma once

#include "render/pipeline.h"
#include "render/buffer.h"

#define MESH_VERTEX_STRUCT struct

#define MVS_POSITION(VAR) union{ glm::vec3 MVS_position; glm::vec3 VAR; }

#define MVS_TEXTURE_COORDINATE_2D
#define MVS_TEXTURE_COORDINATE_2D(VAR) union{ glm::vec2 MVS_texture_coordinate_2d; glm::vec2 VAR; }
#define MVS_TEXTURE_COORDINATE_3D(VAR) union{ glm::vec3 MVS_texture_coordinate_3d; glm::vec3 VAR; }

#define MVS_NORMAL(VAR) union{ glm::vec3 MVS_NORMAL; glm::vec3 VAR; }


namespace render{
typedef uint32_t MeshAttributeFlagBits;
enum MeshAttributeFlags{
    MESH_ATTRIBUTE_POSITION = 1,
    MESH_ATTRIBUTE_TEXTURE_COORDINATE_2D = 2,
    MESH_ATTRIBUTE_TEXTURE_COORDINATE_3D = 4,
    MESH_ATTRIBUTE_NORMAL = 8,
};

namespace MVS{
template <typename T, typename = int>
struct HasPosition : std::false_type { };

template <typename T>
struct HasPosition <T, decltype((void) T::MVS_position, 0)> : std::true_type { };

template <typename T, typename = int>
struct HasTextureCoordinate2D : std::false_type { };

template <typename T>
struct HasTextureCoordinate2D <T, decltype((void) T::MVS_texture_coordinate_2d, 0)> : std::true_type { };

template <typename T, typename = int>
struct HasTextureCoordinate3D : std::false_type { };

template <typename T>
struct HasTextureCoordinate3D <T, decltype((void) T::MVS_texture_coordinate_3d, 0)> : std::true_type { };

template <typename T, typename = int>
struct HasNormal : std::false_type { };

template <typename T>
struct HasNormal <T, decltype((void) T::MVS_normal, 0)> : std::true_type { };

template<typename T>
constexpr VertexBinding Binding(const uint32_t binding){
    return {binding, sizeof(T), VK_VERTEX_INPUT_RATE_VERTEX};
}

template<typename T>
constexpr VertexAttribute PositionAttribute(const uint32_t location, const uint32_t binding){
    if constexpr(HasPosition<T>()){
        return {location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(T, MVS_position)};
    } else {
        return {};
    }
};

template<typename T>
constexpr VertexAttribute TextureCoordinate2DAttribute(const uint32_t location, const uint32_t binding){
    if constexpr(HasTextureCoordinate2D<T>()){
        return {location, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(T, MVS_texture_coordinate_2d)};
    } else {
        return {};
    }
};
}

template<typename T>
class Mesh{
public:
     Mesh(){};
    ~Mesh(){};
    
    void Initialize(uint32_t vertex_count, uint32_t index_count){
        vertex_allocation = gpu_buffer.Allocate<T>(vertex_count);
        index_allocation  = gpu_buffer.Allocate<uint32_t>(index_count);
    }
    void Terminate(){
        gpu_buffer.Free(vertex_allocation);
        gpu_buffer.Free(index_allocation);
    }
    
    void Draw(VkCommandBuffer vk_command_buffer, uint32_t instance_count, uint32_t instance_offset){
        vkCmdDrawIndexed(vk_command_buffer,
                         index_allocation.count,  instance_count,
                         index_allocation.offset, vertex_allocation.offset, instance_offset);
    }
    /*template<typename IT>
    void Draw(VkCommandBuffer vk_command_buffer, BAllocation<IT> instance_allocation,
              const uint32_t instance_offset, const uint32_t instance_count){
        vkCmdDrawIndexed(vk_command_buffer,
                         index_allocation.count, instance_count,
                         index_allocation.offset, vertex_allocation.offset, instance_allocation.offset + instance_offset);
    }
    template<typename IT>
    void Draw(VkCommandBuffer vk_command_buffer, BAllocation<IT> instance_allocation){
        vkCmdDrawIndexed(vk_command_buffer,
                         index_allocation.count, instance_allocation.count,
                         index_allocation.offset, vertex_allocation.offset, instance_allocation.offset);
    }*/
    
    render::TBAllocation<T>       vertex_allocation;
    render::TBAllocation<uint32_t> index_allocation;
};
}
