#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "render/buffer.h"

namespace engine{
template<typename T>
class Mesh{
    Mesh();
    ~Mesh();
    
    
    
    render::BAllocation<T>       vertex_allocation;
    render::BAllocation<uint32_t> index_allocation;
};
class Asset{
public:
    static void LoadMesh(char* vertex_destination, char* index_destination, 
                         uint32_t* vertex_count, uint32_t* index_count, const char* filepath);
};
}
