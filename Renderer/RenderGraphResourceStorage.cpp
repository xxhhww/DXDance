#include "RenderGraphResourceStorage.h"
#include "Tools/Assert.h"

namespace Renderer {

	RenderGraphResourceStorage::RenderGraphResourceStorage(const GHL::Device* device) 
	: mDevice(device) 
	, mAliasingHelper(this) {

	}

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