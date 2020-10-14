#ifndef INTEGRATOR_FACTORY_H_
#define INTEGRATOR_FACTORY_H_

#include "IntegratorBase.hpp"
#include "CircularSystem.hpp"
#include "ForwardEulerIntegrator.hpp"
#include "TrapezoidalIntegrator.hpp"
#include "RkFourIntegrator.hpp"

#include <stdexcept>

#include "gloo/utils.hpp"

#include "IntegratorType.hpp"

namespace GLOO {
class IntegratorFactory {
 public:
  template <class TSystem, class TState>
  static std::unique_ptr<IntegratorBase<TSystem, TState>> CreateIntegrator(
      IntegratorType type) {
    switch (type) {
      case IntegratorType::Euler:
          return make_unique<ForwardEulerIntegrator<TSystem, TState>>();
          break;
      case IntegratorType::Trapezoidal:
          return make_unique<TrapezoidalIntegrator<TSystem, TState>>();
          break;
      case IntegratorType::RK4:
          return make_unique<RkFourIntegrator<TSystem, TState>>();
          break;

      }
  }
};
}  // namespace GLOO

#endif
