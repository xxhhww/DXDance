#include "Game/GameInitializer.h"
#include "Game/AssetManger.h"
#include "Game/SystemManger.h"
#include "Game/TankSystem.h"
#include "Game/CameraSystem.h"
#include "Game/StreamPhysicsSystem.h"

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
        Math::Vector2 testPositionXZ = Math::Vector2{ 0.0f, 0.0f };

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
            for (int32_t row = 0; row < nodeCountPerAxisInMaxLOD; row++) {
                for (int32_t col = 0; col < nodeCountPerAxisInMaxLOD; col++) {
                    auto entity = ECS::Entity::Create<ECS::Transform, ECS::HeightField, ECS::CollisionBody>();

                    auto& transform = entity.GetComponent<ECS::Transform>();
                    auto& heightField = entity.GetComponent<ECS::HeightField>();

                    // 计算当前地块的中心点的坐标和rectInWorldSpace

                    // 先以左上角为原点计算每一个地块的左上角的坐标
                    Math::Vector2 ltOriginPos = Math::Vector2{ col * nodeSizeInMaxLOD - worldMeterSize.x / 2.0f, -row * nodeSizeInMaxLOD + worldMeterSize.y / 2.0f };
                    Math::Vector2 centerPos = Math::Vector2{ ltOriginPos.x + nodeSizeInMaxLOD / 2.0f, ltOriginPos.y - nodeSizeInMaxLOD / 2.0f };

                    transform.worldPosition = Math::Vector3{ centerPos.x, 0.0f, centerPos.y };
                    heightField.tileIndex = Math::Vector2{ static_cast<float>(row), static_cast<float>(col) };
                    heightField.centerPos = centerPos;
                    heightField.extend = nodeSizeInMaxLOD / 2.0f;
                    heightField.lbOriginPos = Math::Vector2{ centerPos.x - nodeSizeInMaxLOD / 2.0f, centerPos.y - nodeSizeInMaxLOD / 2.0f  };
                    heightField.ltOriginPosInRightHanded = Math::Vector2{ heightField.lbOriginPos.x, -(heightField.lbOriginPos.y + nodeSizeInMaxLOD) };
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

            transform.worldPosition = Math::Vector3{ testPositionXZ.x, 550.0f, testPositionXZ.y };
            transform.worldScaling = Math::Vector3{ 0.02f,0.02f, 0.02f };
            meshRenderer.mesh = CORESERVICE(AssetManger).GetMesh("Cube");
        }
        {
            auto entity = ECS::Entity::Create<ECS::Transform, ECS::MeshRenderer, ECS::CollisionBody>();
            auto& transform = entity.GetComponent<ECS::Transform>();
            auto& meshRenderer = entity.GetComponent<ECS::MeshRenderer>();
            auto& collisionBody = entity.GetComponent<ECS::CollisionBody>();

            transform.worldPosition = Math::Vector3{ testPositionXZ.x, 650.0f, testPositionXZ.y };
            transform.worldScaling = Math::Vector3{ 0.02f, 0.02f, 0.02f };
            meshRenderer.mesh = CORESERVICE(AssetManger).GetMesh("Cube");
        }
    }

    void GameInitializer::InitializeSystem() {
        CORESERVICE(Game::SystemManger).Emplace(std::make_unique<TankSystem>());
        CORESERVICE(Game::SystemManger).Emplace(std::make_unique<CameraSystem>());
        CORESERVICE(Game::SystemManger).Emplace(std::make_unique<StreamPhysicsSystem>());

        CORESERVICE(Game::SystemManger).Create();
    }

}