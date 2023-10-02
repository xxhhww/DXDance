#include "Physics/PhysicsSystem.h"

namespace Physics {

	PhysicsSystem::PhysicsSystem(JPH::JobSystem* jobSystem) 
	: mJobSystem(jobSystem) {
		mTempAllocator = new TempAllocatorImpl(32 * 1024 * 1024);

		mPhysicsSystem = new JPH::PhysicsSystem();
		mPhysicsSystem->Init(smNumBodies, smNumBodyMutexes, smMaxBodyPairs, smMaxContactConstraints, mBroadPhaseLayerInterface, mObjectVsBroadPhaseLayerFilter, mObjectVsObjectLayerFilter);
		mPhysicsSystem->SetPhysicsSettings(mPhysicsSettings);

		// Restore gravity
		mPhysicsSystem->SetGravity(mGravity);
	}

	PhysicsSystem::~PhysicsSystem() {
		delete mPhysicsSystem;
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
	}

}