#include "CircularNode.hpp"

#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/debug/PrimitiveFactory.hpp"
#include "gloo/shaders/PhongShader.hpp"
#include "IntegratorFactory.hpp"
#include <algorithm>
namespace GLOO {
	CircularNode::CircularNode(float integration_step) : SceneNode() {
		// Constructor
		time_ = 0.0f;
		integration_step_ = integration_step;
		rollover_time_ = 0.0f;
		integrator_ = IntegratorFactory::CreateIntegrator<CircularSystem, ParticleState>(IntegratorType::Euler);
		integrator_2_ = IntegratorFactory::CreateIntegrator<CircularSystem, ParticleState>(IntegratorType::Trapezoidal);

		glm::vec3 initial_pos{ 1.f,0.f,0.f };

		state_.positions = std::vector<glm::vec3>{ initial_pos };
		state_.velocities = std::vector<glm::vec3>{ glm::vec3(0.f,0.f,0.f) };

		
		shader_ = std::make_shared<PhongShader>();
		sphere_mesh_ = PrimitiveFactory::CreateSphere(0.2f, 25, 25);
		auto sphere_node = make_unique<SceneNode>();
		sphere_node->CreateComponent<ShadingComponent>(shader_);
		sphere_node->CreateComponent<RenderingComponent>(sphere_mesh_);
		sphere_node->GetTransform().SetPosition(state_.positions[0]);
		sphere_ = sphere_node.get();
		AddChild(std::move(sphere_node));


		state_2_.positions = std::vector<glm::vec3>{ initial_pos };
		state_2_.velocities = std::vector<glm::vec3>{ glm::vec3(0.f,0.f,0.f) };

		glm::vec3 color(0.0f, 1.0f, 0.0f);
		auto material = std::make_shared<Material>(color, color, color, 0);

		auto sphere_node_2 = make_unique<SceneNode>();
		sphere_node_2->CreateComponent<ShadingComponent>(shader_);
		sphere_node_2->CreateComponent<RenderingComponent>(sphere_mesh_);
		sphere_node_2->CreateComponent<MaterialComponent>(material);
		sphere_node_2->GetTransform().SetPosition(state_.positions[0]);
		sphere_2_ = sphere_node_2.get();
		AddChild(std::move(sphere_node_2));
	}

	void CircularNode::Update(double delta_time) {
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
			num_steps = int(rollover_time_ / dt);
			rollover_time_ -= dt * num_steps;

		}
		for (int i = 0; i < num_steps; i++) {
			state_ = integrator_->Integrate(system_, state_, time_, dt);
			state_2_ = integrator_2_->Integrate(system_, state_2_, time_, dt);
			//std::cout << "New Pos: " << state_.positions[0].x << " " << state_.positions[0].y << " " << state_.positions[0].z << " " << std::endl;

			time_ += dt;
		}
		sphere_->GetTransform().SetPosition(state_.positions[0]);
		sphere_2_->GetTransform().SetPosition(state_2_.positions[0]);

	}

}