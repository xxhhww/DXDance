#pragma once
#include "CommandAllocator.h"

namespace GHL {
	
	class CommandList : public D3DObject {
	public:
		CommandList(const Device* device, ID3D12CommandAllocator* allocator, D3D12_COMMAND_LIST_TYPE type);
		~CommandList() = default;
		
		inline const auto& GetListType()    const { return mType; }
		inline const auto  D3DCommandList() const { return mCommandList.Get(); }

		/*
		* 重置命令列表
		*/
		void Reset();

		/*
		* 关闭命令列表
		*/
		void Close();

		/*
		* 设置调试名称
		*/
		void SetDebugName(const std::string& name) override;

	private:
		const Device* mDevice{ nullptr };
		ID3D12CommandAllocator* mAllocator{ nullptr };
		D3D12_COMMAND_LIST_TYPE mType;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> mCommandList;
	};

}