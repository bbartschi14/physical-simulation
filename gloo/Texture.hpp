#ifndef GLOO_TEXTURE_H_
#define GLOO_TEXTURE_H_

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include <glad/glad.h>
#include <gloo/Image.hpp>

namespace GLOO {
class Texture {
 public:
  Texture(const std::string& texture_path, float tile_size);
  unsigned int GetTextureIndex() {
      return texture_index_;
  }
  unsigned int GetNormalIndex() {
      return normal_texture_index_;
  }
  float GetTileSize() {
      return tile_size_;
  }
  void SetTileSize(float tile_size) {
      tile_size_ = tile_size;
  }
  void SetDiffuseMap(const std::string& diffuse_path);

  void SetNormalMap(const std::string& normal_path);
  bool HasNormal() {
      return has_normal_;
  }
  void ToggleNormal() {
      has_normal_ = !has_normal_;
  }
  void ToggleDiffuse() {
      diffuse_on_ = !diffuse_on_;
  }
  bool DiffuseOn() {
      return diffuse_on_;
  }
  void ToggleVisualizeNormals() {
      visualize_normals_ = !visualize_normals_;
  }
  bool VisualizeNormals() {
      return visualize_normals_;
  }

 private:
     std::string texture_path_;
     std::string normal_path_;
     unsigned int texture_index_;
     unsigned int normal_texture_index_;
     bool has_normal_ = false;
     bool diffuse_on_ = true;
     bool visualize_normals_ = false;

     float tile_size_ = 1.0f;
};

}  // namespace GLOO

#endif
