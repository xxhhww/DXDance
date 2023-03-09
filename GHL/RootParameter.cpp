#include "RootParameter.h"
#include "Tools/Assert.h"

namespace GHL {
	RootParameter::RootParameter(D3D12_ROOT_PARAMETER_TYPE parameterType) {
		mParameter.ParameterType = parameterType;
		mParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		mParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
	}

	void RootParameter::AddOccupation(OccupationInSignature&& occupation) {
		mOccupations.emplace_back(std::forward<OccupationInSignature>(occupation));
	}

	RootConstantsParameter::RootConstantsParameter(uint16_t numberOf32BitValues, uint16_t shaderRegister, uint16_t registerSpace)
	: RootParameter(D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) {
		mParameter.Constants.Num32BitValues = numberOf32BitValues;
		mParameter.Constants.ShaderRegister = shaderRegister;
		mParameter.Constants.RegisterSpace = registerSpace;

		AddOccupation({ shaderRegister, registerSpace, 1 });
	}

	RootDescriptorParameter::RootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE parameterType, uint16_t shaderRegister, uint16_t registerSpace)
	: RootParameter(parameterType) {
		mParameter.Descriptor.ShaderRegister = shaderRegister;
		mParameter.Descriptor.RegisterSpace = registerSpace;
		mParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;

		AddOccupation({ shaderRegister, registerSpace, 1 });
	}

	RootConstantBufferParameter::RootConstantBufferParameter(uint16_t shaderRegister, uint16_t registerSpace)
	: RootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, shaderRegister, registerSpace) {}

	RootShaderResourceParameter::RootShaderResourceParameter(uint16_t shaderRegister, uint16_t registerSpace)
	: RootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, shaderRegister, registerSpace) {}

	RootUnorderedAccessParameter::RootUnorderedAccessParameter(uint16_t shaderRegister, uint16_t registerSpace)
	: RootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_UAV, shaderRegister, registerSpace) {}

	RootDescriptorTableParameter::RootDescriptorTableParameter()
	: RootParameter(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {}


	void RootDescriptorTableParameter::AddDescriptorTableRange(const DescriptorTableRange& range) {
		mRanges.push_back(range.D3DDescriptorRange());
		mParameter.DescriptorTable.pDescriptorRanges = &mRanges[0];
		mParameter.DescriptorTable.NumDescriptorRanges = (UINT)mRanges.size();

		AddOccupation({
			range.D3DDescriptorRange().BaseShaderRegister,
			range.D3DDescriptorRange().RegisterSpace,
			range.D3DDescriptorRange().NumDescriptors });
	}
}