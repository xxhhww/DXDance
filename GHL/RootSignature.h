#pragma once
#include "pbh.h"
#include "D3DObject.h"
#include "RootParameter.h"

namespace GHL {
	class Device;

	class RootSignature : public D3DObject {
	public:
		RootSignature(const Device* device);
		~RootSignature() = default;

		/*
		* 添加根常数
		*/
		void AddRootConstantsParameter(const RootConstantsParameter& parameter);

		/*
		* 添加根描述符
		*/
		void AddRootDescriptorParameter(const RootDescriptorParameter& parameter);
		
		/*
		* 添加根描述符表
		*/
		void AddRootDescriptorTableParameter(const RootDescriptorTableParameter& parameter);

		/*
		* 添加静态根采样器
		*/
		void AddStaticSampler(D3D12_STATIC_SAMPLER_DESC staticSamplerDesc);

		/*
		* 编译
		*/
		void Compile();

		inline auto* D3DRootSignature() const { return mSignature.Get(); }

		/*
		* 设置调试名称
		*/
		void SetDebugName(const std::string& name) override;

	protected:
		/*
		* 验证根参数是否冲突
		*/
		bool VerifyParameterConflict(const RootParameter& parameter);

	private:
		bool mCompiled{ false };
		const Device* mDevice{ nullptr };
		std::vector<D3D12_STATIC_SAMPLER_DESC> mStaticSamplers;
		std::vector<D3D12_ROOT_PARAMETER1> mParameters;
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC mDesc{};
		Microsoft::WRL::ComPtr<ID3D12RootSignature> mSignature;
		
		// 记录根参数的占用情况
		std::vector<RootParameter::OccupationInSignature> mOccupations;
	};
}