#include "DescriptorTableRange.h"

namespace GHL {
	DescriptorTableRange::DescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE rangeType, uint32_t numDescriptors, uint32_t baseShaderRegister, uint32_t shaderSpace) {
		mRange.RangeType = rangeType;
		mRange.NumDescriptors = numDescriptors;
		mRange.BaseShaderRegister = baseShaderRegister;
		mRange.RegisterSpace = shaderSpace;
		mRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		mRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

		if (mRange.RangeType != D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
			mRange.Flags |= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
		}
	}

	CBDescriptorTableRange::CBDescriptorTableRange(uint32_t baseShaderRegister, uint32_t shaderSpace, uint32_t numDescriptors)
	: DescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, numDescriptors, baseShaderRegister, shaderSpace) {}

	SRDescriptorTableRange::SRDescriptorTableRange(uint32_t baseShaderRegister, uint32_t shaderSpace, uint32_t numDescriptors)
	: DescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numDescriptors, baseShaderRegister, shaderSpace) {}

	UADescriptorTableRange::UADescriptorTableRange(uint32_t baseShaderRegister, uint32_t shaderSpace, uint32_t numDescriptors)
	: DescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, numDescriptors, baseShaderRegister, shaderSpace) {}

	SamplerDescriptorRange::SamplerDescriptorRange(uint32_t baseShaderRegister, uint32_t shaderSpace, uint32_t numDescriptors)
	: DescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, numDescriptors, baseShaderRegister, shaderSpace) {}
}