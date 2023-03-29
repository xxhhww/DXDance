#pragma once
#include "RenderGraph.h"
#include "RenderGraphResourceStorage.h"

namespace Renderer {

	class RenderGraphBuilder {
	public:
		RenderGraphBuilder(RenderGraph::GraphNode* graphNode, RenderGraphResourceStorage* resourceStorage);
		~RenderGraphBuilder() = default;

		/*
		* ����һ����ʼ״̬ΪRenderTarget��Texture
		*/
		void NewRenderTarget(const std::string& name, const RGTextureDesc& desc);

		/*
		* ����һ����ʼ״̬ΪDepthWrite��Texture
		*/
		void NewDepthStencil(const std::string& name, const RGTextureDesc& desc);

		/*
		* ����һ����ʼ״̬ΪUnorderedAccess��Texture
		*/
		void NewTexture(const std::string& name, const RGTextureDesc& desc);
		
		/*
		* ����һ����ʼ״̬ΪUnorderedAccess��Buffer
		*/
		void NewBuffer(const std::string& name, const RGBufferDesc& desc);

		// Ŀǰ���ڶ�����Դ����ʱ�Ⱥ��� ��д�����ȣ���ʵ��¼������ʱ��Ȼ����ָ�� ��д������

		// ���嵱ǰPass��������Դ״̬ΪPixelAccess / NonPixelAccess / AnyPixelAccess
		void ReadTexture(const std::string& name);

		// ���嵱ǰPass��������Դ״̬ΪUnorderedAccess
		void WriteTexture(const std::string& name);

		// ���嵱ǰPass��������Դ״̬ΪDepthRead
		void ReadDepthStencil(const std::string& name);

		// ���嵱ǰPass��������Դ״̬ΪDepthWrite
		void WriteDepthStencil(const std::string& name);

		// ���嵱ǰPass��������Դ״̬ΪPixelAccess / NonPixelAccess / AnyPixelAccess
		void ReadBuffer(const std::string& name);

		// ���嵱ǰPass��������Դ״̬ΪUnorderedAccess
		void WriteBuffer(const std::string& name);

		/*
		* ����һ�������ƵĲ���
		*/
		void CopyTexture(const std::string& src, const std::string& dst);

		/*
		* ����һ�����帴�ƵĲ���
		*/
		void CopyBuffer(const std::string& src, const std::string dst);

		/*
		* ����Pass������GPU����(Ĭ��Ϊͼ������)
		*/
		void SetPassExecutionQueue(PassExecutionQueue queueIndex = PassExecutionQueue::General);

	private:
		RenderGraph::GraphNode* mGraphNode{ nullptr };
		RenderGraphResourceStorage* mResourceStorage{ nullptr };
	};

}