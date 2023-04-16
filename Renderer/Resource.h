#pragma once
#include "GHL/Resource.h"
#include "ResourceFormat.h"

namespace Renderer {

	class Resource : public GHL::Resource {
	public:
		Resource(const GHL::Device* device, const ResourceFormat& resourceFormat);
		virtual ~Resource() = default;

		inline const auto* GetDevice()         const { return mDevice; }
		inline const auto& GetResourceFormat() const { return mResourceFormat; }

		bool IsBuffer() const;

	protected:
		const GHL::Device* mDevice{ nullptr };
		ResourceFormat mResourceFormat;
	};

}