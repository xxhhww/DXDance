#pragma once
#include "ECS/IComponent.h"
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"

namespace ECS {

	enum class BodyState {
		UnLoad,
		Loading,
		Loaded
	};

	class CollisionBody : public IComponent {
	public:

	public:
		JPH::BodyID bodyID;
		BodyState state{ BodyState::UnLoad };

	public:
		void SerializeJson(Tool::JsonWriter& writer) const override {

		}

		void DeserializeJson(const Tool::JsonReader& reader) override {

		}

		void OnInspector(UI::IWidgetContainer* container) override {
		}

	};

}