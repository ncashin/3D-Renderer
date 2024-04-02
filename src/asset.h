#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "render/mesh.h"
#include "render/staging.h"

namespace engine{
namespace asset{
enum Type{
    TYPE_SHADER,
    TYPE_IMAGE,
    TYPE_MESH,
};



template<typename T>
render::Mesh<T> GetMesh(const char* filepath){
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filepath,      
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_Triangulate |
                                             aiProcess_FlipUVs);
    
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
        //cout << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
        return;
    }
    uint32_t vertex_count = 0;
    uint32_t index_count  = 0;
    
    for(uint32_t i = 0; i < scene->mNumMeshes; i++){
        vertex_count += scene->mMeshes[i]->mNumVertices;
    }
    for(uint32_t mesh_i = 0; mesh_i < scene->mNumMeshes; mesh_i++){
        aiMesh* mesh = scene->mMeshes[mesh_i];
        for(uint32_t face_i = 0; face_i < mesh->mNumFaces; face_i++){
            aiFace face = mesh->mFaces[face_i];
            index_count += face.mNumIndices;
        }
    }
    
    printf("vert: %u, index: %u\n", vertex_count, index_count);
    render::Mesh<T> render_mesh{};
    render_mesh.Initialize(vertex_count, index_count);
    T* vertex_destination =
    (T*)render::staging_manager.UploadToTBAllocation(render::gpu_buffer, render_mesh.vertex_allocation);
    uint32_t* index_destination =
    (uint32_t*)render::staging_manager.UploadToTBAllocation(render::gpu_buffer, render_mesh.index_allocation);
    
    for(uint32_t i = 0; i < scene->mNumMeshes; i++){
        aiMesh* mesh = scene->mMeshes[i];
        for(uint32_t vertex_index = 0; vertex_index < mesh->mNumVertices; vertex_index++){
            if constexpr (render::MeshVertexStruct::HasPosition<T>()){
                *(aiVector3D*)&vertex_destination->MVS_position = mesh->mVertices[vertex_index];
            }
            if constexpr (render::MeshVertexStruct::HasTextureCoordinate2D<T>()){
                if(mesh->mTextureCoords[0] != nullptr){
                    aiVector2D coordinate;
                    coordinate.x = mesh->mTextureCoords[0][vertex_index].x;
                    coordinate.y = mesh->mTextureCoords[0][vertex_index].y;
                    *(aiVector2D*)&vertex_destination->MVS_texture_coordinate_2d = coordinate;
                }else{
                    *(aiVector2D*)&vertex_destination->MVS_texture_coordinate_2d = {};
                }
            }
            if constexpr (render::MeshVertexStruct::HasTextureCoordinate3D<T>()){
                if(mesh->mTextureCoords[0] != nullptr){
                    *(aiVector3D*)&vertex_destination->MVS_texture_coordinate_3d 
                    = mesh->mTextureCoords[0][vertex_index];
                }else{
                    *(aiVector3D*)&vertex_destination->MVS_texture_coordinate_3d = {};
                }
            }
            if constexpr (render::MeshVertexStruct::HasNormal<T>()){
                if(mesh->mNormals != nullptr){
                    *(aiVector3D*)&vertex_destination->MVS_normal = mesh->mNormals[vertex_index];
                }else{
                    *(aiVector3D*)&vertex_destination->MVS_normal = {};
                }
            }
            ++vertex_destination;
        }
    }
    uint32_t mesh_index_offset = 0;
    for(uint32_t mesh_i = 0; mesh_i < scene->mNumMeshes; mesh_i++){
        aiMesh* mesh = scene->mMeshes[mesh_i];
        for(uint32_t face_i = 0; face_i < mesh->mNumFaces; face_i++){
            aiFace face = mesh->mFaces[face_i];
            for(uint32_t index_i = 0; index_i < face.mNumIndices; index_i++){
                *index_destination = mesh_index_offset + face.mIndices[index_i];
                ++index_destination;
            }       
        }
        mesh_index_offset += mesh->mNumVertices;
    }
    return render_mesh;
    return render_mesh;
};
}
}
