#pragma once
#include "ResourceFormat.h"
#include <string>
#include <unordered_map>

namespace Renderer {

	class Buffer;
	class Texture;

	/*
	* ��������
	*/
	struct RGTextureDesc {
		GHL::ETextureDimension dimension = GHL::ETextureDimension::Texture2D;
		uint32_t               width = 0u;
		uint32_t               height = 0u;
		uint32_t               depth = 1u;
		uint32_t               arraySize = 1u;
		uint32_t               mipLevals = 1u;
		uint32_t               sampleCount = 1u;
		DXGI_FORMAT            format = DXGI_FORMAT_UNKNOWN;
		GHL::EResourceUsage    usage = GHL::EResourceUsage::Default;
		GHL::ETextureMiscFlag  miscFlag = GHL::ETextureMiscFlag::None;
		D3D12_CLEAR_VALUE      clearValue{};
	};

	/*
	* ��������
	*/
	struct RGBufferDesc {
		uint32_t               stride = 0u;
		size_t                 size = 0u;
		DXGI_FORMAT            format = DXGI_FORMAT_UNKNOWN;
		GHL::EResourceUsage    usage = GHL::EResourceUsage::Upload;
		GHL::EBufferMiscFlag   miscFlag = GHL::EBufferMiscFlag::None;
	};

	using RGResourceDesc = std::variant<RGTextureDesc, RGBufferDesc>;

	class RenderGraphResource {
	public:
		using ResourceUsageTimeline = std::pair<uint64_t, uint64_t>;

	public:
		RenderGraphResource(const GHL::Device* device, const std::string& name);
		RenderGraphResource(const std::string& name, Texture* resource);
		RenderGraphResource(const std::string& name, Buffer*  resource);
		~RenderGraphResource() = default;

		void StartTimeline(uint64_t nodeGlobalExecutionIndex);

		void UpdateTimeline(uint64_t nodeGlobalExecutionIndex);

		void SetExpectedStates(uint64_t nodeIndex, GHL::EResourceState states);

	private:
		const GHL::Device* mDevice{ nullptr };
		std::string mResName; // ��Դ����

		Texture* mTexture{ nullptr };
		Buffer*  mBuffer { nullptr };

		// ��������Դ������Ϣ

		bool mImported{ false };         // ��Դ�Ƿ��������ⲿ����
		ResourceUsageTimeline mTimeline; // ��Դ����������
		ResourceFormat mResourceFormat;
		uint64_t mHeapOffset{ 0u };
		std::unordered_map<uint64_t, GHL::EResourceState> mExpectedStatesPerPass; // ����Pass�Ը���Դ��Ҫ��
	};

}