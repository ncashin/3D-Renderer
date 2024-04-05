#include "asset.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include "stb_image.h"

namespace asset{
render::Texture GetTexture(const char* filepath){
    stbi_set_flip_vertically_on_load(true);

    int width, height, component_count;
    stbi_uc* data = stbi_load(filepath, &width, &height, &component_count, STBI_rgb_alpha);
    if (!data) {
        throw std::runtime_error("failed to load texture image!");
    }
    uint32_t uwidth  = width;
    uint32_t uheight = height;
    
    render::Texture texture{};
    texture.Initialize({uwidth, uheight, 1});
    size_t image_bytesize = uwidth * uheight * STBI_rgb_alpha;
    void* staging_pointer = render::staging_manager.UploadToImage(image_bytesize, &texture);
    std::memcpy(staging_pointer, data, image_bytesize);
    stbi_image_free(data);
    return texture;
}
}

