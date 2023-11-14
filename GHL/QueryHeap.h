#pragma once
#include "pbh.h"
#include "D3DObject.h"
#include "Device.h"

namespace GHL {

	class QueryHeap : public D3DObject {
	public:
		QueryHeap(const Device* device, uint64_t size, EQueryHeapType queryHeapType);
		QueryHeap(const QueryHeap& other) = delete;
		QueryHeap(QueryHeap&& other) = default;
		~QueryHeap() = default;

		QueryHeap& operator=(const QueryHeap& other) = delete;
		QueryHeap& operator=(QueryHeap&& other) = default;

		/*
		* Get方法
		*/
		inline const auto& GetSize()          const { return mSize; }
		inline const auto& GetQueryType()     const { return mQueryType; }
		inline const auto& GetQueryHeapType() const { return mQueryHeapType; }
		inline const auto& GetQueryDesc()     const { return mDesc; }
		inline const auto  D3DQueryHeap()     const { return mHeap.Get(); }

		/*
		* 设置调试名称
		*/
		void SetDebugName(const std::string& name) override;

		/*
		* 获取D3DObject的调试名称
		*/
		const std::string& GetDebugName() override;

	private:
		const Device* mDevice{ nullptr };
		std::string mName;
		uint64_t mSize;
		D3D12_QUERY_TYPE      mQueryType;
		D3D12_QUERY_HEAP_TYPE mQueryHeapType;
		D3D12_QUERY_HEAP_DESC mDesc{};
		Microsoft::WRL::ComPtr<ID3D12QueryHeap> mHeap;
	};
}