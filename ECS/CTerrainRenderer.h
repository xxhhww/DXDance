#pragma once
#include "ECS/IComponent.h"
#include "ECS/CMeshRenderer.h"

#include "Renderer/Terrain.h"

namespace ECS {
	
	class TerrainRenderer : public ECS::IComponent {
	public:
		RenderLayer mRenderLayer{ RenderLayer::DEFERRED };

	public:
		void SerializeJson(Tool::JsonWriter& writer) const override {

		}

		void DeserializeJson(const Tool::JsonReader& reader) override {

		}

		void OnInspector(UI::IWidgetContainer* container) override {

		}
	};

}