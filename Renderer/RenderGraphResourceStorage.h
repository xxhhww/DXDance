#pragma once
#include "RenderGraphResourceID.h"
#include "MemoryAliasingHelper.h"
#include <memory>
#include <functional>

namespace GHL {

	class Device;
	class Heap;

}

namespace Renderer {

	class Texture;
	class Buffer;
	class RenderGraphResource;
	class PoolDescriptorAllocator;

	class RenderGraphResourceStorage {
	public:
		RenderGraphResourceStorage(const GHL::Device* device, PoolDescriptorAllocator* descriptorAllocator);
		~RenderGraphResourceStorage() = default;

		void BuildAliasing();

		/*
		* Import External Texture Pipeline Resource
		*/
		RenderGraphResource* ImportResource(const std::string& name, Texture* resource);

		/*
		* Import External Buffer Pipeline Resource
		*/
		RenderGraphResource* ImportResource(const std::string& name, Buffer* resource);

		RenderGraphResource* DeclareResource(const std::string& name);

		RenderGraphResource* GetResourceByName(const std::string& name);
		
		RenderGraphResource* GetResourceByID(const RenderGraphResourceID& resourceID);

		inline auto&       GetResources()       { return mRenderGraphResources; }
		inline const auto& GetResources() const { return mRenderGraphResources; }

	private:
		const GHL::Device* mDevice{ nullptr };
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };

		std::unique_ptr<GHL::Heap> mHeap;
		std::unique_ptr<MemoryAliasingHelper> mAliasingHelper;
		// 自定义智能指针的删除操作。对于Imported的资源，不进行Delete
		std::unordered_map<RenderGraphResourceID, std::unique_ptr<RenderGraphResource>, RenderGraphResourceID::HashFunc> mRenderGraphResources;
	};

}