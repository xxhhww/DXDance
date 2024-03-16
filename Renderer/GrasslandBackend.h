#pragma once
#include <vector>

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

	class GrasslandBackend {
	public:


	private:
		RenderEngine* mRenderEngine{ nullptr };

		std::vector<GrasslandNodeDescriptor>   mGrasslandNodeDescriptors;		// �ݵ�ȫ�ڵ�����״̬��
		std::vector<GrasslandNodeRuntimeState> mGrasslandNodeRuntimeStates;		// �ݵ�ȫ�ڵ�����ʱ״̬


	};

}