#ifndef GLOO_TEXTURE_COMPONENT_H_
#define GLOO_TEXTURE_COMPONENT_H_

#include "ComponentBase.hpp"

#include "gloo/Texture.hpp"

namespace GLOO {
class TextureComponent : public ComponentBase {
 public:
     TextureComponent(std::shared_ptr<Texture> texture) {
    SetTexture(std::move(texture));
  }

  void SetTexture(std::shared_ptr<Texture> texture) {
      texture_ = std::move(texture);
  }

  Texture& GetTexture() {
    return *texture_;
  }

 private:
  std::shared_ptr<Texture> texture_;
};

CREATE_COMPONENT_TRAIT(TextureComponent, ComponentType::Texture);
}  // namespace GLOO

#endif
