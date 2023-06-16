#pragma once
#include "ECS/IComponent.h"

#include "Math/Vector.h"
#include "Math/Color.h"

namespace ECS {

	using Lumen = float;
	using Nit = float;

	enum class LightType : uint32_t {
		DIRECTIONAL = 0,
		POINT = 1,
		SPOT  = 2,
		COUNT = 3
	};

	class Light : public ECS::IComponent {
	public:
		LightType mLightType{ LightType::DIRECTIONAL };
		Lumen mLuminousPower{ 0.0f };
		Nit mLuminance{ 0.0f };
		Math::Color mColor{ 1.0f, 1.0f, 1.0f };

	public:
		void SerializeJson(Tool::JsonWriter& writer) const override {

		}

		void DeserializeJson(const Tool::JsonReader& reader) override {

		}

		void OnInspector(UI::IWidgetContainer* container) override {

		}
	};

}