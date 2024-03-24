#pragma once
#include <vector>
#include "Renderer/GrasslandLinearBuffer.h"
#include "Renderer/GrasslandLinearBufferCache.h"


namespace Renderer {

	class RenderEngine;

	/*
	* 草地节点请求任务
	*/
	struct GrasslandNodeRequestTask {
	public:

	};

	/*
	* 草地节点描述
	*/
	struct GrasslandNodeDescriptor {
	public:

	};

	/*
	* 草地节点实时状态
	*/
	struct GrasslandNodeRuntimeState {
	public:

	};

	class GrasslandRenderer {
	public:
		GrasslandRenderer(RenderEngine* renderEngine);
		~GrasslandRenderer();

		void Initialize();

		void AddPass();

	private:
		RenderEngine* mRenderEngine{ nullptr };

		std::vector<GrasslandNodeDescriptor>   mGrasslandNodeDescriptors;		// 草地全节点描述状态表(GPU端)
		std::vector<GrasslandNodeRuntimeState> mGrasslandNodeRuntimeStates;		// 草地全节点运行时状态(CPU端)

		std::unique_ptr<GrasslandLinearBuffer>      mGrasslandLinearBuffer;
		std::unique_ptr<GrasslandLinearBufferCache> mGrasslandLinearBufferCache;
	};

}