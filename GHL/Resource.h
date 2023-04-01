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
		inline auto* D3DResource()   const { return mResource.Get(); }
		inline const auto& GetGpuAddress() const { return mResource->GetGPUVirtualAddress(); }

		/*
		* 设置调试名称
		*/
		void SetDebugName(const std::string& name) override;

		/*
		* 创建描述符
		*/
		virtual void CreateDescriptor() = 0;

	protected:
		Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
	};

}