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
		inline auto*       D3DResource()         { return mD3DResource.Get(); }
		inline const auto* D3DResource()   const { return mD3DResource.Get(); }
		inline const auto& GetGpuAddress() const { return mD3DResource->GetGPUVirtualAddress(); }

		/*
		* 设置调试名称
		*/
		void SetDebugName(const std::string& name) override;

		/*
		* 获取调试名称
		*/
		const std::string& GetDebugName() override;

		/*
		* 创建描述符
		*/
		virtual void CreateDescriptor() = 0;

	protected:
		std::string mName;
		Microsoft::WRL::ComPtr<ID3D12Resource> mD3DResource;
	};

}