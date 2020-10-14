#include "PendulumNode.hpp"

#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/debug/PrimitiveFactory.hpp"
#include "gloo/shaders/PhongShader.hpp"
#include "IntegratorFactory.hpp"
#include "gloo/shaders/SimpleShader.hpp"

#include <algorithm>
namespace GLOO {
	PendulumNode::PendulumNode(float integration_step, IntegratorType integrator_type) : SceneNode() {
		// Constructor
		time_ = 0.0f;
		integration_step_ = integration_step;
		integrator_type_ = integrator_type;
		rollover_time_ = 0.0f;
		integrator_ = IntegratorFactory::CreateIntegrator<PendulumSystem, ParticleState>(integrator_type);

		// ---- Variables to set ----
		float rest_length = .75f;
		float stiffness = 20.0f;
		float mass = 1.5f;
		std::vector<glm::vec3> initial_positions{ glm::vec3(0.f,0.f,0.f),
												  glm::vec3(.5f,-1.f,0.f),
												  glm::vec3(0.f,-2.f,0.f),
												  glm::vec3(-.2f,-3.f,0.f)
												   };
		// -------------------------

		state_.positions = initial_positions;
		std::vector<float> masses;

		for (int i = 0; i < state_.positions.size(); i++) {
			state_.velocities.push_back(glm::vec3(0.f, 0.f, 0.f));
			masses.push_back(mass);
		}

		shader_ = std::make_shared<PhongShader>();
		sphere_mesh_ = PrimitiveFactory::CreateSphere(0.1f, 25, 25);

		// Create one sphere for each particle, initialize  particles in system, and create a chain between sequential particles
		for (int i = 0; i < state_.positions.size(); i++) {
			auto sphere_node = make_unique<SceneNode>();
			sphere_node->CreateComponent<ShadingComponent>(shader_);
			sphere_node->CreateComponent<RenderingComponent>(sphere_mesh_);
			sphere_node->GetTransform().SetPosition(state_.positions[i]);
			sphere_ptrs_.push_back(sphere_node.get());
			AddChild(std::move(sphere_node));

			system_.AddParticle(masses[i]);

			if (i > 0) {
				system_.AddSpring(i - 1, i, rest_length, stiffness);

				// Create a line between particles to visualize springs
				auto line = std::make_shared<VertexObject>();
				auto line_shader = std::make_shared<SimpleShader>();
				auto indices = IndexArray({ 0,1 });
				line->UpdateIndices(make_unique<IndexArray>(indices));
				auto positions = PositionArray({ state_.positions[i - 1] , state_.positions[i] });
				line->UpdatePositions(make_unique<PositionArray>(positions));

				auto line_node = make_unique<SceneNode>();
				line_node->CreateComponent<ShadingComponent>(line_shader);
				auto& rc_line = line_node->CreateComponent<RenderingComponent>(line);
				rc_line.SetDrawMode(DrawMode::Lines);
				auto color = glm::vec3(1.f, 0.f, 0.f);
				auto material = std::make_shared<Material>(color, color, color, 0.0f);
				line_node->CreateComponent<MaterialComponent>(material);
				line_ptrs_.push_back(line_node.get());
				AddChild(std::move(line_node));
			}
		}

		system_.FixParticle(0);
	}

	void PendulumNode::Update(double delta_time) {
		rollover_time_ += delta_time;
		float dt = 0.0f;
		int num_steps = 0;
		if (float(delta_time) < integration_step_) {
			dt = float(delta_time);
			num_steps = 1;
			rollover_time_ = 0.0f;
		}
		else {
			dt = integration_step_;
			num_steps = std::fmod(rollover_time_, dt);
			rollover_time_ -= dt * num_steps;
		}
		for (int i = 0; i < num_steps; i++) {
			state_ = integrator_->Integrate(system_, state_, time_, dt);
			time_ += dt;
		}

		for (int i = 0; i < sphere_ptrs_.size(); i++) {
			sphere_ptrs_[i]->GetTransform().SetPosition(state_.positions[i]);
			if (i > 0) {
				auto positions = make_unique<PositionArray>();
				positions->push_back(state_.positions[i - 1]);
				positions->push_back(state_.positions[i]);
				line_ptrs_[i-1]->GetComponentPtr<RenderingComponent>()->GetVertexObjectPtr()->UpdatePositions(std::move(positions));
			}
		}

	}

}