#pragma once
#include "ECS/IComponent.h"

#include "Renderer/Mesh.h"
#include "Renderer/Material.h"

namespace ECS {

	enum class RenderLayer {
		DEFERRED = 0,
		FORWARD = 1,
		COUNT = 2
	};

	class MeshRenderer : public ECS::IComponent {
	public:
		Renderer::Mesh* mesh{ nullptr };
		Renderer::Material* material{ nullptr };
		RenderLayer renderLayer{ RenderLayer::FORWARD };

	public:
		void SerializeJson(Tool::JsonWriter& writer) const override {

		}

		void DeserializeJson(const Tool::JsonReader& reader) override {

		}

		void OnInspector(UI::IWidgetContainer* container) override {

		}
	};

}