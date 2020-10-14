#ifndef CIRCULAR_SYSTEM_H_
#define CIRCULAR_SYSTEM_H_

#include "ParticleSystemBase.hpp"


namespace GLOO {
    class CircularSystem : public ParticleSystemBase {
    public:
        // Constructor
        CircularSystem();
        ParticleState ComputeTimeDerivative(const ParticleState& state, float time) const override;
    private:
        
    };
}  // namespace GLOO

#endif