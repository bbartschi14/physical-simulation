#include "CircularSystem.hpp"
#include <iostream>

namespace GLOO {
	CircularSystem::CircularSystem() : ParticleSystemBase() {
		// Constructor
	}

	ParticleState CircularSystem::ComputeTimeDerivative(const ParticleState& state, float time) const {
		// Circular system, represented by Eq. 2
		std::vector<glm::vec3> new_positions;
		glm::vec3 pos_0 = state.positions[0];
		new_positions.push_back(glm::vec3(-pos_0.y, pos_0.x, 0.f));
		std::vector<glm::vec3> new_velocities;
		new_velocities.push_back(glm::vec3(0.f,0.f,0.f));

		ParticleState derived_state{ new_positions , new_velocities };
		return derived_state;
	}

}