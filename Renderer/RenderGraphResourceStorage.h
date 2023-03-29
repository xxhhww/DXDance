#pragma once
#include "RenderGraphResource.h"
#include "GHL/Heap.h"

#include <memory>
#include <functional>

namespace Renderer {

	class RenderGraphResourceStorage {
	public:
		RenderGraphResourceStorage();
		~RenderGraphResourceStorage() = default;

		/*
		* ����һ���µ���Դ�������䷵��
		*/
		RenderGraphResource* DeclareResource(const std::string& name);

		inline auto* GetResource(const std::string& name) const { return mRenderGraphResources.at(name).get(); }

	private:
		const GHL::Device* mDevice{ nullptr };
		std::unique_ptr<GHL::Heap> mHeap;
		// �Զ�������ָ���ɾ������������Imported����Դ��������Delete
		std::unordered_map<std::string, std::unique_ptr<RenderGraphResource, std::function<void(RenderGraphResource*)>>> mRenderGraphResources;
	};

}