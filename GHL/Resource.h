#pragma once
#include "pbh.h"
#include "D3DObject.h"

namespace GHL {

	class Resource : public D3DObject {
	public:
		Resource() = default;
		Resource(const Resource& other) = delete;
		Resource(Resource&& other) = default;
		virtual ~Resource() = default;

		Resource& operator=(const Resource& other) = delete;
		Resource& operator=(Resource&& other) = default;

		/*
		* Get方法
		*/
		inline const auto  D3DResource()       const { return mResource.Get(); }
		inline const auto& GetResourceDesc()   const { return mResourceDesc; }
		inline const auto& GetInitialStates()  const { return mInitialStates; }
		inline const auto& GetExpectedStates() const { return mExpectedStates; }
		inline const auto& GetGpuAddress()     const { return mResource->GetGPUVirtualAddress(); }

		/*
		* 设置调试名称
		*/
		void SetDebugName(const std::string& name) override;

		/*
		* 解算D3D12_RESOURCE_DESC
		*/
		virtual void ResolveResourceDesc() = 0;

		/*
		* 创建描述符
		*/
		virtual void CreateDescriptor() = 0;

	protected:
		D3D12_RESOURCE_DESC mResourceDesc{};
		Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
		size_t mResourceSizeInBytes{ 0u }; // 总字节大小
		D3D12_RESOURCE_STATES mInitialStates = D3D12_RESOURCE_STATE_COMMON;
		D3D12_RESOURCE_STATES mExpectedStates = D3D12_RESOURCE_STATE_COMMON;
	};

}