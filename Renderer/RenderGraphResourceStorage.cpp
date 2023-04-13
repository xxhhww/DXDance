#include "RenderGraphResourceStorage.h"
#include "RenderGraphResource.h"
#include "MemoryAliasingHelper.h"

#include "Texture.h"
#include "Buffer.h"

#include "Tools/Assert.h"
#include "Tools/VisitorHelper.h"

namespace Renderer {

	RenderGraphResourceStorage::RenderGraphResourceStorage(const GHL::Device* device, PoolDescriptorAllocator* descriptorAllocator)
	: mDevice(device)
	, mDescriptorAllocator(descriptorAllocator) 
	, mAliasingHelper(std::make_unique<MemoryAliasingHelper>()) {}

	void RenderGraphResourceStorage::BuildAliasing() {
		for (auto& pair : mRenderGraphResources) {
			pair.second->BuildResourceFormat();
			mAliasingHelper->AddResource(pair.second.get());
		}

		// 构建资源别名
		size_t totalHeapSize = mAliasingHelper->BuildAliasing();

		// 创建默认堆
		mHeap = std::make_unique<GHL::Heap>(mDevice, totalHeapSize, GHL::EResourceUsage::Default);

		// 为非Imported的资源进行创建工作
		for (auto& pair : mRenderGraphResources) {
			if (pair.second->imported) continue;

			std::visit(MakeVisitor(
				[&](const NewTextureProperties& properties) {
					pair.second->texture = new Texture(mDevice, pair.second->resourceFormat, mDescriptorAllocator, mHeap.get(), pair.second->heapOffset);
				},
				[&](const NewBufferProperties& properties) {
					pair.second->buffer = new Buffer(mDevice, pair.second->resourceFormat, mDescriptorAllocator, mHeap.get(), pair.second->heapOffset);
				})
				, pair.second->newResourceProperties);
		}
	}

	RenderGraphResource* RenderGraphResourceStorage::ImportResource(const std::string& name, Texture* resource) {

		RenderGraphResourceID resourceID = RenderGraphResourceID::FindOrCreateResourceID(name);
		ASSERT_FORMAT(mRenderGraphResources.find(resourceID) == mRenderGraphResources.end(), "Resource: ", name, " is Redeclared!");
		mRenderGraphResources[resourceID] = std::make_unique<RenderGraphResource>(name, resource);
		return mRenderGraphResources.at(resourceID).get();
	}

	RenderGraphResource* RenderGraphResourceStorage::ImportResource(const std::string& name, Buffer* resource) {

		RenderGraphResourceID resourceID = RenderGraphResourceID::FindOrCreateResourceID(name);
		ASSERT_FORMAT(mRenderGraphResources.find(resourceID) == mRenderGraphResources.end(), "Resource: ", name, " is Redeclared!");
		mRenderGraphResources[resourceID] = std::make_unique<RenderGraphResource>(name, resource);
		return mRenderGraphResources.at(resourceID).get();
	}

	RenderGraphResource* RenderGraphResourceStorage::DeclareResource(const std::string& name) {

		RenderGraphResourceID resourceID = RenderGraphResourceID::FindOrCreateResourceID(name);
		ASSERT_FORMAT(mRenderGraphResources.find(resourceID) == mRenderGraphResources.end(), "Resource: ", name, " is Redeclared!");
		mRenderGraphResources[resourceID] = std::make_unique<RenderGraphResource>(mDevice, name);
		return mRenderGraphResources.at(resourceID).get();
	}

	RenderGraphResource* RenderGraphResourceStorage::GetResourceByName(const std::string& name) {

		RenderGraphResourceID resourceID = RenderGraphResourceID::FindOrCreateResourceID(name);
		ASSERT_FORMAT(mRenderGraphResources.find(resourceID) != mRenderGraphResources.end(), "Resource: ", name, " is not Declared!");
		return mRenderGraphResources.at(resourceID).get();
	}

	RenderGraphResource* RenderGraphResourceStorage::GetResourceByID(const RenderGraphResourceID& resourceID) {

		ASSERT_FORMAT(mRenderGraphResources.find(resourceID) != mRenderGraphResources.end(), "Resource is not Declared!");
		return mRenderGraphResources.at(resourceID).get();
	}

}