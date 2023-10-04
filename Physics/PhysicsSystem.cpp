#include "Physics/PhysicsSystem.h"

#include "ECS/Entity.h"
#include "ECS/CTransform.h"
#include "ECS/CMeshRenderer.h"
#include "ECS/CCollisionBody.h"

namespace Physics {

	PhysicsSystem::PhysicsSystem(JPH::JobSystem* jobSystem) 
	: mJobSystem(jobSystem) {
		// Create factory
		JPH::Factory::sInstance = new JPH::Factory;

		// Register physics types with the factory
		JPH::RegisterTypes();

		mTempAllocator = new TempAllocatorImpl(32 * 1024 * 1024);

		mPhysicsSystem = new JPH::PhysicsSystem();
		mPhysicsSystem->Init(smNumBodies, smNumBodyMutexes, smMaxBodyPairs, smMaxContactConstraints, mBroadPhaseLayerInterface, mObjectVsBroadPhaseLayerFilter, mObjectVsObjectLayerFilter);
		mPhysicsSystem->SetPhysicsSettings(mPhysicsSettings);

		// Restore gravity
		mPhysicsSystem->SetGravity(mGravity);

		mBodyInterface = &mPhysicsSystem->GetBodyInterface();
	}

	PhysicsSystem::~PhysicsSystem() {
		delete mPhysicsSystem;

		JPH::UnregisterTypes();

		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}

	void PhysicsSystem::StepPhysics() {
		float delta_time = 1.0f / mUpdateFrequency;

		// Remember start time
		std::chrono::high_resolution_clock::time_point clock_start = std::chrono::high_resolution_clock::now();

		// Step the world (with fixed frequency)
		mPhysicsSystem->Update(delta_time, mCollisionSteps, mTempAllocator, mJobSystem);
		JPH_ASSERT(static_cast<TempAllocatorImpl*>(mTempAllocator)->IsEmpty());

		// Accumulate time
		std::chrono::high_resolution_clock::time_point clock_end = std::chrono::high_resolution_clock::now();
		std::chrono::microseconds duration = std::chrono::duration_cast<std::chrono::microseconds>(clock_end - clock_start);
		mTotalTime += duration;
		mStepNumber++;

		// 将物理空间中的数据更新到游戏空间中
		ECS::Entity::Foreach([&](ECS::Entity::ID& id, ECS::Transform& transform, ECS::CollisionBody& collisionBody, ECS::MeshRenderer& meshRenderer) {
			// 碰撞体已加载，并且状态合法
			if (collisionBody.state == ECS::BodyState::Loaded && !collisionBody.bodyID.IsInvalid()) {
				auto rVec3 = mBodyInterface->GetCenterOfMassPosition(collisionBody.bodyID);
				auto rQuat = mBodyInterface->GetRotation(collisionBody.bodyID);
				transform.SetPhysicalSpaceWorldPosition(rVec3);
				transform.SetPhysicalSpaceWorldRotation(rQuat);
			}
		});
	}

}