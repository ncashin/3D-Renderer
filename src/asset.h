#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace engine{
struct Mesh{
    
};
class Asset{
public:
    static void LoadMesh(char* destination, uint32_t* vertex_count, uint32_t* index_count, const char* filepath);
};
}
