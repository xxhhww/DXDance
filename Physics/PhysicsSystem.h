#pragma once
#include "Physics/Layers.h"

namespace Physics {

	class PhysicsSystem {
	public:
		PhysicsSystem(JPH::JobSystem* jobSystem);
		~PhysicsSystem();

		void StepPhysics();

		inline JPH::BodyInterface& GetBodyInterface() const { return mPhysicsSystem->GetBodyInterface(); }

	private:
		inline static uint32_t smNumBodies = 10240u;
		inline static uint32_t smNumBodyMutexes = 0u;						// Autodetect
		inline static uint32_t smMaxBodyPairs = 65536u;
		inline static uint32_t smMaxContactConstraints = 20480u;

		BPLayerInterfaceImpl mBroadPhaseLayerInterface;						// The broadphase layer interface that maps object layers to broadphase layers
		ObjectVsBroadPhaseLayerFilterImpl mObjectVsBroadPhaseLayerFilter;	// Class that filters object vs broadphase layers
		ObjectLayerPairFilterImpl mObjectVsObjectLayerFilter;

		float mUpdateFrequency = 60.0f;										// Physics update frequency
		int	mCollisionSteps = 1;											// How many collision detection steps per physics update
		TempAllocator* mTempAllocator{ nullptr };							// Allocator for temporary allocations

		JPH::JobSystem* mJobSystem{ nullptr };
		JPH::PhysicsSystem* mPhysicsSystem{ nullptr };
		JPH::BodyInterface* mBodyInterface{ nullptr };
		JPH::PhysicsSettings mPhysicsSettings;
		JPH::Vec3 mGravity{ 0.0f, -9.81f, 0.0f };

		// Timing
		uint					    mStepNumber{ 0u };						// Which step number we're accumulating
		std::chrono::microseconds	mTotalTime{ 0 };						// How many nano seconds we spent simulating
	};

}