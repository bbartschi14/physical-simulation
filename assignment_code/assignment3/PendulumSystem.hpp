#ifndef PENDULUM_SYSTEM_H_
#define PENDULUM_SYSTEM_H_

#include "ParticleSystemBase.hpp"
#include "Spring.hpp"
#include <vector>
#include "IntegratorType.hpp"

namespace GLOO {
    class PendulumSystem : public ParticleSystemBase {
    public:
        // Constructor
        PendulumSystem(glm::vec3 gravity, float drag);
        ParticleState ComputeTimeDerivative(const ParticleState& state, float time) const override;
        void AddParticle(float mass);
        void AddSpring(int start, int end, float rest_length, float stiffness);
        void FixParticle(int index);
        void PopulateSpringData();
    private:
        std::vector<std::vector<Spring>> springs_per_particle_;
        std::vector<Spring> springs_;
        std::vector<bool> fixed_particles_;
        std::vector<float> particle_masses_;
        glm::vec3 gravity_;
        float drag_;

    };
}  // namespace GLOO

#endif