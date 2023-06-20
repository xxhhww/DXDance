#pragma once
#include "ECS/IComponent.h"

#include "Math/Vector.h"
#include "Math/Color.h"

namespace ECS {

	enum class LightType : uint32_t {
		Sun = 0,	// 平行光源
		POINT = 1,	// 点光源
		SPOT  = 2,	// 聚光源
		COUNT = 3
	};

	class Light : public ECS::IComponent {
	public:
		LightType lightType{ LightType::Sun };

		Math::Color color{ 1.0f, 1.0f, 1.0f };

		Light() {

		}

	public:
		void SerializeJson(Tool::JsonWriter& writer) const override {

		}

		void DeserializeJson(const Tool::JsonReader& reader) override {

		}

		void OnInspector(UI::IWidgetContainer* container) override {

		}
	};

}