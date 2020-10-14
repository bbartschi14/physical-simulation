#ifndef PENDULUM_NODE_H_
#define PENDULUM_NODE_H_

#include "gloo/SceneNode.hpp"
#include "ParticleState.hpp"
#include "PendulumSystem.hpp"
#include "ForwardEulerIntegrator.hpp"
#include "gloo/shaders/ShaderProgram.hpp"
#include "gloo/VertexObject.hpp"

namespace GLOO {
    class PendulumNode : public SceneNode {
    public:
        // Constructor
        PendulumNode(float integration_step, IntegratorType integrator_type);
        void Update(double delta_time) override;
    private:
        ParticleState state_;
        // Set gravity and drag value for system calculations
        glm::vec3 gravity_{ 0.0f, -2.0f, 0.0f };
        float drag_ = .1f;
        PendulumSystem system_ = PendulumSystem(gravity_, drag_);
        std::unique_ptr<IntegratorBase<PendulumSystem, ParticleState>> integrator_;

        std::vector<SceneNode*> sphere_ptrs_;
        std::vector<SceneNode*> line_ptrs_;

        std::shared_ptr<ShaderProgram> shader_;
        std::shared_ptr<VertexObject> sphere_mesh_;
        double time_;
        float integration_step_;
        IntegratorType integrator_type_;
        float rollover_time_;
    };
}  // namespace GLOO

#endif

