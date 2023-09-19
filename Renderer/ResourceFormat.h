#pragma once
#include "GHL/pbh.h"
#include "GHL/Device.h"

#include <variant>

namespace Renderer {

	/*
	* ��������
	*/
	struct TextureDesc {
		GHL::ETextureDimension dimension = GHL::ETextureDimension::Texture2D; // �����ά��
		uint32_t               width = 0u;
		uint32_t               height = 0u;
		uint32_t               depth = 1u;	// ������ά������άʱ������depth���
		uint32_t               arraySize = 1u;
		uint32_t               mipLevals = 1u;
		uint32_t               sampleCount = 1u;
		DXGI_FORMAT            format = DXGI_FORMAT_UNKNOWN;
		GHL::EResourceUsage    usage = GHL::EResourceUsage::Default;	// ����Ĭ����Default���Ͻ��д���
		GHL::ETextureMiscFlag  miscFlag = GHL::ETextureMiscFlag::None;
		GHL::ClearValue        clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
		GHL::EResourceState    initialState = GHL::EResourceState::Common;
		GHL::EResourceState    expectedState = GHL::EResourceState::Common;
		GHL::ECreatedMethod    createdMethod = GHL::ECreatedMethod::Committed;
	};

	/*
	* ��������Դ����
	*/
	struct TextureSubResourceDesc {
	public:
		bool operator==(const TextureSubResourceDesc& other) const {
			return firstSlice == other.firstSlice && sliceCount == other.sliceCount && firstMip == other.firstMip && other.mipCount == other.mipCount;
		}

	public:
		uint32_t firstSlice = 0u;
		uint32_t sliceCount = static_cast<uint32_t>(-1); // -1 ��ʾȫ��
		uint32_t firstMip = 0u;
		uint32_t mipCount = static_cast<uint32_t>(-1); // -1 ��ʾȫ��
	};

	struct TextureSubResourceDescHashFunc {
	public:
		std::size_t operator()(const TextureSubResourceDesc& desc) const {
			return std::hash<uint32_t>()(desc.firstSlice) ^ std::hash<uint32_t>()(desc.sliceCount) ^ std::hash<uint32_t>()(desc.firstMip) ^ std::hash<uint32_t>()(desc.mipCount);
		}
	};

	/*
	* ��������
	*/
	struct BufferDesc {
		uint32_t               stride = 1u;
		size_t                 size = 0u;
		DXGI_FORMAT            format = DXGI_FORMAT_UNKNOWN;
		GHL::EResourceUsage    usage = GHL::EResourceUsage::Upload;
		GHL::EBufferMiscFlag   miscFlag = GHL::EBufferMiscFlag::None;
		GHL::EResourceState    initialState = GHL::EResourceState::Common;
		GHL::EResourceState    expectedState = GHL::EResourceState::Common;
		GHL::ECreatedMethod    createdMethod = GHL::ECreatedMethod::Committed;
	};

	/*
	* ��������Դ����
	*/
	struct BufferSubResourceDesc {
	public:
		bool operator==(const BufferSubResourceDesc& other) const {
			return offset == other.offset && size == other.size;
		}

	public:
		size_t offset = 0u;
		size_t size = static_cast<size_t>(-1);
	};

	struct BufferSubResourceDescHashFunc {
		std::size_t operator()(const BufferSubResourceDesc& desc) const {
			return std::hash<size_t>()(desc.offset) ^ std::hash<size_t>()(desc.size);
		}
	};

	using ResourceDescVariant = std::variant<TextureDesc, BufferDesc>;

	/*
	* ��Ҫ��Device����ѯ������Դ�Ĵ�С��������˷�װ
	*/
	class ResourceFormat {
	public:
		ResourceFormat() = default;
		ResourceFormat(const GHL::Device* device);
		ResourceFormat(const GHL::Device* device, const TextureDesc& desc);
		ResourceFormat(const GHL::Device* device, const BufferDesc& desc);

		void Build();

		void SetTextureDesc(const TextureDesc& desc);
		void SetBufferDesc(const BufferDesc& desc);

		bool IsBuffer()  const;
		bool IsTexture() const;

		bool CanUseClearValue();

		inline const auto& D3DResourceDesc() const { return mResourceDesc; }

		inline const auto& GetAlignment()              const { return mAlignment; }
		inline const auto& GetSizeInBytes()            const { return mSizeInBytes; }
		inline const auto& GetResourceDescVariant()    const { return mResourceDescVariant; }
		inline const auto& GetTextureDesc()            const { return std::get<TextureDesc>(mResourceDescVariant); }
		inline const auto& GetBufferDesc()             const { return std::get<BufferDesc>(mResourceDescVariant); }
		inline const auto& GetInitialState()           const { return mInitialState; }
		inline const auto& GetExpectedState()          const { return mExpectedState; }

		/*
		* Get ClearColor
		*/
		inline const auto& GetColorClearValue()        const { return std::get<GHL::ColorClearValue>(GetTextureDesc().clearVaule); }
		inline const auto& GetDepthStencilClearValue() const { return std::get<GHL::DepthStencilClearValue>(GetTextureDesc().clearVaule); }

		uint32_t SubresourceCount() const;

		/*
		* ΪBackBuffer������Դ״̬
		*/
		void SetBackBufferStates();

	private:
		void ResolveD3DResourceDesc();

		void QueryResourceAllocationInfo();

	private:
		const GHL::Device* mDevice{ nullptr };
		D3D12_RESOURCE_DESC mResourceDesc{};

		ResourceDescVariant mResourceDescVariant{};

		// ����Դ����������ȡ������ͨ����Ϣ
		DXGI_FORMAT			mFormat{ DXGI_FORMAT_UNKNOWN };
		GHL::EResourceUsage mUsage{ GHL::EResourceUsage::Default };
		GHL::EResourceState mInitialState{ GHL::EResourceState::Common };
		GHL::EResourceState mExpectedState{ GHL::EResourceState::Common };

		size_t mAlignment{ 0u };
		size_t mSizeInBytes{ 0u };
		size_t mRequiredSize{ 0u };
	};
}