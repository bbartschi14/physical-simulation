#ifndef SIMULATION_APP_H_
#define SIMULATION_APP_H_

#include "gloo/Application.hpp"

#include "IntegratorType.hpp"
#include "CircularNode.hpp"
#include "PendulumNode.hpp"
#include "ClothNode.hpp"

namespace GLOO {
class SimulationApp : public Application {
 public:
  SimulationApp(const std::string& app_name,
                glm::ivec2 window_size,
                IntegratorType integrator_type,
                float integration_step);
  void SetupScene() override;

 private:
  IntegratorType integrator_type_;
  float integration_step_;
  void DrawGUI() override;
  ClothNode* cloth_node_;
  SceneNode* point_light_node_;
  std::shared_ptr<ShaderProgram> shader_;

};
}  // namespace GLOO

#endif
