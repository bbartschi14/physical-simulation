#include "Texture.hpp"

#include "gloo/utils.hpp"


namespace GLOO {
    Texture::Texture(const std::string& texture_path, float tile_size){
        tile_size_ = tile_size;
        SetDiffuseMap(texture_path);
        
    }

    void Texture::SetNormalMap(const std::string& texture_path) {
        unsigned int texture;
        glGenTextures(1, &texture);
        normal_texture_index_ = texture;
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        std::unique_ptr<Image> textureImage = Image::LoadPNG(GetAssetDir() + texture_path, false);
        std::vector<float> data = textureImage->ToFloatData();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureImage->GetWidth(), textureImage->GetHeight(), 0, GL_RGB, GL_FLOAT, data.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        has_normal_ = true;
    }

    void Texture::SetDiffuseMap(const std::string& texture_path) {
        unsigned int texture;
        glGenTextures(1, &texture);
        texture_index_ = texture;
        diffuse_on_ = true;
        texture_path_ = texture_path;
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        std::unique_ptr<Image> textureImage = Image::LoadPNG(GetAssetDir() + texture_path, false);
        std::vector<float> data = textureImage->ToFloatData();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureImage->GetWidth(), textureImage->GetHeight(), 0, GL_RGB, GL_FLOAT, data.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        has_normal_ = true;
    }


}  // namespace GLOO
