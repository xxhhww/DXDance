#pragma once
#include "Renderer/RenderGraph.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/Mesh.h"

namespace Renderer {

	class RenderEngine;

	class SkyPass {
	public:
		struct SkyPassData {

		};

	public:
		SkyPassData skyPassData;
	
		std::unique_ptr<Renderer::Mesh> cubeMesh;

	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);
	};

}