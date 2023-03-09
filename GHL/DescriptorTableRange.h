#pragma once
#include "pbh.h"

namespace GHL {
	class DescriptorTableRange {
	public:
		DescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE rangeType, uint32_t numDescriptors, uint32_t baseShaderRegister, uint32_t shaderSpace);
		virtual ~DescriptorTableRange() = default;

		inline const auto& D3DDescriptorRange() const { return mRange; }
	protected:
		D3D12_DESCRIPTOR_RANGE1 mRange;
	};

	class CBDescriptorTableRange : public DescriptorTableRange {
	public:
		CBDescriptorTableRange(uint32_t baseShaderRegister, uint32_t shaderSpace, uint32_t numDescriptors = UINT_MAX);
	};

	class SRDescriptorTableRange : public DescriptorTableRange {
	public:
		SRDescriptorTableRange(uint32_t baseShaderRegister, uint32_t shaderSpace, uint32_t numDescriptors = UINT_MAX);
	};

	class UADescriptorTableRange : public DescriptorTableRange {
	public:
		UADescriptorTableRange(uint32_t baseShaderRegister, uint32_t shaderSpace, uint32_t numDescriptors = UINT_MAX);
	};

	class SamplerDescriptorRange : public DescriptorTableRange {
	public:
		SamplerDescriptorRange(uint32_t baseShaderRegister, uint32_t shaderSpace, uint32_t numDescriptors = UINT_MAX);
	};
}