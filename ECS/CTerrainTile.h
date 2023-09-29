#pragma once
#include "ECS/IComponent.h"
#include "Math/Vector.h"

namespace ECS {

	class TerrainTile : public IComponent {
	public:
		Math::Vector2 tileCoord{ 0.0f, 0.0f };
		Math::Vector4 terrainRect{ 0.0f, 0.0f, 1024.0f, 1024.0f };

	public:
		void SerializeJson(Tool::JsonWriter& writer) const override {

		}

		void DeserializeJson(const Tool::JsonReader& reader) override {

		}

		void OnInspector(UI::IWidgetContainer* container) override {
		}

	};

}