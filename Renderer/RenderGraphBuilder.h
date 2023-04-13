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
		* 创建一个初始状态为RenderTarget的Texture
		*/
		void NewRenderTarget(const std::string& name, const NewTextureProperties& desc);

		/*
		* 创建一个初始状态为DepthWrite的Texture
		*/
		void NewDepthStencil(const std::string& name, const NewTextureProperties& desc);

		/*
		* 创建一个初始状态为UnorderedAccess的Texture
		*/
		void NewTexture(const std::string& name, const NewTextureProperties& desc);

		/*
		* 定义当前Pass期望该资源状态为PixelAccess / NonPixelAccess / AnyPixelAccess
		*/
		void ReadTexture(const std::string& name, const ShaderAccessFlag& accessFlag, uint32_t firstMip = 0u, uint32_t mipCount = -1);

		/*
		* 定义当前Pass期望该资源状态为UnorderedAccess
		*/
		void WriteTexture(const std::string& name, uint32_t firstMip = 0u, uint32_t mipCount = -1);

		/*
		* 定义当前Pass期望该资源状态为DepthRead
		*/
		void ReadDepthStencil(const std::string& name, uint32_t firstMip = 0u, uint32_t mipCount = -1);

		/*
		* 定义当前Pass期望该资源状态为DepthWrite
		*/
		void WriteDepthStencil(const std::string& name, uint32_t firstMip = 0u, uint32_t mipCount = -1);

		/*
		* 定义一个纹理复制的操作
		*/
		void CopyTexture(const std::string& src, const std::string& dst);


		/*
		* 创建一个初始状态为UnorderedAccess的Buffer
		*/
		void NewBuffer(const std::string& name, const NewBufferProperties& desc);

		/*
		* 定义当前Pass期望该资源状态为PixelAccess / NonPixelAccess / AnyPixelAccess
		*/
		void ReadBuffer(const std::string& name, const ShaderAccessFlag& accessFlag);

		/*
		* 定义当前Pass期望该资源状态为UnorderedAccess
		*/
		void WriteBuffer(const std::string& name);

		/*
		* 定义一个缓冲复制的操作
		*/
		void CopyBuffer(const std::string& src, const std::string dst);

		/*
		* 设置Pass所属的GPU引擎(默认为图形引擎)
		*/
		void SetPassExecutionQueue(GHL::EGPUQueue queueIndex = GHL::EGPUQueue::Graphics);

	private:
		PassNode* mPassNode{ nullptr };
		RenderGraphResourceStorage* mResourceStorage{ nullptr };
	};

}