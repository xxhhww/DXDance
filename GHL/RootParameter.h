#pragma once
#include "pbh.h"
#include "DescriptorTableRange.h"
#include <vector>

namespace GHL {
	/*
	* ������
	*/
	class RootParameter {
	public:
		/*
		* �������ڸ�ǩ����ռ�ݵ�λ�ã�����Ϊ�˷�ֹ�������ĵ�λ�÷�����ͻ
		*/
		struct OccupationInSignature {
			uint32_t shaderRegister;	// ��ǩ��ע��λ��
			uint32_t registerSpace;		// ��ǩ��ע��ռ�
			uint32_t numDescriptors;	// ��ǩ�����������ĸ���(һ��������Ҳ����һ��������)
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
	* ������(����)
	*/
	class RootConstantsParameter : public RootParameter {
	public:
		RootConstantsParameter(uint16_t numberOf32BitValues, uint16_t shaderRegister, uint16_t registerSpace);
	};

	/*
	* ������(������)
	*/
	class RootDescriptorParameter : public RootParameter {
	public:
		RootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE parameterType, uint16_t shaderRegister, uint16_t registerSpace);
		virtual ~RootDescriptorParameter() = default;
	};

	/*
	* ������(CBV)
	*/
	class RootConstantBufferParameter : public RootDescriptorParameter {
	public:
		RootConstantBufferParameter(uint16_t shaderRegister, uint16_t registerSpace);
	};

	/*
	* ������(SRV)
	*/
	class RootShaderResourceParameter : public RootDescriptorParameter {
	public:
		RootShaderResourceParameter(uint16_t shaderRegister, uint16_t registerSpace);
	};

	/*
	* ������(UAV)
	*/
	class RootUnorderedAccessParameter : public RootDescriptorParameter {
	public:
		RootUnorderedAccessParameter(uint16_t shaderRegister, uint16_t registerSpace);
	};

	/*
	* ������(��������)
	*/
	class RootDescriptorTableParameter : public RootParameter {
	public:
		RootDescriptorTableParameter();

		/*
		* ��� ���������Range
		*/
		void AddDescriptorTableRange(const DescriptorTableRange& range);
	private:
		std::vector<D3D12_DESCRIPTOR_RANGE1> mRanges;
	};
}