#pragma once
#include "pbh.h"
#include "D3DObject.h"
#include "Device.h"

namespace GHL {

	/*
	* ��Դ��
	*/
	class Heap : public D3DObject {
	public:
		Heap(const Device* device, EResourceUsage usage);
		~Heap() = default;

		/*
		* Get����
		*/
		inline const auto& GetHeapType() const { return mType; }
		inline const auto& GetHeapDesc() const { return mDesc; }
		inline const auto  D3DHeap()     const { return mHeap.Get(); }
	
		/*
		* ���õ�������
		*/
		void SetDebugName(const std::string& name) override;

	private:
		const Device* mDevice{ nullptr };
		D3D12_HEAP_TYPE mType;
		D3D12_HEAP_DESC mDesc{};
		Microsoft::WRL::ComPtr<ID3D12Heap> mHeap;
	};

}