#include "Game/GameInitializer.h"
#include "Game/SystemManger.h"
#include "Game/StreamPhysicsSystem.h"
#include "Game/CameraSystem.h"
#include "Game/AssetManger.h"

#include "ECS/Entity.h"
#include "ECS/CTransform.h"
#include "ECS/CCamera.h"
#include "ECS/CSky.h"
#include "ECS/CCollisionBody.h"
#include "ECS/CHeightField.h"
#include "ECS/CMeshRenderer.h"

#include "Core/ServiceLocator.h"

#include "Renderer/RenderEngine.h"
#include "Renderer/TerrainSystem.h"

namespace Game {

	void GameInitializer::DoInitialization() {
        InitializeEntity();
        InitializeSystem();
	}

    void GameInitializer::InitializeEntity() {
        /*
        * ���������
        */
        // RenderCamera
        {
            auto entity = ECS::Entity::Create<ECS::Transform, ECS::Camera>();

            auto& transform = entity.GetComponent<ECS::Transform>();
            transform.worldPosition = Math::Vector3{ 0.0f, 1500.0f, 1024.0f };

            auto& camera = entity.GetComponent<ECS::Camera>();
            camera.cameraType = ECS::CameraType::RenderCamera;
            camera.mainCamera = true;
            camera.frustum.farZ = 5000.0f;
        }

        // EditorCamera
        {
            auto entity = ECS::Entity::Create<ECS::Transform, ECS::Camera>();

            auto& transform = entity.GetComponent<ECS::Transform>();
            transform.worldPosition = Math::Vector3{ 0.0f, 1500.0f, 1024.0f };

            auto& camera = entity.GetComponent<ECS::Camera>();
            camera.translationSpeed *= 5.0f;
            camera.cameraType = ECS::CameraType::EditorCamera;
            camera.frustum.farZ = 5000.0f;
        }

        /*
        * ����Sky
        */
        // Sky
        {
            auto entity = ECS::Entity::Create<ECS::Transform, ECS::Sky>();

            auto& transform = entity.GetComponent<ECS::Transform>();
            transform.worldRotation = Math::Vector3{ DirectX::XM_PIDIV2, 0.0f, 0.0f };

            auto& sky = entity.GetComponent<ECS::Sky>();
        }

        /*
        * Ϊÿһ���ؿ�(1024 * 1024)����Entity
        */
        {
            Renderer::TerrainSystem* terrainSystem = CORESERVICE(Renderer::RenderEngine).mTerrainSystem.get();
            Math::Vector2 worldMeterSize = terrainSystem->worldMeterSize;
            uint32_t nodeCountPerAxisInMaxLOD = terrainSystem->nodeCountPerAxisInMaxLOD;
            float nodeSizeInMaxLOD = std::pow(2, terrainSystem->maxLOD) * terrainSystem->mostDetailNodeMeterSize;
            for (int32_t row = 0; row < nodeCountPerAxisInMaxLOD; row++) {
                for (int32_t col = 0; col < nodeCountPerAxisInMaxLOD; col++) {
                    auto entity = ECS::Entity::Create<ECS::Transform, ECS::HeightField, ECS::CollisionBody>();

                    auto& transform = entity.GetComponent<ECS::Transform>();
                    auto& heightField = entity.GetComponent<ECS::HeightField>();

                    // ���㵱ǰ�ؿ�����ĵ�������rectInWorldSpace

                    // �������Ͻ�Ϊԭ�����ÿһ���ؿ�����Ͻǵ�����
                    Math::Vector2 ltOriginPos = Math::Vector2{ col * nodeSizeInMaxLOD - worldMeterSize.x / 2.0f, -row * nodeSizeInMaxLOD + worldMeterSize.y / 2.0f };
                    Math::Vector2 centerPos = Math::Vector2{ ltOriginPos.x + nodeSizeInMaxLOD / 2.0f, ltOriginPos.y - nodeSizeInMaxLOD / 2.0f };

                    transform.worldPosition = Math::Vector3{ centerPos.x, 0.0f, centerPos.y };
                    heightField.tileIndex = Math::Vector2{ static_cast<float>(row), static_cast<float>(col) };
                    heightField.centerPos = centerPos;
                    heightField.extend = nodeSizeInMaxLOD / 2.0f;
                    heightField.lbOriginPos = Math::Vector2{ centerPos.x - nodeSizeInMaxLOD / 2.0f, centerPos.y - nodeSizeInMaxLOD / 2.0f  };
                }
            }
        }

        /*
        * ����һ��������
        */
        {
            auto entity = ECS::Entity::Create<ECS::Transform, ECS::MeshRenderer, ECS::CollisionBody>();
            auto& transform = entity.GetComponent<ECS::Transform>();
            auto& meshRenderer = entity.GetComponent<ECS::MeshRenderer>();
            auto& collisionBody = entity.GetComponent<ECS::CollisionBody>();

            transform.worldPosition = Math::Vector3{ 0.0f, 2000.0f, 1024.0f };
            meshRenderer.mesh = CORESERVICE(AssetManger).GetMesh("Cube");
        }
        {
            auto entity = ECS::Entity::Create<ECS::Transform, ECS::MeshRenderer, ECS::CollisionBody>();
            auto& transform = entity.GetComponent<ECS::Transform>();
            auto& meshRenderer = entity.GetComponent<ECS::MeshRenderer>();
            auto& collisionBody = entity.GetComponent<ECS::CollisionBody>();

            transform.worldPosition = Math::Vector3{ 0.0f, 1850.0f, 1024.0f };
            meshRenderer.mesh = CORESERVICE(AssetManger).GetMesh("Cube");
        }
    }

    void GameInitializer::InitializeSystem() {
        CORESERVICE(Game::SystemManger).Emplace(std::make_unique<CameraSystem>());
        CORESERVICE(Game::SystemManger).Emplace(std::make_unique<StreamPhysicsSystem>());
    }

}