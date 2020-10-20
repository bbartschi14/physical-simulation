#include "ClothNode.hpp"

#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/components/TextureComponent.hpp"

#include "gloo/debug/PrimitiveFactory.hpp"
#include "gloo/shaders/PhongShader.hpp"
#include "IntegratorFactory.hpp"
#include "gloo/shaders/SimpleShader.hpp"
#include "gloo/InputManager.hpp"
#include "gloo/shaders/CheckerShader.hpp"

#include <algorithm>

namespace GLOO {
	ClothNode::ClothNode(float integration_step, IntegratorType integrator_type, Raycaster* raycaster) : SceneNode() {
		// Constructor
		raycaster_ = raycaster;
		time_ = 0.0f;
		integration_step_ = integration_step;
		integrator_type_ = integrator_type;
		rollover_time_ = 0.0f;
		integrator_ = IntegratorFactory::CreateIntegrator<PendulumSystem, ParticleState>(integrator_type);
		wireframe_on_ = false;
		normals_on_ = false;
		pause_on_ = false;
		wind_on_ = false;
		ball_collision_ = true;
		pinned_ = 2;
		ball_start_pos_ = glm::vec3(3.f, -8.f, 7.5f);

		// ---- Cloth Properties ----
		cloth_size_ = 12;
		cloth_width_ = 10.0f;
		// --------------------------

		// ---- Spring Properties ----
		float structural_rest_length = cloth_width_/float(cloth_size_);
		float shear_rest_length = sqrt(2) * cloth_width_ / float(cloth_size_);
		float flex_rest_length = 2 * cloth_width_ / float(cloth_size_);

		float stiffness = 150.f;
		float mass = .075f;
		glm::vec3 start_pos = glm::vec3(0.f, 0.f, 0.f);
		// -------------------------

		float z_start = 0.f;
		for (int i = 0; i < cloth_size_; i++) {
			for (int j = 0; j < cloth_size_; j++) {
				glm::vec3 new_pos(start_pos[0] + i * cloth_width_ / float(cloth_size_), 
								  start_pos[1] - j * cloth_width_ / float(cloth_size_), 
					              0.f);
								  //j*.5f);
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
				float shear_scalar = 1.f;
				if (col < cloth_size_ - 1 && row < cloth_size_ - 1) {
					CreateSpring(IndexOf(row, col), IndexOf(row + 1, col + 1), shear_rest_length * shear_scalar, stiffness, false);
				}
				if (row > 0 && col < cloth_size_ - 1) {
					CreateSpring(IndexOf(row, col), IndexOf(row - 1, col + 1), shear_rest_length * shear_scalar, stiffness, false);
				}
				// ---- Flex Springs ----
				float flex_scalar = 1.3f;
				if (col < cloth_size_ - 2) {
					CreateSpring(IndexOf(row, col), IndexOf(row, col + 2), flex_rest_length, stiffness * flex_scalar, false);
				}
				if (row  < cloth_size_ - 2) {
					CreateSpring(IndexOf(row, col), IndexOf(row + 2, col), flex_rest_length, stiffness * flex_scalar, false);
				}
			}

		}
		// Pin top left and top right particles
		FixCorners();
		

		system_.PopulateSpringData();

		// Create intersection ball
		auto ball_node = make_unique<SceneNode>();
		ball_node->CreateComponent<ShadingComponent>(shader_);
		ball_radius_ = 2.f;
		ball_node->CreateComponent<RenderingComponent>(PrimitiveFactory::CreateSphere(ball_radius_, 25, 25));
		glm::vec3 ball_color(.3f, 0.3f, 0.9f);
		ball_node->CreateComponent<MaterialComponent>(
			std::make_shared<Material>(Material::GetDefault()));
		ball_node->GetComponentPtr<MaterialComponent>()->GetMaterial().SetDiffuseColor(ball_color);
		ball_node->GetComponentPtr<MaterialComponent>()->GetMaterial().SetAmbientColor(ball_color);
		ball_node->GetTransform().SetPosition(ball_start_pos_);
		ball_ptr_ = ball_node.get();
		AddChild(std::move(ball_node));

		// Create ray collision mesh
		auto collision_node = make_unique<SceneNode>();
		collision_node->CreateComponent<ShadingComponent>(shader_);
		collision_node->CreateComponent<RenderingComponent>(PrimitiveFactory::CreateSphere(.45f, 4, 4));
		glm::vec3 collision_color(1.f, 1.f, 1.f);
		collision_node->CreateComponent<MaterialComponent>(
			std::make_shared<Material>(Material::GetDefault()));
		collision_node->GetComponentPtr<MaterialComponent>()->GetMaterial().SetDiffuseColor(collision_color);
		collision_node->GetComponentPtr<MaterialComponent>()->GetMaterial().SetAmbientColor(collision_color);
		collision_node->GetComponentPtr<RenderingComponent>()->SetPolygonMode(PolygonMode::Wireframe);
		collision_ptr_ = collision_node.get();
		AddChild(std::move(collision_node));

		// Create cloth mesh
		cloth_mesh_ = std::make_shared<VertexObject>();
		auto mesh_node = make_unique<SceneNode>();
		mesh_node->CreateComponent<ShadingComponent>(std::make_shared<CheckerShader>());
		glm::vec3 mesh_color(.67f, .84f, 0.9f);
		mesh_node->CreateComponent<MaterialComponent>(
			std::make_shared<Material>(Material::GetDefault()));
		mesh_node->GetComponentPtr<MaterialComponent>()->GetMaterial().SetDiffuseColor(mesh_color);
		mesh_node->GetComponentPtr<MaterialComponent>()->GetMaterial().SetAmbientColor(mesh_color);

		mesh_node->CreateComponent<TextureComponent>(std::make_shared<Texture>(diffuse_maps_[0],1.f));
		mesh_node->GetComponentPtr<TextureComponent>()->GetTexture().SetNormalMap(normal_maps_[0]);

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

		auto tex_coords = make_unique<TexCoordArray>();
		for (int col = 0; col < cloth_size_; col++) {
			for (int row = 0; row < cloth_size_; row++) {
				float u = float(col) / (cloth_size_-1.0f);
				float v = float(row) / (cloth_size_-1.0f);
				//std::cout << u << " " << v << std::endl;
				tex_coords->push_back(glm::vec2( u, v ));
			}
		}
		cloth_mesh_->UpdateTexCoord(std::move(tex_coords));

		


		DrawClothPositions();
		FindIncidentTriangles();
		CreateNormalLines();
		CreateTangentLines();
		UpdateClothNormals();
		UpdateClothTangents();
		

		auto& rc = mesh_node->CreateComponent<RenderingComponent>(cloth_mesh_);
		rc.SetDrawMode(DrawMode::Triangles);
		cloth_mesh_node_ = mesh_node.get();

		AddChild(std::move(mesh_node));

		CreateFrame();
	}

	void ClothNode::Update(double delta_time) {
		if (!dragging_) {
			auto ray_data = raycaster_->GetCameraPosCurrentRay();
			int current_vertex_hit_ = CheckVertexCollision(ray_data);
			if (current_vertex_hit_ != -1) {
				collision_ptr_->SetActive(true);
				collision_ptr_->GetTransform().SetPosition(state_.positions[current_vertex_hit_]);
			}
			else {
				collision_ptr_->SetActive(false);
			}
		}
		else {
			if (current_vertex_hit_ != -1) {
				//::cout << "Current vertex" << current_vertex_hit_ << std::endl;
				collision_ptr_->GetTransform().SetPosition(state_.positions[current_vertex_hit_]);
				DragCloth(InputManager::GetInstance().GetCursorPosition());
			}
		}
		


		static bool prev_released = true;
		if (InputManager::GetInstance().IsKeyPressed('R')) {
			if (prev_released) {
				ResetSystem();
			}
			prev_released = false;
		}
		else if (InputManager::GetInstance().IsKeyPressed('B')) {
			if (prev_released) {
				ToggleBall();
			}
			prev_released = false;
		}
		else if (InputManager::GetInstance().IsKeyPressed('N')) {
			if (prev_released) {
				if (wireframe_on_) { ToggleWireframe(); TogglePause(); }
				ToggleNormals();
				UpdateClothNormals();
				UpdateClothTangents();
				TogglePause();
				
				
			}
			prev_released = false;
		}
		else if (InputManager::GetInstance().IsKeyPressed('T')) {
			if (prev_released) {
				ToggleWireframePolygonMode();
			}
			prev_released = false;
		}
		else if (InputManager::GetInstance().IsLeftMousePressed()) {
			if (prev_released) {
				auto ray_data = raycaster_->GetCameraPosCurrentRay(); 
				//std::cout << "Ray: " << ray_data[1][0] << " " << ray_data[1][1] << " " << ray_data[1][2] << " " << std::endl;
				//std::cout << "Camera: " << ray_data[0][0] << " " << ray_data[0][1] << " " << ray_data[0][2] << " " << std::endl;

				//raycaster_->CastRay(ray_data[1]);
				int vertex_hit = CheckVertexCollision(ray_data);
				if (vertex_hit != -1) {
					current_vertex_hit_ = vertex_hit;
					dragging_ = true;
					start_click_pos_ = InputManager::GetInstance().GetCursorPosition();
					//glm::vec3 prev_pos = state_.positions[current_vertex_hit_];
					//state_.positions[current_vertex_hit_] = prev_pos + glm::vec3{ 1.f,.0f,.0f };
					//state_.velocities[current_vertex_hit_] += (state_.positions[current_vertex_hit_] - prev_pos);
				}
			}
			prev_released = false;
		}
		else {
			prev_released = true;
			dragging_ = false;
		}

		if (!pause_on_) {
			float dist = 8.5f;
			float z = dist * cos(.75f * time_) - dist;
			ball_ptr_->GetTransform().SetPosition(ball_start_pos_ + glm::vec3(0.f, 0.f, z));

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

				if (ball_collision_) {
					glm::vec3 ball_pos = ball_ptr_->GetTransform().GetPosition();
					float eps = .12f;
					for (int j = 0; j < state_.positions.size(); j++) {

						glm::vec3 diff = state_.positions[j] - ball_pos;
						float distance = glm::length(diff);
						// Checks collision
						if (distance < ball_radius_ + eps) {
							//std::cout << "Found collision" << std::endl;
							glm::vec3 direction = glm::normalize(diff);
							glm::vec3 new_pos = ball_pos + direction * (ball_radius_ + eps);
							glm::vec3 delta_pos = new_pos - state_.positions[j];
							state_.positions[j] = new_pos;
							state_.velocities[j] += delta_pos / dt;

						}
					}
				}
				if (pinned_ < 3) {
					float eps = .05f;
					for (int j = 0; j < state_.positions.size(); j++) {
						float height = state_.positions[j].y;
						// Checks collision
						if (height < ground_height_ + eps) {
							//std::cout << "Found collision" << std::endl;
							glm::vec3 direction = glm::vec3(0.f, 1.f, 0.f);
							glm::vec3 new_pos = glm::vec3(state_.positions[j].x, ground_height_ + eps, state_.positions[j].z);
							glm::vec3 delta_pos = state_.positions[j]-new_pos;
							state_.positions[j] = new_pos;
							state_.velocities[j] = delta_pos / dt;

						}
					}
				}
				time_ += dt;
			}
			if (wireframe_on_) {
				DrawWireframe();
			}
			DrawClothPositions();
			UpdateClothNormals();
			UpdateClothTangents();
		}
		
	}

	void ClothNode::DragCloth(glm::dvec2 pos) {
		glm::vec3 distance = glm::vec3{ pos.x- start_click_pos_.x, start_click_pos_.y - pos.y   , 0.f};
		float delta = 40.0f;

		if (distance.x != 0.f || distance.y != 0.f) {
			glm::vec3 prev_pos = state_.positions[current_vertex_hit_];
			state_.positions[current_vertex_hit_] = prev_pos + distance / delta;
			state_.velocities[current_vertex_hit_] += (state_.positions[current_vertex_hit_] - prev_pos);
			start_click_pos_ = glm::dvec2(pos.x, pos.y);
		}


	}
	int ClothNode::CheckVertexCollision(std::vector<glm::vec3> data) {
		int final_vert = -1;
		bool collision_found = false;
		std::vector<float> hit_distances;
		std::vector<glm::vec3> centers_hit;
		std::vector<int> vert_indices;
		int vert_index = 0;
		for (glm::vec3 center : state_.positions) {
			float dist = CheckSphereCollision(data[1], data[0], center, cloth_width_/cloth_size_);
			if (dist >= 0) {
				collision_found = true;
				hit_distances.push_back(dist);
				centers_hit.push_back(center);
				vert_indices.push_back(vert_index);
			}
			vert_index += 1;
		}
		if (collision_found) {
			//std::cout << "Collision found" << std::endl;
			float nearest_dist = hit_distances[0];
			glm::vec3 nearest_center = centers_hit[0];
			int nearest_vert = vert_indices[0];
			for (int i = 1; i < hit_distances.size(); i++) {
				if (hit_distances[i] < nearest_dist) {
					nearest_dist = hit_distances[i];
					nearest_center = centers_hit[i];
					nearest_vert = vert_indices[i];
				}
			}
			final_vert = nearest_vert;
		}
		//std::cout << "Vert found" << final_vert << std::endl;
		return final_vert;


	}

	float ClothNode::CheckSphereCollision(glm::vec3 ray, glm::vec3 camera_pos, glm::vec3 center, float radius) {
		glm::vec3 oc = camera_pos - center;
		float a = glm::dot(ray, ray);
		float b = 2.0f * glm::dot(oc, ray);
		float c = glm::dot(oc, oc) - (radius * radius);
		float discriminant = b * b - 4 * a * c;
		if (discriminant < 0) {
			return -1.0f;
		}
		else {
			return (-b - sqrt(discriminant)) / 2.0f * a;
		}

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

		ball_ptr_->GetTransform().SetPosition(ball_start_pos_);
	}

	void ClothNode::DrawWireframe() {
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

		auto normals = cloth_mesh_->GetNormals();
		float normal_size = .5f;

		if (normals_on_) {
			for (int normal_index = 0; normal_index < normals_ptrs_.size(); normal_index++) {
				
				auto positions = make_unique<PositionArray>();
				positions->push_back(state_.positions[normal_index]);
				positions->push_back((state_.positions[normal_index] + normals[normal_index] * normal_size));
				normals_ptrs_[normal_index]->GetComponentPtr<RenderingComponent>()->GetVertexObjectPtr()->UpdatePositions(std::move(positions));
				

			}
		}
		
	}

	void ClothNode::UpdateClothTangents() {
		auto indices2 = cloth_mesh_->GetIndices();
		auto positions = cloth_mesh_->GetPositions();
		auto uvs = cloth_mesh_->GetTexCoords();

		auto tangents = make_unique<TangentArray>();

		for (int i = 0; i < positions.size(); i++) {
			tangents->push_back(glm::vec3(0.f));
		}
		for (int i = 0; i < indices2.size() - 2; i+=3) {
			int pos0 = indices2[i];
			int pos1 = indices2[i + 1];
			int pos2 = indices2[i + 2];

			glm::vec3 v0 = positions[pos0];
			glm::vec3 v1 = positions[pos1];
			glm::vec3 v2 = positions[pos2];

			glm::vec3 edge1 = v1 - v0;
			glm::vec3 edge2 = v2 - v0;

			float deltaU1 = uvs[pos1].x - uvs[pos0].x;
			float deltaV1 = uvs[pos1].y - uvs[pos0].y;
			float deltaU2 = uvs[pos2].x - uvs[pos0].x;
			float deltaV2 = uvs[pos2].y - uvs[pos0].y;

			float f = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);

			glm::vec3 tangent, bitangent;

			tangent.x = f * (deltaV2 * edge1.x - deltaV1 * edge2.x);
			tangent.y = f * (deltaV2 * edge1.y - deltaV1 * edge2.y);
			tangent.z = f * (deltaV2 * edge1.z - deltaV1 * edge2.z);

			tangents->at(pos0) += tangent;
			tangents->at(pos1) += tangent;
			tangents->at(pos2) += tangent;

		}

		for (int i = 0; i < positions.size(); i++) {
			tangents->at(i) = glm::normalize(tangents->at(i));
		}


		cloth_mesh_->UpdateTangents(std::move(tangents));

		auto tangents2 = cloth_mesh_->GetTangents();
		float normal_size = .5f;

		if (normals_on_) {
			for (int normal_index = 0; normal_index < tangents_ptrs_.size(); normal_index++) {
				auto positions = make_unique<PositionArray>();
				positions->push_back(state_.positions[normal_index]);
				positions->push_back((state_.positions[normal_index] + tangents2[normal_index] * normal_size));
				tangents_ptrs_[normal_index]->GetComponentPtr<RenderingComponent>()->GetVertexObjectPtr()->UpdatePositions(std::move(positions));


			}
		}

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

	void ClothNode::ToggleBall() {
		ball_collision_ = !ball_collision_;
		ball_ptr_->SetActive(ball_collision_);
	}

	void ClothNode::CreateNormalLines() {
		for (int i = 0; i < state_.positions.size(); i++) {
			auto line = std::make_shared<VertexObject>();
			auto line_shader = std::make_shared<SimpleShader>();
			auto indices = IndexArray({ 0,1 });
			line->UpdateIndices(make_unique<IndexArray>(indices));
			auto positions = PositionArray({ glm::vec3(0.f) , glm::vec3(0.f) });
			line->UpdatePositions(make_unique<PositionArray>(positions));

			auto line_node = make_unique<SceneNode>();
			line_node->CreateComponent<ShadingComponent>(line_shader);
			auto& rc_line = line_node->CreateComponent<RenderingComponent>(line);
			rc_line.SetDrawMode(DrawMode::Lines);
			auto color = glm::vec3(0.f, 0.f, 1.f);
			auto material = std::make_shared<Material>(color, color, color, 0.0f);
			line_node->CreateComponent<MaterialComponent>(material);
			normals_ptrs_.push_back(line_node.get());
	
			AddChild(std::move(line_node));
			normals_ptrs_.back()->SetActive(false);
		}
		
	}

	void ClothNode::CreateTangentLines() {
		for (int i = 0; i < state_.positions.size(); i++) {
			auto line = std::make_shared<VertexObject>();
			auto line_shader = std::make_shared<SimpleShader>();
			auto indices = IndexArray({ 0,1 });
			line->UpdateIndices(make_unique<IndexArray>(indices));
			auto positions = PositionArray({ glm::vec3(0.f) , glm::vec3(0.f) });
			line->UpdatePositions(make_unique<PositionArray>(positions));

			auto line_node = make_unique<SceneNode>();
			line_node->CreateComponent<ShadingComponent>(line_shader);
			auto& rc_line = line_node->CreateComponent<RenderingComponent>(line);
			rc_line.SetDrawMode(DrawMode::Lines);
			auto color = glm::vec3(1.f, 0.f, 0.f);
			auto material = std::make_shared<Material>(color, color, color, 0.0f);
			line_node->CreateComponent<MaterialComponent>(material);
			tangents_ptrs_.push_back(line_node.get());

			AddChild(std::move(line_node));
			tangents_ptrs_.back()->SetActive(false);
		}

	}
	void ClothNode::ToggleNormals() {
		normals_on_ = !normals_on_;
		for (auto ptr : normals_ptrs_) {
			ptr->SetActive(normals_on_);
		}
		for (auto ptr : tangents_ptrs_) {
			ptr->SetActive(normals_on_);
		}
	}

	void ClothNode::TogglePause() {
		pause_on_ = !pause_on_;

	}
	void ClothNode::FixCorners() {
		state_.positions[IndexOf(0, 0)] = glm::vec3(0.f, 0.f, 0.f);
		state_.positions[IndexOf(0, cloth_size_ - 1)] = glm::vec3(cloth_width_, 0.f, 0.f);
		system_.FixParticle(IndexOf(0, 0));
		system_.FixParticle(IndexOf(0, cloth_size_ - 1));

	}

	void ClothNode::CreateFrame() {
		glm::vec3 frame_color(1.f, 1.f, 1.f);
		std::shared_ptr<Material> material = std::make_shared<Material>(Material::GetDefault());
		material->SetDiffuseColor(frame_color);
		material->SetAmbientColor(frame_color);

		auto cylinder_node = make_unique<SceneNode>();
		cylinder_node->CreateComponent<MaterialComponent>(material);
		cylinder_node->CreateComponent<ShadingComponent>(shader_);
		cylinder_node->CreateComponent<RenderingComponent>(PrimitiveFactory::CreateCylinder(0.05f, 1, 25));
		float height = 12.0f;
		cylinder_node->GetTransform().SetScale(glm::vec3(4.0f, height, 4.0f));
		cylinder_node->GetTransform().SetPosition(glm::vec3(-2.f, -height, 0.f));
		AddChild(std::move(cylinder_node));


		auto cylinder_node1 = make_unique<SceneNode>();
		cylinder_node1->CreateComponent<MaterialComponent>(material);
		cylinder_node1->CreateComponent<ShadingComponent>(shader_);
		cylinder_node1->CreateComponent<RenderingComponent>(PrimitiveFactory::CreateCylinder(0.05f, 1, 25));
		cylinder_node1->GetTransform().SetScale(glm::vec3(4.0f, cloth_width_+4.0f, 4.0f));
		float pi = atan(1) * 4;
		cylinder_node1->GetTransform().SetPosition(glm::vec3(-2.f,0.f,0.f));
		cylinder_node1->GetTransform().SetRotation(glm::vec3(0.f,0.f,1.f), -pi/2);
		AddChild(std::move(cylinder_node1));

		auto cylinder_node2 = make_unique<SceneNode>();
		cylinder_node2->CreateComponent<MaterialComponent>(material);
		cylinder_node2->CreateComponent<ShadingComponent>(shader_);
		cylinder_node2->CreateComponent<RenderingComponent>(PrimitiveFactory::CreateCylinder(0.05f, 1, 25));
		cylinder_node2->GetTransform().SetScale(glm::vec3(4.0f, height, 4.0f));
		cylinder_node2->GetTransform().SetPosition(glm::vec3(cloth_width_+2.0f, -height, 0.f));
		AddChild(std::move(cylinder_node2));
	}
 }