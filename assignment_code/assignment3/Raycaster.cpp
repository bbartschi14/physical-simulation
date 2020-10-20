#include "Raycaster.hpp"


#include "gloo/cameras/ArcBallCameraNode.hpp"
#include "gloo/InputManager.hpp"
#include "gloo/shaders/SimpleShader.hpp"
#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/debug/PrimitiveFactory.hpp"
#include "gloo/shaders/PhongShader.hpp"

namespace GLOO {
	Raycaster::Raycaster(Scene* scene, ArcBallCameraNode* camera) : SceneNode() {
		scene_ptr_ = scene;
		camera_ptr_ = camera;
		projection_matrix_ = scene_ptr_->GetActiveCameraPtr()->GetProjectionMatrix();
		view_matrix_ = scene_ptr_->GetActiveCameraPtr()->GetViewMatrix();
		current_ray_ = glm::vec3(0.0f);
		camera_pos_ = glm::vec3(0.0f);
		sphere_hit_ = nullptr;
	}

	glm::vec3 Raycaster::GetCurrentRay() {
		return current_ray_;
	}

	void Raycaster::Update(double delta_time) {
		/*static bool prev_released = true;
		if (InputManager::GetInstance().IsLeftMousePressed()) {
			if (prev_released) {
				current_ray_ = CalculateMouseRay();
				//CastRay(current_ray_);
			}
			prev_released = false;
		}
		else {
			prev_released = true;

		}*/
		
	}

	

	void Raycaster::CastRay(glm::vec3 ray) {
		//std::cout << "Casting Ray" << std::endl;
		auto line = std::make_shared<VertexObject>();
		auto line_shader = std::make_shared<SimpleShader>();
		auto indices = IndexArray();
		indices.push_back(0);
		indices.push_back(1);
		line->UpdateIndices(make_unique<IndexArray>(indices));
		auto positions = make_unique<PositionArray>();
		glm::vec3 pos = camera_pos_;
		//std::cout << pos[0] << " " << pos[1] << " " << pos[2] << " " << std::endl;
		float length = 20.0f;

		auto sphere_node = make_unique<SceneNode>();
		sphere_node->CreateComponent<ShadingComponent>(std::make_shared<PhongShader>());
		sphere_node->CreateComponent<RenderingComponent>(PrimitiveFactory::CreateSphere(0.005f, 25, 25));
		sphere_node->GetTransform().SetPosition(pos);
		AddChild(std::move(sphere_node));


		positions->push_back(pos);
		positions->push_back(pos +(ray*length));
		line->UpdatePositions(std::move(positions));

		auto line_node = make_unique<SceneNode>();
		line_node->CreateComponent<ShadingComponent>(line_shader);
		auto& rc_line = line_node->CreateComponent<RenderingComponent>(line);
		rc_line.SetDrawMode(DrawMode::Lines);
		auto color = glm::vec3(1.f, 0.f, 0.f);
		auto material = std::make_shared<Material>(color, color, color, 0.0f);
		line_node->CreateComponent<MaterialComponent>(material);
		AddChild(std::move(line_node));
	}

	SceneNode* Raycaster::FindSphereHit(glm::vec3 ray, std::vector<SceneNode*> nodes) {
		SceneNode* sphere_hit_ptr = nullptr;
		std::vector<float> sphere_hit_distances;
		std::vector<SceneNode*> sphere_hit_ptrs;
		bool hit_registered = false;
		for (auto sphere_ptr : nodes) {
			float sphere_dist = CheckCollision(ray, sphere_ptr);
			if (sphere_dist >= 0) {
				hit_registered = true;
				sphere_hit_distances.push_back(sphere_dist);
				sphere_hit_ptrs.push_back(sphere_ptr);
			}
		}
		if (hit_registered) {
			float nearest_dist = sphere_hit_distances[0];
			auto nearest_sphere = sphere_hit_ptrs[0];
			for (int i = 1; i < sphere_hit_distances.size(); i++) {
				if (sphere_hit_distances[i] < nearest_dist) {
					nearest_dist = sphere_hit_distances[i];
					nearest_sphere = sphere_hit_ptrs[i];
				}
			}
			sphere_hit_ptr = nearest_sphere;
		}
		
		return sphere_hit_ptr;

	}

	

	float Raycaster::CheckCollision(glm::vec3 ray, SceneNode* sphere) {
		float radius = .025f;
		glm::vec3 center = sphere->GetTransform().GetWorldPosition();
		//std::cout << "Center: " << center[0] << " " << center[1] << " " << center[2] << std::endl;
		//std::cout << "Ray: " << ray[0] << " " << ray[1] << " " << ray[2] << std::endl;
		//std::cout << "Origin: " << camera_pos_[0] << " " << camera_pos_[1] << " " << camera_pos_[2] << std::endl;

		glm::vec3 oc = camera_pos_ - center;
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

	glm::vec3 Raycaster::CalculateMouseRay() {
		view_matrix_ = scene_ptr_->GetActiveCameraPtr()->GetViewMatrix();
		glm::vec2 mouse_pos = InputManager::GetInstance().GetCursorPosition();
		glm::vec2 normalized_coords = GetNormalizedDeviceCoords(mouse_pos);
		glm::vec4 clip_coords = glm::vec4(normalized_coords[0], normalized_coords[1], -1.0f, 1.0f);
		glm::vec4 eye_coords = GetEyeCoords(clip_coords);
		//std::cout << "Coords: " << eye_coords[0] << " " << eye_coords[1] << " " << eye_coords[2] << " " << eye_coords[3] << std::endl;

		glm::vec3 world_ray = GetWorldCoords(eye_coords);
		glm::vec3 ray = glm::normalize(world_ray);
		//std::cout << "Ray: " << ray[0] << " " << ray[1] << " " << ray[2] << std::endl;
	
		return ray;
	}

	glm::vec2 Raycaster::GetNormalizedDeviceCoords(glm::vec2 mouse_position) {
		glm::vec2 window_size = InputManager::GetInstance().GetWindowSize();
		float x = (2.0f * mouse_position[0]) / window_size[0] - 1.0f;
		float y = (2.0f * mouse_position[1]) / window_size[1] - 1.0f;
		return glm::vec2(x, -y);
	}

	glm::vec4 Raycaster::GetEyeCoords(glm::vec4 clip_coords) {
		glm::mat4 inverted_projection = glm::inverse(projection_matrix_);
		glm::vec4 eye_coords = inverted_projection * clip_coords;
		return glm::vec4(eye_coords[0], eye_coords[1], eye_coords[2], eye_coords[3]);
	}

	glm::vec3 Raycaster::GetWorldCoords(glm::vec4 eye_coords) {
		glm::mat4 inverted_view = glm::inverse(view_matrix_);

		camera_pos_ = glm::vec3(inverted_view[3][0], inverted_view[3][1], inverted_view[3][2]);
		glm::vec4 divided_eye = glm::vec4(eye_coords[0] / eye_coords[3], eye_coords[1] / eye_coords[3], eye_coords[2] / eye_coords[3], 0);
		glm::vec4 world_ray = inverted_view * divided_eye;
		glm::vec3 mouse_ray = glm::vec3(world_ray[0], world_ray[1], world_ray[2]);
		return mouse_ray;
	}
	

}