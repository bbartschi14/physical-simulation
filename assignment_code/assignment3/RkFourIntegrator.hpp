#ifndef RK_FOUR_INTEGRATOR_H_
#define RK_FOUR_INTEGRATOR_H_

#include "IntegratorBase.hpp"

namespace GLOO {
template <class TSystem, class TState>
class RkFourIntegrator : public IntegratorBase<TSystem, TState> {
  TState Integrate(const TSystem& system,
                   const TState& state,
                   float start_time,
                   float dt) const override {

      ParticleState k1 = system.ComputeTimeDerivative(state, start_time);
      ParticleState k2 = system.ComputeTimeDerivative(state + dt / 2 * k1, start_time + dt / 2);
      ParticleState k3 = system.ComputeTimeDerivative(state + dt / 2 * k2, start_time + dt / 2);
      ParticleState k4 = system.ComputeTimeDerivative(state + dt * k3, start_time + dt);

      ParticleState end_state = state + dt/6 * (k1 + k2 + k3 + k4);

      return end_state;
  }
};
}  // namespace GLOO

#endif
