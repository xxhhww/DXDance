#pragma once
#include "pbh.h"
#include "D3DObject.h"

namespace GHL {

	class GpuDevice;

	class Resource : public D3DObject {
	public:
		Resource(GpuDevice* gpuDevice);
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

		/*
		* 设置调试名称
		*/
		void SetDebugName(const std::string& name) override;

	protected:
		GpuDevice* mGpuDevice{ nullptr };
		D3D12_RESOURCE_DESC mResourceDesc{};
		Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
		D3D12_RESOURCE_STATES mInitialStates = D3D12_RESOURCE_STATE_COMMON;
		D3D12_RESOURCE_STATES mExpectedStates = D3D12_RESOURCE_STATE_COMMON;
	};

}