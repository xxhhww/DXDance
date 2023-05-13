#pragma once
#include "ECS/IComponent.h"

#include "Renderer/Mesh.h"
#include "Renderer/Material.h"

namespace Renderer {

	enum class RenderLayer {
		Deferred,
		Forward,
	};

	class MeshRenderer : public ECS::IComponent {
	public:
		Renderer::Mesh* mesh{ nullptr };
		Renderer::Material* material{ nullptr };
		RenderLayer renderLayer{ RenderLayer::Forward };

	public:
	};

}