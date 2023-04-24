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

	void RootSignature::InitStaticSampler() {
		const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
			0, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW
		AddStaticSampler(pointWrap);

		const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
			1, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			0.0f, 16u, D3D12_COMPARISON_FUNC_ALWAYS); // addressW
		AddStaticSampler(pointClamp);

		const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
			2, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW
		AddStaticSampler(linearWrap);

		const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
			3, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW
		AddStaticSampler(linearClamp);

		const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
			4, // shaderRegister
			D3D12_FILTER_ANISOTROPIC, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
			0.0f,                             // mipLODBias
			8);                               // maxAnisotropy
		AddStaticSampler(anisotropicWrap);

		const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
			5, // shaderRegister
			D3D12_FILTER_ANISOTROPIC, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
			0.0f,                              // mipLODBias
			8);                                // maxAnisotropy
		AddStaticSampler(anisotropicClamp);
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