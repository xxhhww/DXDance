#pragma once
#include <vector>

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

	class GrasslandBackend {
	public:


	private:
		RenderEngine* mRenderEngine{ nullptr };

		std::vector<GrasslandNodeDescriptor>   mGrasslandNodeDescriptors;		// 草地全节点描述状态表
		std::vector<GrasslandNodeRuntimeState> mGrasslandNodeRuntimeStates;		// 草地全节点运行时状态


	};

}