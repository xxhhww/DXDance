#include "RootSignature.h"
#include "Device.h"
#include "Tools/StrUtil.h"
#include "Tools/Assert.h"

namespace GHL {
	RootSignature::RootSignature(const Device* device) 
	: mDevice(device) {}

	void RootSignature::AddRootConstantsParameter(const RootConstantsParameter& parameter) {
		if (VerifyParameterConflict(parameter)) {
			mParameters.push_back(parameter.D3DParameter());
		}
	}

	void RootSignature::AddRootDescriptorParameter(const RootDescriptorParameter& parameter) {
		if (VerifyParameterConflict(parameter)) {
			mParameters.push_back(parameter.D3DParameter());
		}
	}

	void RootSignature::AddRootDescriptorTableParameter(const RootDescriptorTableParameter& parameter) {
		if (VerifyParameterConflict(parameter)) {
			mParameters.push_back(parameter.D3DParameter());
		}
	}

	void RootSignature::AddStaticSampler(D3D12_STATIC_SAMPLER_DESC staticSamplerDesc) {
		mStaticSamplers.push_back(staticSamplerDesc);
	}

	void RootSignature::Compile() {
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData{};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		HRASSERT(mDevice->D3DDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)));

		// 两个DIRECTLY_INDEXED标识符分别启用HLSL中的ResourceDescriptorHeap 与 SamplerDescriptorHeap
		mDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		mDesc.Desc_1_1.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
			D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

		mDesc.Desc_1_1.NumParameters = (UINT)mParameters.size();
		mDesc.Desc_1_1.pParameters = mParameters.data();

		mDesc.Desc_1_1.NumStaticSamplers = (UINT)mStaticSamplers.size();
		mDesc.Desc_1_1.pStaticSamplers = mStaticSamplers.data();

		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errors;
		HRASSERT(D3D12SerializeVersionedRootSignature(&mDesc, &signatureBlob, &errors));
		ASSERT_FORMAT(!errors, (char*)errors->GetBufferPointer());
		HRASSERT(mDevice->D3DDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&mSignature)));
	}

	void RootSignature::SetDebugName(const std::string& name) {
		if (mSignature) {
			mSignature->SetName(Tool::StrUtil::UTF8ToWString(name).c_str());
		}
	}

	bool RootSignature::VerifyParameterConflict(const RootParameter& parameter) {
		const auto& target = parameter.GetOccupation();
		for (const auto& item : target) {

			for (const auto& occ : mOccupations) {
				if (occ.registerSpace == item.registerSpace) {
					uint32_t segMin = occ.shaderRegister;
					uint32_t segMax = segMin + occ.numDescriptors;

					//TODO 两个线段相交判断
				}
			}
		}

		return true;
	}
}