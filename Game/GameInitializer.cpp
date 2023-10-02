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
        * 设置摄像机
        */
        // RenderCamera
        {
            auto entity = ECS::Entity::Create<ECS::Transform, ECS::Camera>();

            auto& transform = entity.GetComponent<ECS::Transform>();
            transform.worldPosition = Math::Vector3{ 0.0f, 2000.0f, 0.0f };

            auto& camera = entity.GetComponent<ECS::Camera>();
            camera.cameraType = ECS::CameraType::RenderCamera;
            camera.mainCamera = true;
            camera.frustum.farZ = 5000.0f;
        }

        // EditorCamera
        {
            auto entity = ECS::Entity::Create<ECS::Transform, ECS::Camera>();

            auto& transform = entity.GetComponent<ECS::Transform>();
            transform.worldPosition = Math::Vector3{ 0.0f, 2000.0f, 0.0f };

            auto& camera = entity.GetComponent<ECS::Camera>();
            camera.cameraType = ECS::CameraType::EditorCamera;
            camera.frustum.farZ = 5000.0f;
        }

        /*
        * 设置Sky
        */
        // Sky
        {
            auto entity = ECS::Entity::Create<ECS::Transform, ECS::Sky>();

            auto& transform = entity.GetComponent<ECS::Transform>();
            transform.worldRotation = Math::Vector3{ DirectX::XM_PIDIV2, 0.0f, 0.0f };

            auto& sky = entity.GetComponent<ECS::Sky>();
        }

        /*
        * 为每一个地块(1024 * 1024)创建Entity
        */
        {
            Renderer::TerrainSystem* terrainSystem = CORESERVICE(Renderer::RenderEngine).mTerrainSystem.get();
            Math::Vector2 worldMeterSize = terrainSystem->worldMeterSize;
            uint32_t nodeCountPerAxisInMaxLOD = terrainSystem->nodeCountPerAxisInMaxLOD;
            float nodeSizeInMaxLOD = std::pow(2, terrainSystem->maxLOD) * terrainSystem->mostDetailNodeMeterSize;
            for (int32_t i = 0; i < nodeCountPerAxisInMaxLOD; i++) {
                for (int32_t j = 0; j < nodeCountPerAxisInMaxLOD; j++) {
                    auto entity = ECS::Entity::Create<ECS::Transform, ECS::HeightField, ECS::CollisionBody>();

                    auto& transform = entity.GetComponent<ECS::Transform>();
                    auto& heightField = entity.GetComponent<ECS::HeightField>();
                    auto& collisionBody = entity.GetComponent<ECS::CollisionBody>();

                    // 计算当前地块的中心点的坐标和rectInWorldSpace

                    // 先以左上角为原点计算每一个地块的左上角的坐标
                    Math::Vector2 ltOriginPos = Math::Vector2{ j * nodeSizeInMaxLOD - worldMeterSize.x / 2.0f, -i * nodeSizeInMaxLOD + worldMeterSize.y / 2.0f };
                    Math::Vector2 centerPos = Math::Vector2{ ltOriginPos.x + nodeSizeInMaxLOD / 2.0f, ltOriginPos.y - nodeSizeInMaxLOD / 2.0f };

                    transform.worldPosition = Math::Vector3{ centerPos.x, 0.0f, centerPos.y };
                    heightField.tileIndex = Math::Vector2{ static_cast<float>(i), static_cast<float>(j) };
                    heightField.centerPos = centerPos;
                    heightField.extend = nodeSizeInMaxLOD / 2.0f;
                    heightField.lbOriginPos = Math::Vector2{ centerPos.x - nodeSizeInMaxLOD / 2.0f, centerPos.y - nodeSizeInMaxLOD / 2.0f  };
                }
            }
        }

        /*
        * 创建一个长方体
        */
        {
            auto entity = ECS::Entity::Create<ECS::Transform, ECS::MeshRenderer, ECS::CollisionBody>();
            auto& transform = entity.GetComponent<ECS::Transform>();
            auto& meshRenderer = entity.GetComponent<ECS::MeshRenderer>();
            auto& collisionBody = entity.GetComponent<ECS::CollisionBody>();

            transform.worldPosition = Math::Vector3{ 0.0f, 2000.0f, 0.0f };
            meshRenderer.mesh = CORESERVICE(AssetManger).GetMesh("Cube");
        }
    }

    void GameInitializer::InitializeSystem() {
        CORESERVICE(Game::SystemManger).Emplace(std::make_unique<CameraSystem>());
        CORESERVICE(Game::SystemManger).Emplace(std::make_unique<StreamPhysicsSystem>());
    }

}