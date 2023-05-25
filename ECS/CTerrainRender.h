#pragma once
#include "ECS/IComponent.h"
#include "ECS/CMeshRenderer.h"

#include "Renderer/Texture.h"

namespace ECS {
	
	class TerrainRender : public ECS::IComponent {
	public:
		Renderer::Texture* mHeightMap{ nullptr };
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