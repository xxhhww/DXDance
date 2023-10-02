#include "Game/StreamPhysicsSystem.h"
#include "Core/ServiceLocator.h"
#include "Jolt/Jolt.h"
#include "Jolt/Core/JobSystem.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/HeightFieldShape.h"

#include "ECS/Entity.h"
#include "ECS/CCamera.h"
#include "ECS/CTransform.h"
#include "ECS/CCollisionBody.h"
#include "ECS/CHeightField.h"

#include "Physics/PhysicsSystem.h"

#include "Tools/Assert.h"

namespace Game {

	JPH::Array<uint8> ReadData(const char* inFileName)
	{
		JPH::Array<uint8> data;
		std::ifstream input(inFileName, std::ios::binary);
		if (!input) {
			ASSERT_FORMAT(false, "Unable to open file: %s", inFileName);
		}
		input.seekg(0, std::ios_base::end);
		std::ifstream::pos_type length = input.tellg();
		input.seekg(0, std::ios_base::beg);
		data.resize(size_t(length));
		input.read((char*)&data[0], length);
		if (!input) {
			ASSERT_FORMAT(false, "Unable to read file: %s", inFileName);
		}
		return data;
	}

	void StreamPhysicsSystem::Create() {
	}

	void StreamPhysicsSystem::Destory() {
	}

	void StreamPhysicsSystem::Run() {

		// 获得当前渲染摄像机的位置
		Math::Vector3 cameraPos{ 0.0f, 0.0f, 0.0f };
		ECS::Entity::Foreach([&](ECS::Entity::ID& id, ECS::Transform& transform, ECS::Camera& camera) {
			if (camera.cameraType == ECS::CameraType::RenderCamera && camera.mainCamera) {
				cameraPos = transform.worldPosition;
			}
		});

		JPH::JobSystem* jobSystem = &CORESERVICE(JPH::JobSystem);
		Physics::PhysicsSystem* physicsSystem = &CORESERVICE(Physics::PhysicsSystem);
		JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();

		ECS::Entity::Foreach([&](ECS::Entity::ID& id, ECS::Transform& transform, ECS::HeightField& heightField, ECS::CollisionBody& collisionBody) {
			
			// 计算XZ平面上的距离
			float distance = Math::Vector2{ cameraPos.x - transform.worldPosition.x, cameraPos.z - transform.worldPosition.z }.Length();
			if (distance < 1024.0f && collisionBody.state == ECS::CollisionBody::State::UnLoad) {
				collisionBody.state = ECS::CollisionBody::State::Loading;
				jobSystem->CreateJob("", JPH::ColorArg::sGreen, [&]() {
					const int n = 1024;

					// Get height samples
					std::string binFile = "E:/MyProject/DXDance/Resources/Textures/Terrain/HeightMap/" + heightField.GetHeightMapBinFilename();
					JPH::Array<uint8> data = ReadData(binFile.c_str());
					if (data.size() != sizeof(float) * n * n) {
						ASSERT_FORMAT(false, "Invalid file size");
					}

					JPH::uint terrainSize = n;
					JPH::Array<float> terrainData;
					terrainData.resize(terrainSize * terrainSize);
					memcpy(terrainData.data(), data.data(), terrainSize * terrainSize * sizeof(float));

					// Determine scale and offset
					JPH::Vec3 terrainOffset = JPH::Vec3(heightField.lbOriginPos.x, 0.0f, heightField.lbOriginPos.y);
					JPH::Vec3 terrainScale  = JPH::Vec3(1.0f, heightField.heightScale, 1.0f);

					JPH::PhysicsMaterialList materials;
					JPH::Array<uint8>		 materialIndices;
					int	sBlockSizeShift = 2;

					// Bits per sample
					int	sBitsPerSample = 8;

					JPH::HeightFieldShapeSettings settings(terrainData.data(), terrainOffset, terrainScale, terrainSize, materialIndices.data(), materials);
					settings.mBlockSize = 1 << sBlockSizeShift;
					settings.mBitsPerSample = sBitsPerSample;
					heightField.heightFieldShape = static_cast<const JPH::HeightFieldShape*>(settings.Create().Get().GetPtr());
					JPH::Body& terrain = *bodyInterface.CreateBody(JPH::BodyCreationSettings(heightField.heightFieldShape, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING));
					bodyInterface.AddBody(terrain.GetID(), JPH::EActivation::DontActivate);

					collisionBody.bodyID = terrain.GetID();
					collisionBody.state = ECS::CollisionBody::State::Loaded;
				});
			}
		});
	}

}