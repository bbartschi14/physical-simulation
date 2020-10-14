#ifndef SPRING_H_
#define SPRING_H_

#include <vector>
#include <stdexcept>

#include <glm/glm.hpp>

namespace GLOO {
struct Spring {
  int start;
  int end;
  float rest_length;
  float stiffness;
};

}  // namespace GLOO

#endif
