#include "asset.h"

namespace engine{
void Asset::LoadMesh(char* vertex_destination, char* index_destination, uint32_t* vertex_count, uint32_t* index_count, const char* filepath)
{
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_FlipUVs);
    
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
        //cout << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
        return;
    }
    
    for(int i = 0; i < scene->mNumMeshes; i++){
        aiMesh* mesh = scene->mMeshes[i];
        *vertex_count += mesh->mNumVertices;
        for(int vertex_index = 0; vertex_index < mesh->mNumVertices; vertex_index++){
            *(aiVector3D*)vertex_destination = mesh->mVertices[vertex_index];
            vertex_destination += sizeof(aiVector3D);
            if(mesh->mTextureCoords[vertex_index]){
                aiVector2D vec;
                vec.x = mesh->mTextureCoords[0][vertex_index].x;
                vec.y = mesh->mTextureCoords[0][vertex_index].y;
                
                //printf("TEX COORDS: %f, %f\n", vec.x, vec.y);
                *(aiVector2D*)vertex_destination = vec;
            }else{
                aiVector2D vec(0, 0);
                //printf("TEX COORDS: %f, %f\n", vec.x, vec.y);
                *(aiVector2D*)vertex_destination = vec;
            }
            vertex_destination += sizeof(aiVector2D);
        }
    }
    
    *index_count = 0;
    for(int i = 0; i < scene->mNumMeshes; i++){
        aiMesh* mesh = scene->mMeshes[i];
        uint32_t mesh_index_offset = *index_count;
        for(int i = 0; i < mesh->mNumFaces; i++){
            aiFace face = mesh->mFaces[i];
            *index_count += face.mNumIndices;
            for(uint32_t j = 0; j < face.mNumIndices; j++){
                *(uint32_t*)index_destination = mesh_index_offset + face.mIndices[j];
                index_destination += sizeof(uint32_t);
            }
        }
    }
}
}
