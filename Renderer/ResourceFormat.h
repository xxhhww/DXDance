#pragma once
#include "GHL/pbh.h"
#include "GHL/Device.h"

#include <variant>

namespace Renderer {

	/*
	* 纹理描述
	*/
	struct TextureDesc {
		GHL::ETextureDimension dimension = GHL::ETextureDimension::Texture2D; // 纹理的维度
		uint32_t               width = 0u;
		uint32_t               height = 0u;
		uint32_t               depth = 1u;	// 当纹理维度是三维时，启用depth标记
		uint32_t               arraySize = 1u;
		uint32_t               mipLevals = 1u;
		uint32_t               sampleCount = 1u;
		DXGI_FORMAT            format = DXGI_FORMAT_UNKNOWN;
		GHL::EResourceUsage    usage = GHL::EResourceUsage::Default;	// 纹理默认在Default堆上进行创建
		GHL::ETextureMiscFlag  miscFlag = GHL::ETextureMiscFlag::None;
		GHL::EResourceState    initialState = GHL::EResourceState::Common;
		GHL::EResourceState    expectedState = GHL::EResourceState::Common;
		D3D12_CLEAR_VALUE      clearVaule = D3D12_CLEAR_VALUE{};
		bool                   supportStream = false;	// 是否使用支持纹理流
	};

	/*
	* 纹理子资源描述
	*/
	struct TextureSubResourceDesc {
	public:

		bool operator==(const TextureSubResourceDesc& other) const {
			return firstSlice == other.firstSlice && sliceCount == other.sliceCount && firstMip == other.firstMip && other.mipCount == other.mipCount;
		}

		uint32_t firstSlice = 0u;
		uint32_t sliceCount = static_cast<uint32_t>(-1); // -1 表示全部
		uint32_t firstMip = 0u;
		uint32_t mipCount = static_cast<uint32_t>(-1); // -1 表示全部
	};

	struct TextureSubResourceDescHashFunc {
	public:
		std::size_t operator()(const TextureSubResourceDesc& desc) const {
			return std::hash<uint32_t>()(desc.firstSlice) ^ std::hash<uint32_t>()(desc.sliceCount) ^ std::hash<uint32_t>()(desc.firstMip) ^ std::hash<uint32_t>()(desc.mipCount);
		}
	};

	/*
	* 缓冲描述
	*/
	struct BufferDesc {
		uint32_t               stride = 1u;
		size_t                 size = 0u;
		DXGI_FORMAT            format = DXGI_FORMAT_UNKNOWN;
		GHL::EResourceUsage    usage = GHL::EResourceUsage::Upload;
		GHL::EBufferMiscFlag   miscFlag = GHL::EBufferMiscFlag::None;
		GHL::EResourceState    initialState = GHL::EResourceState::Common;
		GHL::EResourceState    expectedState = GHL::EResourceState::Common;
	};

	/*
	* 缓冲子资源描述
	*/
	struct BufferSubResourceDesc {
		size_t offset = 0u;
		size_t size = static_cast<size_t>(-1);
	};

	using ResourceDescVariant = std::variant<TextureDesc, BufferDesc>;

	/*
	* 需要由Device来查询申请资源的大小，因此做此封装
	*/
	class ResourceFormat {
	public:
		ResourceFormat(const GHL::Device* device);
		ResourceFormat(const GHL::Device* device, const TextureDesc& desc);
		ResourceFormat(const GHL::Device* device, const BufferDesc& desc);

		void Build();

		void SetTextureDesc(const TextureDesc& desc);

		void SetBufferDesc(const BufferDesc& desc);

		inline const auto& D3DResourceDesc() const { return mD3DResourceDesc; }

		inline const auto& GetAlignment()           const { return mAlignment; }
		inline const auto& GetSizeInBytes()         const { return mSizeInBytes; }
		inline const auto& GetResourceDescVariant() const { return mResourceDescVariant; }
		inline const auto& GetTextureDesc()         const { return std::get<TextureDesc>(mResourceDescVariant); }
		inline const auto& GetBufferDesc()          const { return std::get<BufferDesc>(mResourceDescVariant); }

	private:
		void ResolveD3DResourceDesc();

		void QueryResourceAllocationInfo();

	private:
		const GHL::Device* mDevice{ nullptr };
		D3D12_RESOURCE_DESC mD3DResourceDesc{};

		ResourceDescVariant mResourceDescVariant{};
		size_t mAlignment{ 0u };
		size_t mSizeInBytes{ 0u };
	};
}