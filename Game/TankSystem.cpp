#include "Game/TankSystem.h"
#include "Game/AssetManger.h"
#include "Game/GlobalSetting.h"
#include "Game/CTank.h"
#include "Game/CTankBarrel.h"
#include "Game/CTankTurret.h"
#include "Game/CTankWheel.h"

#include "ECS/Entity.h"
#include "ECS/CTransform.h"
#include "ECS/CMeshRenderer.h"

#include "Core/ServiceLocator.h"

#include "Physics/Layers.h"
#include "Physics/PhysicsSystem.h"

#include "Windows/InputManger.h"

namespace Game {
	inline static constexpr float sWheelRadius			{ 0.3f };
	inline static constexpr float sWheelWidth			{ 0.1f };
	inline static constexpr float sHalfVehicleLength	{ 3.2f };	// Along Axis Z
	inline static constexpr float sHalfVehicleWidth		{ 1.7f };	// Along Axis X
	inline static constexpr float sHalfVehicleHeight	{ 0.5f };	// Along Axis Y
	inline static constexpr float sSuspensionMinLength	{ 0.3f };
	inline static constexpr float sSuspensionMaxLength	{ 0.5f };
	inline static constexpr float sSuspensionFrequency	{ 1.0f };

	inline static constexpr float sMinVelocityPivotTurn { 1.0f };	// 最小速度枢轴转动

	inline static Math::Vector3 sStartPosition { 0.0f, 1620.0f, 0.0f };

	inline static JPH::Vec3 sWheelPosition[] = {
		JPH::Vec3{ 0.0f, -0.0f, 2.95f  },
		JPH::Vec3{ 0.0f, -0.3f, 2.1f   },
		JPH::Vec3{ 0.0f, -0.3f, 1.4f   },
		JPH::Vec3{ 0.0f, -0.3f, 0.7f   },
		JPH::Vec3{ 0.0f, -0.3f, 0.0f   },
		JPH::Vec3{ 0.0f, -0.3f, -0.7f  },
		JPH::Vec3{ 0.0f, -0.3f, -1.4f  },
		JPH::Vec3{ 0.0f, -0.3f, -2.1f  },
		JPH::Vec3{ 0.0f, -0.0f, -2.75f },
	};

	void TankSystem::Create() {
		// create filter to prevent body, turret and barrel from colliding
		JPH::GroupFilter* filter = new JPH::GroupFilterTable;
		JPH::PhysicsSystem* physicsSystem = CORESERVICE(Physics::PhysicsSystem).GetPhysicsSystem();
		JPH::BodyInterface& bodyInterface = CORESERVICE(Physics::PhysicsSystem).GetBodyInterface();
		CORESERVICE(Game::GlobalSetting).playerPosition = sStartPosition;

		// create tank entity
		auto tankEntity = ECS::Entity::Create<ECS::Transform, ECS::MeshRenderer, Game::CTank>();
		auto& tankTransform = tankEntity.GetComponent<ECS::Transform>();
		auto& tankMeshRenderer = tankEntity.GetComponent<ECS::MeshRenderer>();
		auto& tankComponent = tankEntity.GetComponent<Game::CTank>();

		tankMeshRenderer.mesh = CORESERVICE(AssetManger).GetMesh("Cube");
		auto& boundingBox = tankMeshRenderer.mesh->GetBoundingBox();
		tankTransform.worldPosition = sStartPosition;
		tankTransform.worldScaling = Math::Vector3{ 
			sHalfVehicleWidth  / boundingBox.Extents.x,
			sHalfVehicleHeight / boundingBox.Extents.y,
			sHalfVehicleLength / boundingBox.Extents.z
		};

		// create physical tank body
		JPH::RefConst<Shape> tankBodyShape = JPH::OffsetCenterOfMassShapeSettings(JPH::Vec3(0, -sHalfVehicleHeight, 0), new JPH::BoxShape(Vec3(sHalfVehicleWidth, sHalfVehicleHeight, sHalfVehicleLength))).Create().Get();
		JPH::BodyCreationSettings tankBodySettings(tankBodyShape, tankTransform.GetPhysicalSpaceWorldPosition(), tankTransform.GetPhysicalSpaceWorldRotation(), JPH::EMotionType::Dynamic, Layers::MOVING);
		tankBodySettings.mCollisionGroup.SetGroupFilter(filter);
		tankBodySettings.mCollisionGroup.SetGroupID(0);
		tankBodySettings.mCollisionGroup.SetSubGroupID(0);
		tankBodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		tankBodySettings.mMassPropertiesOverride.mMass = tankComponent.mass;
		JPH::Body& tankBody = *bodyInterface.CreateBody(tankBodySettings);
		tankComponent.tankBodyID = tankBody.GetID();
		bodyInterface.AddBody(tankBody.GetID(), JPH::EActivation::Activate);

		// create physical vehicle constraint
		JPH::VehicleConstraintSettings vehicleConstraintSettings;
		vehicleConstraintSettings.mDrawConstraintSize = 0.1f;
		vehicleConstraintSettings.mMaxPitchRollAngle = JPH::DegreesToRadians(60.0f);

		// 履带车辆控制器设置
		JPH::TrackedVehicleControllerSettings* controllerSettings = new JPH::TrackedVehicleControllerSettings;
		vehicleConstraintSettings.mController = controllerSettings;

		// 两侧履带
		for (uint32_t sideIndex = 0; sideIndex < 2; sideIndex++) {
			// 履带设置
			JPH::VehicleTrackSettings& trackSettings = controllerSettings->mTracks[sideIndex];
			// last wheel is driven wheel
			trackSettings.mDrivenWheel = (uint)(JPH::size(sWheelPosition) - 1);

			for (int32_t wheelIndex = 0; wheelIndex < JPH::size(sWheelPosition); wheelIndex++) {
				// create tank wheel entity
				auto tankWheelEntity = ECS::Entity::Create<ECS::Transform, ECS::MeshRenderer, Game::CTankWheel>();
				auto& tankWheelTransform = tankWheelEntity.GetComponent<ECS::Transform>();
				auto& tankWheelMeshRenderer = tankWheelEntity.GetComponent<ECS::MeshRenderer>();
				auto& tankWheelComponent = tankWheelEntity.GetComponent<Game::CTankWheel>();

				tankWheelMeshRenderer.mesh = CORESERVICE(AssetManger).GetMesh("Cylinder");
				auto& boundingBox = tankWheelMeshRenderer.mesh->GetBoundingBox();
				tankWheelTransform.worldPosition = Math::Vector3{ 0.0f, 1650.0f, 0.0f };
				tankWheelTransform.worldScaling = Math::Vector3{ 
					sWheelRadius       / boundingBox.Extents.x, 
					0.5f * sWheelWidth / boundingBox.Extents.y, 
					sWheelRadius       / boundingBox.Extents.z 
				};

				// attach tank entity id & wheel index
				tankWheelComponent.parentTankEntity = tankEntity.GetID();
				tankWheelComponent.tankWheelIndex = (uint32_t)vehicleConstraintSettings.mWheels.size();

				// create physical wheel
				JPH::WheelSettingsTV* wheelSettings = new JPH::WheelSettingsTV;
				wheelSettings->mPosition = sWheelPosition[wheelIndex];
				wheelSettings->mPosition.SetX(sideIndex == 0 ? sHalfVehicleWidth : -sHalfVehicleWidth);
				wheelSettings->mRadius = sWheelRadius;
				wheelSettings->mWidth = sWheelWidth;
				wheelSettings->mSuspensionMinLength = sSuspensionMinLength;
				wheelSettings->mSuspensionMaxLength = wheelIndex == 0 || wheelIndex == JPH::size(sWheelPosition) - 1 ? sSuspensionMinLength : sSuspensionMaxLength;
				wheelSettings->mSuspensionSpring.mFrequency = sSuspensionFrequency;

				// add the wheel to the vehicle
				trackSettings.mWheels.push_back((uint32_t)vehicleConstraintSettings.mWheels.size());
				vehicleConstraintSettings.mWheels.push_back(wheelSettings);
			}
		}

		JPH::Ref<JPH::VehicleConstraint> vehicleConstraint = new JPH::VehicleConstraint(tankBody, vehicleConstraintSettings);
		vehicleConstraint->SetVehicleCollisionTester(new JPH::VehicleCollisionTesterRay(Layers::MOVING));
		physicsSystem->AddConstraint(vehicleConstraint);
		physicsSystem->AddStepListener(vehicleConstraint);
		tankComponent.vehicleConstraint = vehicleConstraint;
	}

	void TankSystem::Destory() {
	}

	void TankSystem::PrePhysicsUpdate() {
		JPH::PhysicsSystem* physicsSystem = CORESERVICE(Physics::PhysicsSystem).GetPhysicsSystem();
		JPH::BodyInterface& bodyInterface = CORESERVICE(Physics::PhysicsSystem).GetBodyInterface();

		ECS::Entity::Foreach([&](ECS::Entity::ID& entityID, ECS::Transform& transform, Game::CTank& tank) {
			// Determine acceleration and brake
			float& prevForward = tank.prevForward;
			float& forward = tank.forward;
			float& leftRatio = tank.leftRatio;
			float& rightRatio = tank.rightRatio;
			float& brake = tank.brake;

			if (CORESERVICE(Windows::InputManger).IsKeyPressed(Windows::EKey::KEY_W)) {
				forward = -1.0f;
			}
			else if (CORESERVICE(Windows::InputManger).IsKeyPressed(Windows::EKey::KEY_S)) {
				forward = 1.0f;
			}
			else if (CORESERVICE(Windows::InputManger).IsKeyPressed(Windows::EKey::KEY_SPACE)) {
				brake = 1.0f;
			}

			float velocity = (bodyInterface.GetRotation(tank.tankBodyID).Conjugated() * bodyInterface.GetLinearVelocity(tank.tankBodyID)).GetZ();
			if (CORESERVICE(Windows::InputManger).IsKeyPressed(Windows::EKey::KEY_A)) {
				if (brake == 0.0f && forward == 0.0f && std::abs(velocity) < sMinVelocityPivotTurn) {
					// Pivot turn
					leftRatio = -1.0f;
					forward = 1.0f;
				}
				else {
					leftRatio = 0.6f;
				}
			}
			else if (CORESERVICE(Windows::InputManger).IsKeyPressed(Windows::EKey::KEY_D)) {
				if (brake == 0.0f && forward == 0.0f && abs(velocity) < sMinVelocityPivotTurn)
				{
					// Pivot turn
					rightRatio = -1.0f;
					forward = 1.0f;
				}
				else {
					rightRatio = 0.6f;
				}
			}


			// Check if we're reversing direction
			if (prevForward * forward < 0.0f) {
				// Get vehicle velocity in local space to the body of the vehicle
				if ((forward > 0.0f && velocity < -0.1f) || (forward < 0.0f && velocity > 0.1f)) {
					// Brake while we've not stopped yet
					forward = 0.0f;
					brake = 1.0f;
				}
				else {
					// When we've come to a stop, accept the new direction
					prevForward = forward;
				}
			}

			// Pass the input on to the constraint
			static_cast<TrackedVehicleController*>(tank.vehicleConstraint->GetController())->SetDriverInput(forward, leftRatio, rightRatio, brake);
		});
	}

	void TankSystem::PostPhysicsUpdate() {
		JPH::PhysicsSystem* physicsSystem = CORESERVICE(Physics::PhysicsSystem).GetPhysicsSystem();
		JPH::BodyInterface& bodyInterface = CORESERVICE(Physics::PhysicsSystem).GetBodyInterface();

		// tank body
		std::unordered_map<int32_t, JPH::Ref<JPH::VehicleConstraint>> vehicleConstraintMap;
		ECS::Entity::Foreach([&](ECS::Entity::ID& entityID, ECS::Transform& transform, Game::CTank& tank) {
			if (tank.vehicleConstraint != nullptr && !tank.tankBodyID.IsInvalid()) {
				vehicleConstraintMap[entityID] = tank.vehicleConstraint;
				JPH::Vec3 bodyTranslation = bodyInterface.GetPosition(tank.tankBodyID);
				JPH::Quat bodyQuaternion = bodyInterface.GetRotation(tank.tankBodyID);
				transform.SetPhysicalSpaceWorldPosition(bodyTranslation);
				transform.SetPhysicalSpaceWorldRotation(bodyQuaternion);
			}
		});

		// tank wheel
		ECS::Entity::Foreach([&](ECS::Entity::ID& entityID, ECS::Transform& transform, Game::CTankWheel& tankWheel) {
			auto it = vehicleConstraintMap.find(tankWheel.parentTankEntity);
			if (it != vehicleConstraintMap.end()) {
				JPH::Ref<JPH::VehicleConstraint> vehicleConstraint = it->second;
				const JPH::WheelSettings* wheelSettings = vehicleConstraint->GetWheels()[tankWheel.tankWheelIndex]->GetSettings();
				// the cylinder we draw is aligned with Y so we specify that as rotational axis
				JPH::RMat44 wheelTransform = vehicleConstraint->GetWheelWorldTransform(tankWheel.tankWheelIndex, JPH::Vec3::sAxisY(), JPH::Vec3::sAxisX());
				JPH::Vec3 wheelTranslation = wheelTransform.GetTranslation();
				JPH::Quat wheelQuaternion = wheelTransform.GetRotation().GetQuaternion();
				transform.SetPhysicalSpaceWorldPosition(wheelTranslation);
				transform.SetPhysicalSpaceWorldRotation(wheelQuaternion);
			}
		});

	}

}