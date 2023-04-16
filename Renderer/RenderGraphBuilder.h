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
		* 定义一个纹理资源
		*/
		RenderGraphResourceID DeclareTexture(const std::string& name, const NewTextureProperties& properties);

		/*
		* 声明一个缓冲资源
		*/
		RenderGraphResourceID DeclareBuffer(const std::string& name, const NewBufferProperties& properties);

		/*
		* 以RenderTarget状态写入纹理资源
		*/
		RenderGraphResourceID WriteRenderTarget(const std::string& name, uint32_t mipLevel = 0u);

		/*
		* 以DepthWrite状态写入纹理资源
		*/
		RenderGraphResourceID WriteDepthStencil(const std::string& name, uint32_t mipLevel = 0u);

		/*
		* 以UnorderedAccess状态写入纹理资源
		*/
		RenderGraphResourceID WriteTexture(const std::string& name, uint32_t mipLevel = 0u);

		/*
		* 定义当前Pass期望该资源状态为PixelAccess / NonPixelAccess / AnyPixelAccess
		*/
		RenderGraphResourceID ReadTexture(const std::string& name, const ShaderAccessFlag& accessFlag, uint32_t mipLevel = 0u);

		/*
		* 定义当前Pass期望该资源状态为DepthRead
		*/
		RenderGraphResourceID ReadDepthStencil(const std::string& name, uint32_t mipLevel = 0u);

		/*
		* 定义当前Pass期望该资源状态为UnorderedAccess
		*/
		RenderGraphResourceID WriteBuffer(const std::string& name);

		/*
		* 定义当前Pass期望该资源状态为PixelAccess / NonPixelAccess / AnyPixelAccess
		*/
		RenderGraphResourceID ReadBuffer(const std::string& name, const ShaderAccessFlag& accessFlag);

		/*
		* 设置Pass所属的GPU引擎(默认为图形引擎)
		*/
		void SetPassExecutionQueue(GHL::EGPUQueue queueIndex = GHL::EGPUQueue::Graphics);

	private:
		PassNode* mPassNode{ nullptr };
		RenderGraphResourceStorage* mResourceStorage{ nullptr };
	};

}