#ifndef RAYCASTER_H_
#define RAYCASTER_H_

#include "gloo/SceneNode.hpp"
#include "gloo/VertexObject.hpp"
#include "gloo/shaders/ShaderProgram.hpp"
#include "gloo/cameras/ArcBallCameraNode.hpp"
#include "gloo/Scene.hpp"

#include <string>
#include <vector>

namespace GLOO {
    class Raycaster : public SceneNode {
    public:

        Raycaster(Scene* scene, ArcBallCameraNode* camera);
        glm::vec3 GetCurrentRay();
        void Update(double delta_time) override;
        void CastRay(glm::vec3 ray);
        glm::vec3 CalculateMouseRay();
        std::vector<glm::vec3> GetCameraPosCurrentRay() {
            auto new_ray = CalculateMouseRay();   
            return std::vector<glm::vec3> {camera_pos_, new_ray};
        }

    private:
        glm::vec2 GetNormalizedDeviceCoords(glm::vec2 mouse_position);
        glm::vec4 GetEyeCoords(glm::vec4 clip_coords);
        glm::vec3 GetWorldCoords(glm::vec4 eye_coords);
        SceneNode* FindSphereHit(glm::vec3 ray, std::vector<SceneNode*> nodes);
        float CheckCollision(glm::vec3 ray, SceneNode* sphere);
       
        Scene* scene_ptr_;
        ArcBallCameraNode* camera_ptr_;
        SceneNode* sphere_hit_;
        glm::mat4 projection_matrix_;
        glm::mat4 view_matrix_;
        glm::vec3 camera_pos_;
        glm::vec3 current_ray_;
       
    };
}  // namespace GLOO

#endif

