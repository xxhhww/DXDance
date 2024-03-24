#pragma once
#include <vector>
#include "Renderer/GrasslandLinearBuffer.h"
#include "Renderer/GrasslandLinearBufferCache.h"


namespace Renderer {

	class RenderEngine;

	/*
	* �ݵؽڵ���������
	*/
	struct GrasslandNodeRequestTask {
	public:

	};

	/*
	* �ݵؽڵ�����
	*/
	struct GrasslandNodeDescriptor {
	public:

	};

	/*
	* �ݵؽڵ�ʵʱ״̬
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

		std::vector<GrasslandNodeDescriptor>   mGrasslandNodeDescriptors;		// �ݵ�ȫ�ڵ�����״̬��(GPU��)
		std::vector<GrasslandNodeRuntimeState> mGrasslandNodeRuntimeStates;		// �ݵ�ȫ�ڵ�����ʱ״̬(CPU��)

		std::unique_ptr<GrasslandLinearBuffer>      mGrasslandLinearBuffer;
		std::unique_ptr<GrasslandLinearBufferCache> mGrasslandLinearBufferCache;
	};

}