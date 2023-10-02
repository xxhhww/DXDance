#pragma once
#include "ECS/IComponent.h"
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"

namespace ECS {

	class CollisionBody : public IComponent {
	public:
		enum class State {
			UnLoad,
			Loading,
			Loaded
		};

	public:
		JPH::BodyID bodyID;
		State state{ State::UnLoad };

	public:
		void SerializeJson(Tool::JsonWriter& writer) const override {

		}

		void DeserializeJson(const Tool::JsonReader& reader) override {

		}

		void OnInspector(UI::IWidgetContainer* container) override {
		}

	};

}