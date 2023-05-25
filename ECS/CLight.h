#pragma once
#include "ECS/IComponent.h"
#include "Math/Vector.h"

namespace ECS {

	enum class LightType {
		DIRECTIONAL = 0,
		POINT = 1,
		SPOT  = 2,
		COUNT = 3
	};

	class Light : public ECS::IComponent {
	public:
		LightType mLightType{ LightType::DIRECTIONAL };


	public:
		void SerializeJson(Tool::JsonWriter& writer) const override {

		}

		void DeserializeJson(const Tool::JsonReader& reader) override {

		}

		void OnInspector(UI::IWidgetContainer* container) override {

		}
	};

}