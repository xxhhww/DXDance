#pragma once
#include "pbh.h"
#include "D3DObject.h"
#include "Device.h"

namespace GHL {
	class Fence : public D3DObject {
	public:
		Fence(const Device* device);
		~Fence() = default;

		/*
		* ʹCPU������ֵ��һ
		*/
		uint64_t IncrementExpectedValue();

		/*
		* GPU�Ƿ�����CPU����������
		*/
		bool IsCompleted() const;

		/*
		* ��������¼�
		*/
		void SetCompletionEvent(uint64_t value, HANDLE eventHandle);

		/*
		* Get����
		*/
		inline const auto D3DFence()       const { return mFence.Get(); }
		inline const auto ExpectedValue()  const { return mExpectedValue; }
		inline const auto CompletedValue() const { return mFence->GetCompletedValue(); }

		/*
		* ���õ�������
		*/
		void SetDebugName(const std::string& name) override;
	
	private:
		const Device* mDevice{ nullptr };
		Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
		uint64_t mExpectedValue{ 0u };
	};
}