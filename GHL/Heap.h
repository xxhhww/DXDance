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
		Heap(const Device* device, size_t size, EResourceUsage usage);
		Heap(const Heap& other) = delete;
		Heap(Heap&& other) = default;
		~Heap() = default;

		Heap& operator=(const Heap& other) = delete;
		Heap& operator=(Heap&& other) = default;

		/*
		* Get����
		*/
		inline const auto& GetUsage()    const { return mUsage; }
		inline const auto& GetHeapDesc() const { return mDesc; }
		inline const auto  D3DHeap()     const { return mHeap.Get(); }
	
		/*
		* ���õ�������
		*/
		void SetDebugName(const std::string& name) override;

		/*
		* ��ȡD3DObject�ĵ�������
		*/
		const std::string& GetDebugName() override;

	private:
		const Device* mDevice{ nullptr };
		std::string mName;
		size_t mAlighnedSize;
		EResourceUsage mUsage;
		D3D12_HEAP_DESC mDesc{};
		Microsoft::WRL::ComPtr<ID3D12Heap> mHeap;
	};

}