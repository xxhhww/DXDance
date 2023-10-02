#pragma once
#include "ECS/IComponent.h"

#include "Math/Vector.h"

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/HeightFieldShape.h"

#include <string>

namespace ECS {

	class HeightField : public IComponent {
	public:
		Math::Vector2 tileIndex{ 0.0f, 0.0f };
		Math::Vector2 centerPos{ 0.0f, 0.0f };
		Math::Vector2 lbOriginPos{ 0.0f, 0.0f };	// 地形左下角的点
		float extend{ 1024.0f / 2.0f };
		float heightScale{ 4096.0f };

		JPH::RefConst<JPH::HeightFieldShape> heightFieldShape;

		std::string GetHeightMapBinFilename() const { 
			return "HeightMap_" + std::to_string((uint32_t)tileIndex.x) + "_" + std::to_string((uint32_t)tileIndex.y) + ".bin"; 
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