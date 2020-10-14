#ifndef FORWARD_EULER_INTEGRATOR_H_
#define FORWARD_EULER_INTEGRATOR_H_

#include "IntegratorBase.hpp"

namespace GLOO {
template <class TSystem, class TState>
class ForwardEulerIntegrator : public IntegratorBase<TSystem, TState> {
  TState Integrate(const TSystem& system,
                   const TState& state,
                   float start_time,
                   float dt) const override {
      //std::cout << "Dt: " << dt << std::endl;
      ParticleState delta = dt * system.ComputeTimeDerivative(state, start_time + dt);
      //std::cout << "Delta: " << delta.positions[0].x << " " << delta.positions[0].y << " " << delta.positions[0].z << std::endl;

      ParticleState end_state = state + delta;

      return end_state;
  }
};
}  // namespace GLOO

#endif
