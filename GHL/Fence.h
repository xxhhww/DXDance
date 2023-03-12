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
		* 使CPU期望的值加一
		*/
		uint64_t IncrementExpectedValue();

		/*
		* GPU是否跑完CPU期望的任务
		*/
		bool IsCompleted() const;

		/*
		* 设置完成事件
		*/
		void SetCompletionEvent(uint64_t value, HANDLE eventHandle);

		/*
		* Get方法
		*/
		inline const auto D3DFence()       const { return mFence.Get(); }
		inline const auto ExpectedValue()  const { return mExpectedValue; }
		inline const auto CompletedValue() const { return mFence->GetCompletedValue(); }

		/*
		* 设置调试名称
		*/
		void SetDebugName(const std::string& name) override;
	
	private:
		const Device* mDevice{ nullptr };
		Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
		uint64_t mExpectedValue{ 0u };
	};
}