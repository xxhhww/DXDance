#pragma once
#include "pbh.h"
#include "Device.h"

namespace GHL {

	class CommandAllocator : public D3DObject {
	public:
		CommandAllocator(const Device* device, D3D12_COMMAND_LIST_TYPE type);
		CommandAllocator(const CommandAllocator& other) = delete;
		CommandAllocator(CommandAllocator&& other) = default;

		CommandAllocator& operator=(const CommandAllocator& other) = delete;
		CommandAllocator& operator=(CommandAllocator&& other) = default;

		~CommandAllocator() = default;

		/*
		* ����CommandAllocator
		*/
		void Reset();


		inline const auto& GetCommandType()      const { return mType; }
		inline const auto  D3DCommandAllocator() const { return mAllocator.Get(); }

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
		D3D12_COMMAND_LIST_TYPE mType;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mAllocator;
	};

}