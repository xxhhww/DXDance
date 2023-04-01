#pragma once
#include "MemoryAliasingHelper.h"

#include "GHL/Heap.h"

#include <memory>
#include <functional>

namespace Renderer {

	class RenderGraphResource;
	class PoolDescriptorAllocator;

	class RenderGraphResourceStorage {
	public:
		RenderGraphResourceStorage(const GHL::Device* device, PoolDescriptorAllocator* descriptorAllocator);
		~RenderGraphResourceStorage() = default;

		void Build();

		RenderGraphResource* DeclareResource(const std::string& name);

		RenderGraphResource* GetResource(const std::string& name) const;

	private:
		const GHL::Device* mDevice{ nullptr };
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };
		std::unique_ptr<GHL::Heap> mHeap;

		MemoryAliasingHelper mAliasingHelper;
		// �Զ�������ָ���ɾ������������Imported����Դ��������Delete
		std::unordered_map<std::string, std::unique_ptr<RenderGraphResource, std::function<void(RenderGraphResource*)>>> mRenderGraphResources;
	};

}