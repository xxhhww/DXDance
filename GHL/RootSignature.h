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
		* ��Ӹ�����
		*/
		void AddRootConstantsParameter(const RootConstantsParameter& parameter);

		/*
		* ��Ӹ�������
		*/
		void AddRootDescriptorParameter(const RootDescriptorParameter& parameter);
		
		/*
		* ��Ӹ���������
		*/
		void AddRootDescriptorTableParameter(const RootDescriptorTableParameter& parameter);

		/*
		* ��Ӿ�̬��������
		*/
		void AddStaticSampler(D3D12_STATIC_SAMPLER_DESC staticSamplerDesc);

		/*
		* ����
		*/
		void Compile();

		inline auto* D3DRootSignature() const { return mSignature.Get(); }

		/*
		* ���õ�������
		*/
		void SetDebugName(const std::string& name) override;

	protected:
		/*
		* ��֤�������Ƿ��ͻ
		*/
		bool VerifyParameterConflict(const RootParameter& parameter);

	private:
		bool mCompiled{ false };
		const Device* mDevice{ nullptr };
		std::vector<D3D12_STATIC_SAMPLER_DESC> mStaticSamplers;
		std::vector<D3D12_ROOT_PARAMETER1> mParameters;
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC mDesc{};
		Microsoft::WRL::ComPtr<ID3D12RootSignature> mSignature;
		
		// ��¼��������ռ�����
		std::vector<RootParameter::OccupationInSignature> mOccupations;
	};
}