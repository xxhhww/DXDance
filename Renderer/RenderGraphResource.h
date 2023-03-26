#pragma once
#include <string>
#include <unordered_map>

#include "Buffer.h"
#include "Texture.h"

namespace Renderer {

	template<typename ResourceType>
	struct RGResourceTraits;

	template<>
	struct RGResourceTraits<Buffer>
	{
		using Resource = Buffer;
		using ResourceDesc = BufferDesc;
	};

	template<>
	struct RGResourceTraits<Texture> {
		using Resource = Texture;
		using ResourceDesc = TextureDesc;
	};

	/*
	* ��Ⱦͼ���й������Դ
	*/
	template<typename ResourceType>
	class RenderGraphResource {
	public:
		using ResourceUsageTimeline = std::pair<uint64_t, uint64_t>;
		using Resource = RGResourceTraits<ResourceType>::Resource;
		using ResourceDesc = RGResourceTraits<ResourceType>::ResourceDesc;

	public:
		RenderGraphResource(size_t resID, const ResourceDesc& desc, bool imported = false)
		: mResID(resID)
		, mResourceDesc(desc)
		, mImoprted(imported) {}

		RenderGraphResource(size_t resID, Resource* resource, bool imported = true)
		: mResID(resID)
		, mResource(resource)
		, mImoprted(imported) {}

		~RenderGraphResource() = default;

	private:
		size_t mResID{ 0u };             // ��ԴID
		bool mImported{ false };         // ��Դ�Ƿ��������ⲿ����
		ResourceUsageTimeline mTimeline; // ��Դ����������

		Resource* mResource{ nullptr };
		ResourceDesc mResourceDesc{};
	};

	using RGTexture = RenderGraphResource<Texture>;
	using RGBuffer  = RenderGraphResource<Buffer>;

	using RGResourceID = uint64_t;

}