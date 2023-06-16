#pragma once
#include "RenderGraphResourceID.h"
#include "MemoryAliasingHelper.h"
#include "RootConstantsPerFrame.h"

#include <memory>
#include <functional>

namespace GHL {

	class Device;
	class Heap;

}

namespace Renderer {

	class Resource;
	class RenderGraphResource;
	class PoolDescriptorAllocator;

	class RenderGraphResourceStorage {
	public:
		RenderGraphResourceStorage(const GHL::Device* device, PoolDescriptorAllocator* descriptorAllocator);
		~RenderGraphResourceStorage() = default;

		void BuildAliasing();

		/*
		* Import External Pipeline Resource
		*/
		RenderGraphResource* ImportResource(const std::string& name, Resource* resource);

		void ExportResource(const std::string& name);

		RenderGraphResource* DeclareResource(const std::string& name);

		RenderGraphResource* GetResourceByName(const std::string& name);
		
		RenderGraphResource* GetResourceByID(const RenderGraphResourceID& resourceID);

		inline auto&       GetResources()       { return mRenderGraphResources; }
		inline const auto& GetResources() const { return mRenderGraphResources; }

	public:
		RootConstantsPerFrame rootConstantsPerFrame;
		D3D12_GPU_VIRTUAL_ADDRESS rootConstantsPerFrameAddress;

		std::vector<GPULight> rootLightDataPerFrame;
		D3D12_GPU_VIRTUAL_ADDRESS rootLightDataPerFrameAddress;

	private:
		const GHL::Device* mDevice{ nullptr };
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };

		std::unique_ptr<GHL::Heap> mHeap;
		std::unique_ptr<MemoryAliasingHelper> mAliasingHelper;
		// 自定义智能指针的删除操作。对于Imported的资源，不进行Delete
		std::unordered_map<RenderGraphResourceID, std::unique_ptr<RenderGraphResource>, RenderGraphResourceID::HashFunc> mRenderGraphResources;
	};

}