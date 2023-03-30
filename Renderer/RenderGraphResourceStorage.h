#pragma once
#include "RenderGraphResource.h"
#include "GHL/Heap.h"

#include <memory>
#include <functional>

namespace Renderer {

	class RenderGraphResourceStorage {
	public:
		RenderGraphResourceStorage(const GHL::Device* device);
		~RenderGraphResourceStorage() = default;

		void Build();

		RenderGraphResource* DeclareResource(const std::string& name);

		RenderGraphResource* GetResource(const std::string& name) const;

	private:
		const GHL::Device* mDevice{ nullptr };
		std::unique_ptr<GHL::Heap> mHeap;
		// 自定义智能指针的删除操作。对于Imported的资源，不进行Delete
		std::unordered_map<std::string, std::unique_ptr<RenderGraphResource, std::function<void(RenderGraphResource*)>>> mRenderGraphResources;
	};

}