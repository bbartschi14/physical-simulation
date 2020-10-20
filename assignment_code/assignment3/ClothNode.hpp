#ifndef CLOTH_NODE_H_
#define CLOTH_NODE_H_

#include "gloo/SceneNode.hpp"
#include "ParticleState.hpp"
#include "PendulumSystem.hpp"
#include "ForwardEulerIntegrator.hpp"
#include "gloo/components/TextureComponent.hpp"
#include "gloo/components/RenderingComponent.hpp"
#include "Raycaster.hpp"
#include "gloo/shaders/ShaderProgram.hpp"
#include "gloo/VertexObject.hpp"

namespace GLOO {
    class ClothNode : public SceneNode {
    public:
        // Constructor
        ClothNode(float integration_step, IntegratorType integrator_type, Raycaster* raycaster);
        void Update(double delta_time) override;
        bool GetWireFrameState() {
            return wireframe_on_;

        }
        bool GetBallState() {
            return ball_collision_;
        }
        bool GetNormalsState() {
            return normals_on_;
        }
        bool GetWireframeState() {
            return polygon_is_wire_;
        }
        bool GetWindState() {
            return wind_on_;
        }
        void ToggleWind() {
            system_.ToggleWind();
            wind_on_ = !wind_on_;
        }
        void SetGravity(float amountx, float amounty, float amountz) {
            gravity_ = glm::vec3(amountx, amounty, amountz);
            system_.UpdateGravity(gravity_);
        }
        glm::vec3 GetGravity() {
            return gravity_;
        }
        float GetWindStrength() {
            return system_.GetWindStrength();
        }
        void SetWindStrength(float value) {
            system_.SetWindStrength(value);
        }
        void TogglePins() {
            if (pinned_ == 2) {
                ReleaseOneCorner();
                pinned_ = 1;
            } else if (pinned_ == 1) {
                ReleaseCorners();
                pinned_ = 0;
            }
            else {
                FixCorners();
                pinned_ = 2;
            }
        } 
        void ToggleClothNormal() {
            cloth_mesh_node_->GetComponentPtr<TextureComponent>()->GetTexture().ToggleNormal();
        }
        void ToggleClothDiffuse() {
            cloth_mesh_node_->GetComponentPtr<TextureComponent>()->GetTexture().ToggleDiffuse();
        }
        void ToggleClothNormalsVis() {
            cloth_mesh_node_->GetComponentPtr<TextureComponent>()->GetTexture().ToggleVisualizeNormals();
        }
        void NextTexture() {
            if (texture_index_ == diffuse_maps_.size() - 1) {
                texture_index_ = -1;
            }
            texture_index_ += 1;
            cloth_mesh_node_->GetComponentPtr<TextureComponent>()->GetTexture().SetDiffuseMap(diffuse_maps_[texture_index_]);
            cloth_mesh_node_->GetComponentPtr<TextureComponent>()->GetTexture().SetNormalMap(normal_maps_[texture_index_]);

        }
        void ToggleWireframePolygonMode() {
            if (polygon_is_wire_) {
                cloth_mesh_node_->GetComponentPtr<RenderingComponent>()->SetPolygonMode(PolygonMode::Fill);
            }
            else {
                cloth_mesh_node_->GetComponentPtr<RenderingComponent>()->SetPolygonMode(PolygonMode::Wireframe);
            }
            polygon_is_wire_ = !polygon_is_wire_;
        }
    private:
        void ResetSystem();
        int IndexOf(int row, int col);
        void CreateSpring(int i, int j, float rest_length, float stiffness, bool render);
        void DrawClothPositions();
        void CreateNormalLines();
        void CreateTangentLines();

        void DrawWireframe();
        void UpdateClothNormals();
        void UpdateClothTangents();

        void FixCorners();
        void ReleaseOneCorner() {
            system_.ReleaseParticle(IndexOf(0, 0));
        }
        void ReleaseCorners() {
            system_.ReleaseParticle(IndexOf(0, cloth_size_ - 1));
        }
        glm::vec3 FindTriNorm(glm::vec3 a, glm::vec3 b, glm::vec3 c);
        float FindTriArea(glm::vec3 a, glm::vec3 b, glm::vec3 c);
        void FindIncidentTriangles();
        void ToggleWireframe();
        void ToggleNormals();
        void ToggleBall();
        void TogglePause();
        void CreateFrame();
        int CheckVertexCollision(std::vector<glm::vec3> data);
        float CheckSphereCollision(glm::vec3 ray, glm::vec3 camera_pos, glm::vec3 center, float radius);
        void DragCloth(glm::dvec2 pos);
        glm::dvec2 start_click_pos_;
        ParticleState state_;
        // Set gravity and drag value for system calculations
        glm::vec3 gravity_{ 0.0f, -50.0f, 0.0f };
        float drag_ = .4f;
        PendulumSystem system_ = PendulumSystem(gravity_, drag_);
        std::unique_ptr<IntegratorBase<PendulumSystem, ParticleState>> integrator_;
        int cloth_size_;
        float cloth_width_;
        float ground_height_ = -12.0;
        std::vector<SceneNode*> sphere_ptrs_;
        std::vector<SceneNode*> line_ptrs_;
        std::vector<SceneNode*> tangents_ptrs_;

        std::vector<SceneNode*> normals_ptrs_;

        std::vector<int> line_indices_;

        std::vector<glm::vec3> initial_positions_;

        std::vector<std::vector<int>> incident_triangles_;

        std::shared_ptr<ShaderProgram> shader_;
        std::shared_ptr<VertexObject> sphere_mesh_;
        std::shared_ptr<VertexObject> cloth_mesh_;
        SceneNode* cloth_mesh_node_;
        SceneNode* ball_ptr_;
        SceneNode* collision_ptr_;
        glm::vec3 ball_start_pos_;
        double time_;
        float integration_step_;
        IntegratorType integrator_type_;
        float rollover_time_;
        float ball_radius_;
        
        int pinned_ = 2;
        bool wind_on_;
        bool pause_on_;
        bool wireframe_on_;
        bool normals_on_;
        bool ball_collision_;

        int texture_index_ = 0;
        std::vector<std::string> diffuse_maps_{"stone.png", "14.png", "argyle.png"};
        std::vector<std::string> normal_maps_{ "stone_normal.png", "14_NORM.png", "argyle_norm.png" };

        bool polygon_is_wire_ = false;
        bool dragging_ = false;
        int current_vertex_hit_;
        Raycaster* raycaster_;
    };
}  // namespace GLOO

#endif

