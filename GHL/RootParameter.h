#pragma once
#include "pbh.h"
#include "DescriptorTableRange.h"
#include <vector>

namespace GHL {
	/*
	* 根参数
	*/
	class RootParameter {
	public:
		/*
		* 根参数在根签名中占据的位置，这是为了防止根参数的的位置发生冲突
		*/
		struct OccupationInSignature {
			uint32_t shaderRegister;	// 根签名注册位置
			uint32_t registerSpace;		// 根签名注册空间
			uint32_t numDescriptors;	// 根签名中描述符的个数(一个根常数也算作一个描述符)
		};

	public:
		RootParameter(D3D12_ROOT_PARAMETER_TYPE parameterType);
		virtual ~RootParameter() = default;

		inline const auto& D3DParameter()  const { return mParameter; }
		inline const auto& GetOccupation() const { return mOccupations; }

	protected:
		void AddOccupation(OccupationInSignature&& occupation);
	
	protected:
		D3D12_ROOT_PARAMETER1 mParameter;
		std::vector<OccupationInSignature> mOccupations;
	};

	/*
	* 根参数(常数)
	*/
	class RootConstantsParameter : public RootParameter {
	public:
		RootConstantsParameter(uint16_t numberOf32BitValues, uint16_t shaderRegister, uint16_t registerSpace);
	};

	/*
	* 根参数(描述符)
	*/
	class RootDescriptorParameter : public RootParameter {
	public:
		RootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE parameterType, uint16_t shaderRegister, uint16_t registerSpace);
		virtual ~RootDescriptorParameter() = default;
	};

	/*
	* 根参数(CBV)
	*/
	class RootConstantBufferParameter : public RootDescriptorParameter {
	public:
		RootConstantBufferParameter(uint16_t shaderRegister, uint16_t registerSpace);
	};

	/*
	* 根参数(SRV)
	*/
	class RootShaderResourceParameter : public RootDescriptorParameter {
	public:
		RootShaderResourceParameter(uint16_t shaderRegister, uint16_t registerSpace);
	};

	/*
	* 根参数(UAV)
	*/
	class RootUnorderedAccessParameter : public RootDescriptorParameter {
	public:
		RootUnorderedAccessParameter(uint16_t shaderRegister, uint16_t registerSpace);
	};

	/*
	* 根参数(描述符表)
	*/
	class RootDescriptorTableParameter : public RootParameter {
	public:
		RootDescriptorTableParameter();

		/*
		* 添加 描述符表的Range
		*/
		void AddDescriptorTableRange(const DescriptorTableRange& range);
	private:
		std::vector<D3D12_DESCRIPTOR_RANGE1> mRanges;
	};
}