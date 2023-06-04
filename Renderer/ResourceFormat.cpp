#include "ResourceFormat.h"
#include "Tools/VisitorHelper.h"
#include "Tools/Assert.h"
#include "Math/Helper.h"

namespace Renderer {

	ResourceFormat::ResourceFormat(const GHL::Device* device)
	: mDevice(device) {}

	ResourceFormat::ResourceFormat(const GHL::Device* device, const TextureDesc& desc)
	: mDevice(device)
	, mResourceDescVariant(desc) {
		Build();
	}

	ResourceFormat::ResourceFormat(const GHL::Device* device, const BufferDesc& desc)
	: mDevice(device)
	, mResourceDescVariant(desc) {
		Build();
	}

	void ResourceFormat::Build() {
		ResolveD3DResourceDesc();
		QueryResourceAllocationInfo();
	}

	void ResourceFormat::SetTextureDesc(const TextureDesc& desc) {
		mResourceDescVariant = desc;
	}

	void ResourceFormat::SetBufferDesc(const BufferDesc& desc) {
		mResourceDescVariant = desc;
	}

	bool ResourceFormat::IsBuffer()  const {
		bool isBuffer = false;
		std::visit(MakeVisitor(
			[&](const TextureDesc& desc) {
				isBuffer = false;
			},
			[&](const BufferDesc& desc) {
				isBuffer = true;
			})
			, mResourceDescVariant);
		return isBuffer;
	}

	bool ResourceFormat::IsTexture() const {
		bool isTexture = false;
		std::visit(MakeVisitor(
			[&](const TextureDesc& desc) {
				isTexture = true;
			},
			[&](const BufferDesc& desc) {
				isTexture = false;
			})
			, mResourceDescVariant);
		return isTexture;
	}

	void ResourceFormat::SetBackBufferStates() {
		mInitialState = GHL::EResourceState::Present;
		mExpectedState = (GHL::EResourceState::Present | GHL::EResourceState::RenderTarget);
	}

	void ResourceFormat::ResolveD3DResourceDesc() {
		// 解算D3D12_RESOURCE_DESC
		std::visit(MakeVisitor(
			[this](TextureDesc& desc) {
				mFormat = desc.format;
				mUsage = desc.usage;
				mInitialState = desc.initialState;
				mExpectedState = desc.expectedState;

				mResourceDesc.Dimension = GHL::GetD3DTextureDimension(desc.dimension);
				mResourceDesc.Format = desc.format;
				mResourceDesc.MipLevels = desc.mipLevals;
				mResourceDesc.Alignment = 0u;
				mResourceDesc.DepthOrArraySize = (desc.dimension == GHL::ETextureDimension::Texture3D) ? desc.depth : desc.arraySize;
				mResourceDesc.Height = desc.height;
				mResourceDesc.Width = desc.width;
				mResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
				mResourceDesc.SampleDesc.Count = desc.sampleCount;
				mResourceDesc.SampleDesc.Quality = 0u;

				if (desc.createdMethod == GHL::ECreatedMethod::Reserved) {
					mResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
				}
				else {
					mResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				}

				if (HasAllFlags(desc.expectedState, GHL::EResourceState::RenderTarget)) {
					mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
				}

				if (HasAllFlags(desc.expectedState, GHL::EResourceState::DepthRead) ||
					HasAllFlags(desc.expectedState, GHL::EResourceState::DepthWrite)) {
					mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

					if (!HasAllFlags(desc.expectedState, GHL::EResourceState::PixelShaderAccess) &&
						!HasAllFlags(desc.expectedState, GHL::EResourceState::NonPixelShaderAccess)) {
						mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
					}
				}

				if (HasAllFlags(desc.expectedState, GHL::EResourceState::UnorderedAccess)) {
					mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				}
			},
			[this](BufferDesc& desc) {
				if (desc.usage == GHL::EResourceUsage::Default) {
					if (HasAllFlags(desc.miscFlag, GHL::EBufferMiscFlag::StructuredBuffer)) {
						desc.size = Math::AlignUp(desc.size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);;
					}
					/*
					if (HasAllFlags(desc.expectedState, GHL::EResourceState::RaytracingAccelerationStructure)) {
						desc.initialState |= GHL::EResourceState::RaytracingAccelerationStructure;
					}
					else if (HasAllFlags(desc.expectedState, GHL::EResourceState::IndirectArgument)) {
						desc.initialState |= GHL::EResourceState::IndirectArgument;
					}
					*/
				}
				else if (desc.usage == GHL::EResourceUsage::Upload) {
					desc.initialState |= GHL::EResourceState::GenericRead;
				}
				else if (desc.usage == GHL::EResourceUsage::ReadBack) {
					desc.initialState |= GHL::EResourceState::CopyDestination;
				}
				else {
					ASSERT_FORMAT(false, "Unsupport Resource Usage");
				}

				if (desc.createdMethod == GHL::ECreatedMethod::Placed) {
					// Placed Resource 需要 64k字节对齐
					desc.size = Math::AlignUp(desc.size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
				}

				if (HasAllFlags(desc.miscFlag, GHL::EBufferMiscFlag::ConstantBuffer)) {
					// ConstantBuffer需要字节对齐
					desc.size = Math::AlignUp(desc.size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
				}

				mFormat = desc.format;
				mUsage = desc.usage;
				mInitialState = desc.initialState;
				mExpectedState = desc.expectedState;

				mResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				mResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
				mResourceDesc.MipLevels = 1u;
				mResourceDesc.Alignment = 0u;
				mResourceDesc.DepthOrArraySize = 1u;
				mResourceDesc.Height = 1u;
				mResourceDesc.Width = desc.size;
				mResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
				mResourceDesc.SampleDesc.Count = 1u;
				mResourceDesc.SampleDesc.Quality = 0u;
				mResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

				if (HasAllFlags(desc.expectedState, GHL::EResourceState::UnorderedAccess)) {
					mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				}
				if (!HasAllFlags(desc.expectedState, GHL::EResourceState::PixelShaderAccess) &&
					!HasAllFlags(desc.expectedState, GHL::EResourceState::NonPixelShaderAccess)) {
					mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
				}
			})
			, mResourceDescVariant);
	}

	void ResourceFormat::QueryResourceAllocationInfo() {
		if (mResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && mResourceDesc.Format == DXGI_FORMAT_UNKNOWN) {
			mAlignment = mResourceDesc.Alignment;
			mSizeInBytes = mResourceDesc.Width;
		}
		else {
			// 查询ResourceDesc需求的显存
			D3D12_RESOURCE_ALLOCATION_INFO allocInfo = mDevice->D3DDevice()->GetResourceAllocationInfo(mDevice->GetNodeMask(), 1, &mResourceDesc);
			mAlignment = allocInfo.Alignment;
			mSizeInBytes = allocInfo.SizeInBytes;

			mResourceDesc.Alignment = mAlignment;
		}
	}

	uint32_t ResourceFormat::SubresourceCount() const {
		uint32_t subresourceCount{ 0u };

		std::visit(MakeVisitor(
			[&](const TextureDesc& desc) {
				subresourceCount = desc.mipLevals;
			},
			[&](const BufferDesc& desc) {
				subresourceCount = 1u;
			})
			, mResourceDescVariant);

		return subresourceCount;
	}

}