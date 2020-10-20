#include "SimulationApp.hpp"

#include "glm/gtx/string_cast.hpp"
#include "gloo/shaders/PhongShader.hpp"
#include "gloo/shaders/CheckerShader.hpp"
#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/components/CameraComponent.hpp"
#include "gloo/components/LightComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/components/TextureComponent.hpp"
#include "gloo/MeshLoader.hpp"
#include "gloo/lights/PointLight.hpp"
#include "gloo/lights/AmbientLight.hpp"
#include "gloo/cameras/ArcBallCameraNode.hpp"
#include "gloo/debug/AxisNode.hpp"
#include "gloo/debug/PrimitiveFactory.hpp"
#include <string>
#define _USE_MATH_DEFINES
#include <math.h>
#include <Raycaster.hpp>

namespace GLOO {
SimulationApp::SimulationApp(const std::string& app_name,
                             glm::ivec2 window_size,
                             IntegratorType integrator_type,
                             float integration_step)
    : Application(app_name, window_size),
      integrator_type_(integrator_type),
      integration_step_(integration_step) {
  // TODO: remove the following two lines and use integrator type and step to
  // create integrators; the lines below exist only to suppress compiler
  // warnings.
  UNUSED(integrator_type_);
  UNUSED(integration_step_);
  std::cout << "Integration Step: " << integration_step_ << std::endl;
  shader_ = std::make_shared<CheckerShader>();

}

void SimulationApp::SetupScene() {
  SceneNode& root = scene_->GetRootNode();

  auto camera_node = make_unique<ArcBallCameraNode>();
  camera_node->GetTransform().SetPosition(glm::vec3(0.0f, 0.0f, -3.0f));
  scene_->ActivateCamera(camera_node->GetComponentPtr<CameraComponent>());
  auto camera_ptr = camera_node.get();
  root.AddChild(std::move(camera_node));

  root.AddChild(make_unique<AxisNode>('A'));

  auto ambient_light = std::make_shared<AmbientLight>();
  ambient_light->SetAmbientColor(glm::vec3(0.2f));
  root.CreateComponent<LightComponent>(ambient_light);

  auto point_light = std::make_shared<PointLight>();
  point_light->SetDiffuseColor(glm::vec3(0.8f, 0.8f, 0.8f));
  point_light->SetSpecularColor(glm::vec3(1.0f, 1.0f, 1.0f));
  //point_light->SetAttenuation(glm::vec3(1.0f, 0.09f, 0.032f));
  point_light->SetAttenuation(glm::vec3(0.75f, 0.0001f, 0.0001f));

  auto point_light_node = make_unique<SceneNode>();
  point_light_node->CreateComponent<LightComponent>(point_light);
  point_light_node->GetTransform().SetPosition(glm::vec3(7.0f, 7.0f, 7.f));
  point_light_node_ = point_light_node.get();
  root.AddChild(std::move(point_light_node));

 

  auto circular_node = make_unique<CircularNode>(integration_step_);
  circular_node->GetTransform().SetPosition(glm::vec3(-10.5f, 0.0f, 0.0f));
  root.AddChild(std::move(circular_node));

  auto pendulum_node = make_unique<PendulumNode>(integration_step_, integrator_type_);
  pendulum_node->GetTransform().SetPosition(glm::vec3(-7.5f, 0.0f, 0.0f));
  root.AddChild(std::move(pendulum_node));


  auto raycaster_node = make_unique<Raycaster>(scene_.get(), camera_ptr);
  auto raycast_node = raycaster_node.get();
  root.AddChild(std::move(raycaster_node));

  auto cloth_node = make_unique<ClothNode>(integration_step_, integrator_type_, raycast_node);
  cloth_node_ = cloth_node.get();
  root.AddChild(std::move(cloth_node));


  auto ground_node = make_unique<SceneNode>();
  ground_node->CreateComponent<ShadingComponent>(shader_);
  auto& rc = ground_node->CreateComponent<RenderingComponent>(PrimitiveFactory::CreateQuad());
  glm::vec3 ground_color(.6f, .6f, 0.6f);
  ground_node->CreateComponent<MaterialComponent>(
      std::make_shared<Material>(Material::GetDefault()));
  ground_node->GetComponentPtr<MaterialComponent>()->GetMaterial().SetDiffuseColor(ground_color);
  ground_node->GetComponentPtr<MaterialComponent>()->GetMaterial().SetAmbientColor(ground_color);
  ground_node->CreateComponent<TextureComponent>(std::make_shared<Texture>("grass.png", 10.f));
  ground_node->GetTransform().SetPosition(glm::vec3(0.f,-12.0f,0.f));
  float pi = atan(1) * 4;
  ground_node->GetTransform().SetRotation(glm::vec3(1.0f, 0.f, 0.f), pi/2);
  ground_node->GetTransform().SetScale(glm::vec3(100.0f));

  root.AddChild(std::move(ground_node));

  
}

void SimulationApp::DrawGUI() {
    ImGui::Begin("Control Panel");    
    ImGui::Text("Press R to reset simulation");
    ImGui::Text("Press N to inspect cloth normals: %s", cloth_node_->GetNormalsState() ? "ON" : "OFF");
    ImGui::Text("Press T to inspect cloth wireframe: %s", cloth_node_->GetWireframeState() ? "ON" : "OFF");

    ImGui::Text("Press B to toggle ball: %s", cloth_node_->GetBallState() ? "ON" : "OFF");
    glm::vec3 gravity = cloth_node_->GetGravity();
    ImGui::SliderFloat("X Gravity", &gravity.x, -100.f, 100.f);
    ImGui::SliderFloat("Y Gravity", &gravity.y, -100.f, 100.f);
    ImGui::SliderFloat("Z Gravity", &gravity.z, -100.f, 100.f);
    if (ImGui::Button("Reset Gravity")) {
        gravity = glm::vec3(0.f, -50.f, 0.f);
    }
    cloth_node_->SetGravity(gravity.x, gravity.y, gravity.z);
    if (ImGui::Button("Toggle Pinning")) {
        cloth_node_->TogglePins();
    }
    
    bool wind_on = cloth_node_->GetWindState();
    bool temp = wind_on;
    ImGui::Checkbox("Toggle Wind", &wind_on);
    if (wind_on != temp) {
        cloth_node_->ToggleWind();
    }
    if (wind_on) {
        float wind_strength = cloth_node_->GetWindStrength();
        ImGui::SliderFloat("Wind Strength", &wind_strength, 1.f, 20.f);
        cloth_node_->SetWindStrength(wind_strength);
    }

    if (ImGui::Button("Toggle Diffuse Map")) {
        cloth_node_->ToggleClothDiffuse();
    }
    if (ImGui::Button("Toggle Normal Map")) {
        cloth_node_->ToggleClothNormal();
    }
    if (ImGui::Button("Toggle Visualize Normals")) {
        cloth_node_->ToggleClothNormalsVis();
    }
    if (ImGui::Button("Next map")) {
        cloth_node_->NextTexture();
    }
    ImGui::End();
    }
}  // namespace GLOO
