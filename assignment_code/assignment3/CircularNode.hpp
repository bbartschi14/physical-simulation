#ifndef CIRCULAR_NODE_H_
#define CIRCULAR_NODE_H_

#include "gloo/SceneNode.hpp"
#include "ParticleState.hpp"
#include "CircularSystem.hpp"
#include "ForwardEulerIntegrator.hpp"
#include "gloo/shaders/ShaderProgram.hpp"
#include "gloo/VertexObject.hpp"
#include "IntegratorType.hpp"

namespace GLOO {
    class CircularNode : public SceneNode {
    public:
        // Constructor
        CircularNode(float integration_step);
        void Update(double delta_time) override;
    private:
        ParticleState state_;
        ParticleState state_2_;

        CircularSystem system_;
        std::unique_ptr<IntegratorBase<CircularSystem, ParticleState>> integrator_;
        std::unique_ptr<IntegratorBase<CircularSystem, ParticleState>> integrator_2_;

        SceneNode* sphere_;
        SceneNode* sphere_2_;

        std::shared_ptr<ShaderProgram> shader_;
        std::shared_ptr<VertexObject> sphere_mesh_;
        double time_;
        float integration_step_;
        float rollover_time_;
    };
}  // namespace GLOO

#endif

