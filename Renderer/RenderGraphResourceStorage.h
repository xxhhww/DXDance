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
		* 声明一个新的资源，并将其返回
		*/
		RenderGraphResource* DeclareResource(const std::string& name);

		inline auto* GetResource(const std::string& name) const { return mRenderGraphResources.at(name).get(); }

	private:
		const GHL::Device* mDevice{ nullptr };
		std::unique_ptr<GHL::Heap> mHeap;
		// 自定义智能指针的删除操作。对于Imported的资源，不进行Delete
		std::unordered_map<std::string, std::unique_ptr<RenderGraphResource, std::function<void(RenderGraphResource*)>>> mRenderGraphResources;
	};

}