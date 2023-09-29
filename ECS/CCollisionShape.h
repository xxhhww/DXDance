#pragma once
#include "ECS/IComponent.h"

namespace ECS {

	class CollisionShape : public IComponent {
	public:


	public:
		void SerializeJson(Tool::JsonWriter& writer) const override {

		}

		void DeserializeJson(const Tool::JsonReader& reader) override {

		}

		void OnInspector(UI::IWidgetContainer* container) override {
		}

	};

}