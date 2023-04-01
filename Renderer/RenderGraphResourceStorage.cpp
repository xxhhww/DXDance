#include "RenderGraphResourceStorage.h"
#include "RenderGraphResource.h"
#include "Tools/Assert.h"
#include "Tools/VisitorHelper.h"
#include "Texture.h"
#include "Buffer.h"

namespace Renderer {

	RenderGraphResourceStorage::RenderGraphResourceStorage(const GHL::Device* device, PoolDescriptorAllocator* descriptorAllocator)
	: mDevice(device)
	, mDescriptorAllocator(descriptorAllocator) {}

	void RenderGraphResourceStorage::Build() {
		for (auto& pair : mRenderGraphResources) {
			pair.second->BuildResourceFormat();
			mAliasingHelper.AddResource(pair.second.get());
		}

		// ������Դ����
		size_t totalHeapSize = mAliasingHelper.BuildAliasing();

		// ����Ĭ�϶�
		mHeap = std::make_unique<GHL::Heap>(mDevice, totalHeapSize, GHL::EResourceUsage::Default);

		// Ϊ��Imported����Դ���д�������
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

	RenderGraphResource* RenderGraphResourceStorage::DeclareResource(const std::string& name) {
		ASSERT_FORMAT(mRenderGraphResources.find(name) == mRenderGraphResources.end(), "Resource: ", name, " is Redeclared!");
		mRenderGraphResources[name] = std::make_unique<RenderGraphResource>(mDevice, name);
		return mRenderGraphResources.at(name).get();
	}

	RenderGraphResource* RenderGraphResourceStorage::GetResource(const std::string& name) const {
		ASSERT_FORMAT(mRenderGraphResources.find(name) != mRenderGraphResources.end(), "Resource: ", name, " is not Declared!");
		return mRenderGraphResources.at(name).get();
	}

}