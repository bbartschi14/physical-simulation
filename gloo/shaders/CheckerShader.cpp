#include "CheckerShader.hpp"

#include <stdexcept>

#include <glm/gtc/quaternion.hpp>
#include <glm/matrix.hpp>

#include "gloo/components/CameraComponent.hpp"
#include "gloo/components/LightComponent.hpp"
#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/components/TextureComponent.hpp"

#include "gloo/SceneNode.hpp"
#include "gloo/lights/AmbientLight.hpp"
#include "gloo/lights/PointLight.hpp"
#include "gloo/lights/DirectionalLight.hpp"

namespace GLOO {
CheckerShader::CheckerShader()
    : ShaderProgram(std::unordered_map<GLenum, std::string>{
          {GL_VERTEX_SHADER, "checker.vert"},
          {GL_FRAGMENT_SHADER, "checker.frag"}}) {
}

void CheckerShader::AssociateVertexArray(VertexArray& vertex_array) const {
  if (!vertex_array.HasPositionBuffer()) {
    throw std::runtime_error("Phong shader requires vertex positions!");
  }
  if (!vertex_array.HasNormalBuffer()) {
    throw std::runtime_error("Phong shader requires vertex normals!");
  }
  vertex_array.LinkPositionBuffer(GetAttributeLocation("vertex_position"));
  vertex_array.LinkNormalBuffer(GetAttributeLocation("vertex_normal"));      

  if (vertex_array.HasTexCoordBuffer()) {
    vertex_array.LinkTexCoordBuffer(GetAttributeLocation("vertex_tex_coord"));
  }
  if (vertex_array.HasTangentBuffer()) {
      vertex_array.LinkTangentBuffer(GetAttributeLocation("vertex_tangent"));
  }
  
}

void CheckerShader::SetTargetNode(const SceneNode& node,
                                const glm::mat4& model_matrix) const {
  // Associate the right VAO before rendering.
  AssociateVertexArray(node.GetComponentPtr<RenderingComponent>()
                           ->GetVertexObjectPtr()
                           ->GetVertexArray());

  // Set transform.
  glm::mat3 normal_matrix =
      glm::transpose(glm::inverse(glm::mat3(model_matrix)));
  SetUniform("model_matrix", model_matrix);
  SetUniform("normal_matrix", normal_matrix);

  // Set material.
  MaterialComponent* material_component_ptr =
      node.GetComponentPtr<MaterialComponent>();
  const Material* material_ptr;
  if (material_component_ptr == nullptr) {
    material_ptr = &Material::GetDefault();
  } else {
    material_ptr = &material_component_ptr->GetMaterial();
  }
  SetUniform("material1.ambient", material_ptr->GetAmbientColor());
  SetUniform("material1.diffuse", material_ptr->GetDiffuseColor());
  SetUniform("material1.specular", material_ptr->GetSpecularColor());
  SetUniform("material1.shininess", material_ptr->GetShininess());

  glm::vec3 color2(1.f, 1.f, 1.f);
  SetUniform("material2.ambient", color2);
  SetUniform("material2.diffuse", color2);
  SetUniform("material2.specular", color2);
  SetUniform("material2.shininess", 10.0f);

  TextureComponent* texture_component_ptr =
      node.GetComponentPtr<TextureComponent>();

  if (texture_component_ptr != nullptr && texture_component_ptr->GetTexture().DiffuseOn()) {

      unsigned int index = texture_component_ptr->GetTexture().GetTextureIndex();
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, index);
      SetUniform("tex.map", 0);
      SetUniform("tex.texture_on", true);
      SetUniform("tex.tile_size", texture_component_ptr->GetTexture().GetTileSize());
      
  }
  else {
      SetUniform("tex.texture_on", false);
  }
  if (texture_component_ptr != nullptr && texture_component_ptr->GetTexture().HasNormal()) {
          //std::cout << "Normal found" << std::endl;
          glActiveTexture(GL_TEXTURE1);
          glBindTexture(GL_TEXTURE_2D, texture_component_ptr->GetTexture().GetNormalIndex());
          SetUniform("tex.normal_map", 1);
          SetUniform("tex.normal_on", true);
  }
  else {
      SetUniform("tex.normal_on", false);
  }
  if (texture_component_ptr != nullptr && texture_component_ptr->GetTexture().VisualizeNormals()) {
      SetUniform("tex.visualize_normals", true);
  }
  else {
      SetUniform("tex.visualize_normals", false);
  }
  
}

void CheckerShader::SetCamera(const CameraComponent& camera) const {
  SetUniform("view_matrix", camera.GetViewMatrix());
  SetUniform("projection_matrix", camera.GetProjectionMatrix());
  SetUniform("camera_position",
             camera.GetNodePtr()->GetTransform().GetWorldPosition());
}

void CheckerShader::SetLightSource(const LightComponent& component) const {
  auto light_ptr = component.GetLightPtr();
  if (light_ptr == nullptr) {
    throw std::runtime_error("Light component has no light attached!");
  }

  // First disable all lights.
  // In a single rendering pass, only one light of one type is enabled.
  SetUniform("ambient_light.enabled", false);
  SetUniform("point_light.enabled", false);
  SetUniform("directional_light.enabled", false);

  if (light_ptr->GetType() == LightType::Ambient) {
    auto ambient_light_ptr = static_cast<AmbientLight*>(light_ptr);
    SetUniform("ambient_light.enabled", true);
    SetUniform("ambient_light.ambient", ambient_light_ptr->GetAmbientColor());
  } else if (light_ptr->GetType() == LightType::Point) {
    auto point_light_ptr = static_cast<PointLight*>(light_ptr);
    SetUniform("point_light.enabled", true);
    SetUniform("point_light.position",
               component.GetNodePtr()->GetTransform().GetPosition());
    SetUniform("point_light.diffuse", point_light_ptr->GetDiffuseColor());
    SetUniform("point_light.specular", point_light_ptr->GetSpecularColor());
    SetUniform("point_light.attenuation", point_light_ptr->GetAttenuation());
  } else if (light_ptr->GetType() == LightType::Directional) {
    auto directional_light_ptr = static_cast<DirectionalLight*>(light_ptr);
    SetUniform("directional_light.enabled", true);
    SetUniform("directional_light.direction",
               directional_light_ptr->GetDirection());
    SetUniform("directional_light.diffuse",
               directional_light_ptr->GetDiffuseColor());
    SetUniform("directional_light.specular",
               directional_light_ptr->GetSpecularColor());
  } else {
    throw std::runtime_error(
        "Encountered light type unrecognized by the shader!");
  }
}

}  // namespace GLOO
