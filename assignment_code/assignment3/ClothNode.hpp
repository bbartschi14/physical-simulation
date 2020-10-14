#ifndef CLOTH_NODE_H_
#define CLOTH_NODE_H_

#include "gloo/SceneNode.hpp"
#include "ParticleState.hpp"
#include "PendulumSystem.hpp"
#include "ForwardEulerIntegrator.hpp"
#include "gloo/shaders/ShaderProgram.hpp"
#include "gloo/VertexObject.hpp"

namespace GLOO {
    class ClothNode : public SceneNode {
    public:
        // Constructor
        ClothNode(float integration_step, IntegratorType integrator_type);
        void Update(double delta_time) override;
    private:
        void ResetSystem();
        int IndexOf(int row, int col);
        void CreateSpring(int i, int j, float rest_length, float stiffness, bool render);
        void DrawClothPositions();
        void UpdateClothNormals();

        glm::vec3 FindTriNorm(glm::vec3 a, glm::vec3 b, glm::vec3 c);
        float FindTriArea(glm::vec3 a, glm::vec3 b, glm::vec3 c);
        void FindIncidentTriangles();
        void ToggleWireframe();
        ParticleState state_;
        // Set gravity and drag value for system calculations
        glm::vec3 gravity_{ 0.0f, -2.0f, 0.0f };
        float drag_ = .3f;
        PendulumSystem system_ = PendulumSystem(gravity_, drag_);
        std::unique_ptr<IntegratorBase<PendulumSystem, ParticleState>> integrator_;
        int cloth_size_;
        float cloth_width_;

        std::vector<SceneNode*> sphere_ptrs_;
        std::vector<SceneNode*> line_ptrs_;
        std::vector<int> line_indices_;

        std::vector<glm::vec3> initial_positions_;

        std::vector<std::vector<int>> incident_triangles_;

        std::shared_ptr<ShaderProgram> shader_;
        std::shared_ptr<VertexObject> sphere_mesh_;
        std::shared_ptr<VertexObject> cloth_mesh_;
        SceneNode* cloth_mesh_node_;
        double time_;
        float integration_step_;
        IntegratorType integrator_type_;
        float rollover_time_;

        bool wireframe_on_;
    };
}  // namespace GLOO

#endif

