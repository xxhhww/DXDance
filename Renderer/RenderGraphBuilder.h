#pragma once
#include "RenderGraph.h"
#include "RenderGraphResourceStorage.h"

namespace Renderer {

	class RenderGraphBuilder {
	public:
		RenderGraphBuilder(RenderGraph::GraphNode* graphNode, RenderGraphResourceStorage* resourceStorage);
		~RenderGraphBuilder() = default;

		/*
		* 创建一个初始状态为RenderTarget的Texture
		*/
		void NewRenderTarget(const std::string& name, const RGTextureDesc& desc);

		/*
		* 创建一个初始状态为DepthWrite的Texture
		*/
		void NewDepthStencil(const std::string& name, const RGTextureDesc& desc);

		/*
		* 创建一个初始状态为UnorderedAccess的Texture
		*/
		void NewTexture(const std::string& name, const RGTextureDesc& desc);
		
		/*
		* 创建一个初始状态为UnorderedAccess的Buffer
		*/
		void NewBuffer(const std::string& name, const RGBufferDesc& desc);

		// 目前，在定义资源操作时先忽略 读写的粒度，在实际录制命令时仍然可以指定 读写的粒度

		// 定义当前Pass期望该资源状态为PixelAccess / NonPixelAccess / AnyPixelAccess
		void ReadTexture(const std::string& name);

		// 定义当前Pass期望该资源状态为UnorderedAccess
		void WriteTexture(const std::string& name);

		// 定义当前Pass期望该资源状态为DepthRead
		void ReadDepthStencil(const std::string& name);

		// 定义当前Pass期望该资源状态为DepthWrite
		void WriteDepthStencil(const std::string& name);

		// 定义当前Pass期望该资源状态为PixelAccess / NonPixelAccess / AnyPixelAccess
		void ReadBuffer(const std::string& name);

		// 定义当前Pass期望该资源状态为UnorderedAccess
		void WriteBuffer(const std::string& name);

		/*
		* 定义一个纹理复制的操作
		*/
		void CopyTexture(const std::string& src, const std::string& dst);

		/*
		* 定义一个缓冲复制的操作
		*/
		void CopyBuffer(const std::string& src, const std::string dst);

		/*
		* 设置Pass所属的GPU引擎(默认为图形引擎)
		*/
		void SetPassExecutionQueue(PassExecutionQueue queueIndex = PassExecutionQueue::General);

	private:
		RenderGraph::GraphNode* mGraphNode{ nullptr };
		RenderGraphResourceStorage* mResourceStorage{ nullptr };
	};

}