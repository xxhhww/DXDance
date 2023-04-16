#pragma once
#include "RenderGraphResourceProperties.h"
#include "RenderGraphItem.h"

namespace Renderer {

	class  RenderGraphResourceStorage;

	class RenderGraphBuilder {
	public:
		RenderGraphBuilder(PassNode* passNode, RenderGraphResourceStorage* resourceStorage);
		~RenderGraphBuilder() = default;

		/*
		* ����һ��������Դ
		*/
		RenderGraphResourceID DeclareTexture(const std::string& name, const NewTextureProperties& properties);

		/*
		* ����һ��������Դ
		*/
		RenderGraphResourceID DeclareBuffer(const std::string& name, const NewBufferProperties& properties);

		/*
		* ��RenderTarget״̬д��������Դ
		*/
		RenderGraphResourceID WriteRenderTarget(const std::string& name, uint32_t mipLevel = 0u);

		/*
		* ��DepthWrite״̬д��������Դ
		*/
		RenderGraphResourceID WriteDepthStencil(const std::string& name, uint32_t mipLevel = 0u);

		/*
		* ��UnorderedAccess״̬д��������Դ
		*/
		RenderGraphResourceID WriteTexture(const std::string& name, uint32_t mipLevel = 0u);

		/*
		* ���嵱ǰPass��������Դ״̬ΪPixelAccess / NonPixelAccess / AnyPixelAccess
		*/
		RenderGraphResourceID ReadTexture(const std::string& name, const ShaderAccessFlag& accessFlag, uint32_t mipLevel = 0u);

		/*
		* ���嵱ǰPass��������Դ״̬ΪDepthRead
		*/
		RenderGraphResourceID ReadDepthStencil(const std::string& name, uint32_t mipLevel = 0u);

		/*
		* ���嵱ǰPass��������Դ״̬ΪUnorderedAccess
		*/
		RenderGraphResourceID WriteBuffer(const std::string& name);

		/*
		* ���嵱ǰPass��������Դ״̬ΪPixelAccess / NonPixelAccess / AnyPixelAccess
		*/
		RenderGraphResourceID ReadBuffer(const std::string& name, const ShaderAccessFlag& accessFlag);

		/*
		* ����Pass������GPU����(Ĭ��Ϊͼ������)
		*/
		void SetPassExecutionQueue(GHL::EGPUQueue queueIndex = GHL::EGPUQueue::Graphics);

	private:
		PassNode* mPassNode{ nullptr };
		RenderGraphResourceStorage* mResourceStorage{ nullptr };
	};

}