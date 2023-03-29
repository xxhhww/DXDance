#include "RenderGraphResourceStorage.h"
#include "Tools/Assert.h"

namespace Renderer {

	RenderGraphResourceStorage::RenderGraphResourceStorage() {

	}

	RenderGraphResource* RenderGraphResourceStorage::DeclareResource(const std::string& name) {
		ASSERT_FORMAT(mRenderGraphResources.find(name) != mRenderGraphResources.end(), "Resource: ", name, " is Redeclared!");
		mRenderGraphResources[name] = std::make_unique<RenderGraphResource>(name);
		return mRenderGraphResources.at(name).get();
	}

}