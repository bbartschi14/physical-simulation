#include "PendulumSystem.hpp"
#include <iostream>
#include <stdexcept>

namespace GLOO {
	PendulumSystem::PendulumSystem(glm::vec3 gravity, float drag) : ParticleSystemBase() {
		// Constructor
		gravity_ = gravity;
		drag_ = drag;
		wind_on_ = false;
		wind_scalar_ = 5.0f;
	}

	ParticleState PendulumSystem::ComputeTimeDerivative(const ParticleState& state, float time) const {
		std::vector<glm::vec3> accelerations;
		std::vector<glm::vec3> velocities;

		glm::vec3 gravity_force;
		glm::vec3 drag_force;
		glm::vec3 spring_force;
		glm::vec3 acceleration;
		glm::vec3 velocity;

		glm::vec3 wind;

		if (wind_on_) {
			float windStrength = cos(time / 1) * wind_scalar_;
			wind = glm::normalize(glm::vec3(sin(time / 2), sin(time / 1), cos(time / 3))) * windStrength;
			//std::cout << wind.x << " " << wind.y << " " << wind.z << std::endl;
		}
		else {
			wind = glm::vec3(0.f);
		}

		for (int i = 0; i < state.positions.size(); i++) {
			if (fixed_particles_[i]) {
				// We use a zero acceleration to represent a fixed position particle
				velocity = glm::vec3(0.f);
				acceleration = glm::vec3(0.f, 0.f, 0.f);
			}
			else {
				velocity = state.velocities[i];
				// Calculate gravity forces
				gravity_force = glm::vec3(particle_masses_[i] * gravity_);

				// Calculate drag forces
				drag_force = glm::vec3(-drag_ * state.velocities[i]);

				// Calculate spring forces
				glm::vec3 total_spring_force{ 0.f,0.f,0.f };
				for (Spring spring : springs_per_particle_[i]) {
					int end = 0;
					if (spring.start == i) {
						end = spring.end;
					}
					else if (spring.end == i) {
						end = spring.start;
					}
					float k = spring.stiffness;
					float r = spring.rest_length;
					glm::vec3 d = state.positions[i] - state.positions[end];
					float d_length = glm::length(d);
					total_spring_force += glm::vec3(-k * (d_length - r) * (d / d_length));
				}
				spring_force = total_spring_force;

				// Sum forces and store acceleration
				acceleration = (gravity_force + drag_force + spring_force + wind) / particle_masses_[i];
				

				
			}
			velocities.push_back(velocity);
			accelerations.push_back(acceleration);
			
		}
		ParticleState derived_state{ velocities, accelerations};
		return derived_state;
	}

	void PendulumSystem::AddParticle(float mass) {
		particle_masses_.push_back(mass);
		fixed_particles_.push_back(false);
	}

	void PendulumSystem::AddSpring(int start, int end, float rest_length, float stiffness) {
		if (start >= particle_masses_.size() || end >= particle_masses_.size()) {
			throw std::runtime_error(
				"Cannot create spring, particles not in system");
		}
		else {
			springs_.push_back(Spring{ start, end, rest_length, stiffness });
		}

	}

	void PendulumSystem::FixParticle(int index) {
		if (index >= fixed_particles_.size()) {
			throw std::runtime_error(
				"Cannot fix particle, out of range");
		}
		else {
			fixed_particles_[index] = true;
		}
	}

	void PendulumSystem::PopulateSpringData() {
		for (int i = 0; i < particle_masses_.size(); i++) {
			springs_per_particle_.push_back(std::vector<Spring>());
		}
		for (Spring spring : springs_) {
			springs_per_particle_[spring.start].push_back(spring);
			springs_per_particle_[spring.end].push_back(spring);
		}
	}


}