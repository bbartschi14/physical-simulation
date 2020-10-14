#include "ClothNode.hpp"

#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/debug/PrimitiveFactory.hpp"
#include "gloo/shaders/PhongShader.hpp"
#include "IntegratorFactory.hpp"
#include "gloo/shaders/SimpleShader.hpp"
#include "gloo/InputManager.hpp"

#include <algorithm>

namespace GLOO {
	ClothNode::ClothNode(float integration_step, IntegratorType integrator_type) : SceneNode() {
		// Constructor
		time_ = 0.0f;
		integration_step_ = integration_step;
		integrator_type_ = integrator_type;
		rollover_time_ = 0.0f;
		integrator_ = IntegratorFactory::CreateIntegrator<PendulumSystem, ParticleState>(integrator_type);
		
		

		// ---- Cloth Properties ----
		cloth_size_ = 12;
		cloth_width_ = 10.0f;
		// --------------------------

		// ---- Spring Properties ----
		float structural_rest_length = cloth_width_/float(cloth_size_);
		float shear_rest_length = sqrt(2) * cloth_width_ / float(cloth_size_);
		float flex_rest_length = 2 * cloth_width_ / float(cloth_size_);

		float stiffness = 40.0f;
		float mass = .5f;
		glm::vec3 start_pos = glm::vec3(0.f, 0.f, 0.f);
		// -------------------------

		float z_start = 0.f;
		for (int i = 0; i < cloth_size_; i++) {
			for (int j = 0; j < cloth_size_; j++) {
				glm::vec3 new_pos(start_pos[0] + i * cloth_width_ / float(cloth_size_), 
								  start_pos[1] - j * cloth_width_ / float(cloth_size_), 
								  j*.5f);
				//std::cout << "New pos: " << new_pos.x << " " << new_pos.y << " " << new_pos.z << std::endl;
				initial_positions_.push_back(new_pos);
			}
		}
		state_.positions = initial_positions_;
		std::vector<float> masses;

		for (int i = 0; i < state_.positions.size(); i++) {
			state_.velocities.push_back(glm::vec3(0.f, 0.f, 0.f));
			masses.push_back(mass);
		}

		shader_ = std::make_shared<PhongShader>();
		sphere_mesh_ = PrimitiveFactory::CreateSphere(0.1f, 25, 25);

		// Create one sphere for each particle and initialize particles in system
		for (int i = 0; i < state_.positions.size(); i++) {
			auto sphere_node = make_unique<SceneNode>();
			sphere_node->CreateComponent<ShadingComponent>(shader_);
			sphere_node->CreateComponent<RenderingComponent>(sphere_mesh_);
			sphere_node->GetTransform().SetPosition(state_.positions[i]);
			sphere_ptrs_.push_back(sphere_node.get());
			AddChild(std::move(sphere_node));
			sphere_ptrs_[i]->SetActive(false);

			system_.AddParticle(masses[i]);
		}
		for (int col = 0; col < cloth_size_; col++) {
			for (int row = 0; row < cloth_size_; row++) {

				// ---- Structural Springs ----
				if (col < cloth_size_ - 1) {
					CreateSpring(IndexOf(row, col), IndexOf(row, col + 1), structural_rest_length, stiffness, true);
				}
				if (row < cloth_size_ - 1) {
					CreateSpring(IndexOf(row, col), IndexOf(row+1, col), structural_rest_length, stiffness, true);
				}
				// ---- Shear Springs ----
				if (col < cloth_size_ - 1 && row < cloth_size_ - 1) {
					CreateSpring(IndexOf(row, col), IndexOf(row + 1, col + 1), shear_rest_length, stiffness, false);
				}
				if (row > 0 && col < cloth_size_ - 1) {
					CreateSpring(IndexOf(row, col), IndexOf(row - 1, col + 1), shear_rest_length, stiffness, false);
				}
				// ---- Flex Springs ----
				if (col < cloth_size_ - 2) {
					CreateSpring(IndexOf(row, col), IndexOf(row, col + 2), flex_rest_length, stiffness, false);
				}
				if (row  < cloth_size_ - 2) {
					CreateSpring(IndexOf(row, col), IndexOf(row + 2, col), flex_rest_length, stiffness, false);
				}
			}

		}
		// Pin top left and top right particles
		system_.FixParticle(IndexOf(0,0));
		system_.FixParticle(IndexOf(0, cloth_size_-1));

		system_.PopulateSpringData();

		// Create cloth mesh
		cloth_mesh_ = std::make_shared<VertexObject>();
		auto mesh_node = make_unique<SceneNode>();
		mesh_node->CreateComponent<ShadingComponent>(shader_);
		glm::vec3 color(.9f, 0.9f, 0.9f);
		mesh_node->CreateComponent<MaterialComponent>(
			std::make_shared<Material>(Material::GetDefault()));
		mesh_node->GetComponentPtr<MaterialComponent>()->GetMaterial().SetDiffuseColor(color);
		mesh_node->GetComponentPtr<MaterialComponent>()->GetMaterial().SetAmbientColor(color);

		auto indices = make_unique<IndexArray>();
		for (int col = 0; col < cloth_size_; col++) {
			for (int row = 0; row < cloth_size_; row++) {
				if (row < cloth_size_ - 1 && col < cloth_size_ - 1) {
					int i = IndexOf(row, col);
					indices->push_back(i);
					indices->push_back(i + 1);
					indices->push_back(i + cloth_size_);
					indices->push_back(i + cloth_size_);
					indices->push_back(i + 1);
					indices->push_back(i + cloth_size_ + 1);
				}
			}
		}
		cloth_mesh_->UpdateIndices(std::move(indices));

		DrawClothPositions();
		FindIncidentTriangles();
		UpdateClothNormals();
		auto& rc = mesh_node->CreateComponent<RenderingComponent>(cloth_mesh_);
		rc.SetDrawMode(DrawMode::Triangles);
		cloth_mesh_node_ = mesh_node.get();

		AddChild(std::move(mesh_node));


	}

	void ClothNode::Update(double delta_time) {
		static bool prev_released = true;
		if (InputManager::GetInstance().IsKeyPressed('R')) {
			if (prev_released) {
				ResetSystem();
			}
			prev_released = false;
		}
		else if (InputManager::GetInstance().IsKeyPressed('T')) {
			if (prev_released) {
				ToggleWireframe();
			}
			prev_released = false;
		}
		else {
			prev_released = true;
		}


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
			num_steps = int(rollover_time_/dt);
			rollover_time_ -= dt * num_steps;
		}

		for (int i = 0; i < num_steps; i++) {
			state_ = integrator_->Integrate(system_, state_, time_, dt);
			time_ += dt;
		}

		for (int i = 0; i < sphere_ptrs_.size(); i++) {
			sphere_ptrs_[i]->GetTransform().SetPosition(state_.positions[i]);
		}
		for (int line_index = 0; line_index < line_ptrs_.size(); line_index++) {
			if (line_ptrs_[line_index]->IsActive()) {
				auto positions = make_unique<PositionArray>();
				positions->push_back(state_.positions[line_indices_[line_index * 2]]);
				positions->push_back(state_.positions[line_indices_[(line_index * 2) + 1]]);
				line_ptrs_[line_index]->GetComponentPtr<RenderingComponent>()->GetVertexObjectPtr()->UpdatePositions(std::move(positions));
			}
			
		}
		DrawClothPositions();
		UpdateClothNormals();
	}

	int ClothNode::IndexOf(int row, int col) {
		if (row >= cloth_size_ || row < 0 || col >= cloth_size_ || col < 0) return -1;
		int index = row + col * cloth_size_;
		return index;
	}

	void ClothNode::CreateSpring(int start, int end, float rest_length, float stiffness, bool render) {
		system_.AddSpring(start, end, rest_length, stiffness);

		// Create a line between particles to visualize springs
		if (render) {
			auto line = std::make_shared<VertexObject>();
			auto line_shader = std::make_shared<SimpleShader>();
			auto indices = IndexArray({ 0,1 });
			line->UpdateIndices(make_unique<IndexArray>(indices));
			auto positions = PositionArray({ state_.positions[start] , state_.positions[end] });
			line->UpdatePositions(make_unique<PositionArray>(positions));

			auto line_node = make_unique<SceneNode>();
			line_node->CreateComponent<ShadingComponent>(line_shader);
			auto& rc_line = line_node->CreateComponent<RenderingComponent>(line);
			rc_line.SetDrawMode(DrawMode::Lines);
			auto color = glm::vec3(1.f, 0.f, 0.f);
			auto material = std::make_shared<Material>(color, color, color, 0.0f);
			line_node->CreateComponent<MaterialComponent>(material);
			line_ptrs_.push_back(line_node.get());
			line_indices_.push_back(start);
			line_indices_.push_back(end);
			AddChild(std::move(line_node));
			line_ptrs_.back()->SetActive(false);

			
		}
		
	}

	void ClothNode::ResetSystem() {
		time_ = 0.0f;
		rollover_time_ = 0.0f;
		state_.positions = initial_positions_;
		state_.velocities.clear();
		for (int i = 0; i < state_.positions.size(); i++) {
			state_.velocities.push_back(glm::vec3(0.f, 0.f, 0.f));
		}
	}

	void ClothNode::DrawClothPositions() {
		auto positions = make_unique<PositionArray>();

		for (int col = 0; col < cloth_size_; col++) {
			for (int row = 0; row < cloth_size_; row++) {
				int i = IndexOf(row, col);
				positions->push_back(state_.positions[i]);
			}
		}
		cloth_mesh_->UpdatePositions(std::move(positions));
	}

	void ClothNode::UpdateClothNormals() {
		auto indices = cloth_mesh_->GetIndices();
		auto positions = cloth_mesh_->GetPositions();
		auto new_normals = make_unique<NormalArray>();
		for (int position_index = 0; position_index < positions.size(); position_index++) {
			glm::vec3 vertex_norm = glm::vec3(0.0f);
			for (int tri : incident_triangles_[position_index]) {
				glm::vec3 a = positions[indices[(tri * 3)]];
				glm::vec3 b = positions[indices[(tri * 3) + 1]];
				glm::vec3 c = positions[indices[(tri * 3) + 2]];

				glm::vec3 tri_norm = FindTriNorm(a,b,c);

				vertex_norm += tri_norm * FindTriArea(a, b, c);
			}
			new_normals->push_back(glm::normalize(vertex_norm));
		}

		cloth_mesh_->UpdateNormals(std::move(new_normals));
	}

	glm::vec3 ClothNode::FindTriNorm(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
		glm::vec3 u = b - a;
		glm::vec3 v = c - a;

		glm::vec3 tri_norm = glm::normalize(glm::cross(u, v));

		return tri_norm;

	}

	float ClothNode::FindTriArea(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
		float u_mag = glm::length(glm::cross(b - a, c - a));
		return u_mag / 2;

	}

	void ClothNode::FindIncidentTriangles() {
		auto indices = cloth_mesh_->GetIndices();
		auto positions = cloth_mesh_->GetPositions();
		std::vector<std::vector<int>> incident_tris;
		for (int position_index = 0; position_index < positions.size(); position_index++) {
			std::vector<int> tris;
			for (int i = 0; i < indices.size() - 2; i += 3) {
				if (indices[i] == position_index || indices[i + 1] == position_index || indices[i + 2] == position_index) {
					tris.push_back(i / 3);
				}
			}
			incident_tris.push_back(tris);
		}
		incident_triangles_ = incident_tris;
	}

	void ClothNode::ToggleWireframe() {
		wireframe_on_ = !wireframe_on_;
		for (auto ptr : sphere_ptrs_) {
			ptr->SetActive(wireframe_on_);
		}
		for (auto ptr : line_ptrs_) {
			ptr->SetActive(wireframe_on_);
		}
		cloth_mesh_node_->SetActive(!wireframe_on_);
		
	}
}