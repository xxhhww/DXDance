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
		* ����һ����ʼ״̬ΪRenderTarget��Texture
		*/
		void NewRenderTarget(const std::string& name, const NewTextureProperties& desc);

		/*
		* ����һ����ʼ״̬ΪDepthWrite��Texture
		*/
		void NewDepthStencil(const std::string& name, const NewTextureProperties& desc);

		/*
		* ����һ����ʼ״̬ΪUnorderedAccess��Texture
		*/
		void NewTexture(const std::string& name, const NewTextureProperties& desc);

		/*
		* ���嵱ǰPass��������Դ״̬ΪPixelAccess / NonPixelAccess / AnyPixelAccess
		*/
		void ReadTexture(const std::string& name, const ShaderAccessFlag& accessFlag, uint32_t firstMip = 0u, uint32_t mipCount = -1);

		/*
		* ���嵱ǰPass��������Դ״̬ΪUnorderedAccess
		*/
		void WriteTexture(const std::string& name, uint32_t firstMip = 0u, uint32_t mipCount = -1);

		/*
		* ���嵱ǰPass��������Դ״̬ΪDepthRead
		*/
		void ReadDepthStencil(const std::string& name, uint32_t firstMip = 0u, uint32_t mipCount = -1);

		/*
		* ���嵱ǰPass��������Դ״̬ΪDepthWrite
		*/
		void WriteDepthStencil(const std::string& name, uint32_t firstMip = 0u, uint32_t mipCount = -1);

		/*
		* ����һ�������ƵĲ���
		*/
		void CopyTexture(const std::string& src, const std::string& dst);


		/*
		* ����һ����ʼ״̬ΪUnorderedAccess��Buffer
		*/
		void NewBuffer(const std::string& name, const NewBufferProperties& desc);

		/*
		* ���嵱ǰPass��������Դ״̬ΪPixelAccess / NonPixelAccess / AnyPixelAccess
		*/
		void ReadBuffer(const std::string& name, const ShaderAccessFlag& accessFlag);

		/*
		* ���嵱ǰPass��������Դ״̬ΪUnorderedAccess
		*/
		void WriteBuffer(const std::string& name);

		/*
		* ����һ�����帴�ƵĲ���
		*/
		void CopyBuffer(const std::string& src, const std::string dst);

		/*
		* ����Pass������GPU����(Ĭ��Ϊͼ������)
		*/
		void SetPassExecutionQueue(GHL::EGPUQueue queueIndex = GHL::EGPUQueue::Graphics);

	private:
		PassNode* mPassNode{ nullptr };
		RenderGraphResourceStorage* mResourceStorage{ nullptr };
	};

}