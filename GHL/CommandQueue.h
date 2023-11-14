#pragma once
#include "pbh.h"
#include "D3DObject.h"
#include "Device.h"
#include "Fence.h"
#include <optional>

namespace GHL {
	
	class CommandQueue : public D3DObject {
	public:
		CommandQueue(const Device* device, D3D12_COMMAND_LIST_TYPE type);
		virtual ~CommandQueue() = default;

		/*
		* ��GPU�����в���Fence��Signal����
		*/
		void SignalFence(const Fence& fence, std::optional<uint64_t> expectedValue = std::nullopt);

		/*
		* ��GPU�����в���ȴ�Fence�Ĳ���
		*/
		void WaitFence(const Fence& fence, std::optional<uint64_t> expectedValue = std::nullopt);

		/*
		* ִ��GPU����
		*/
		void ExecuteCommandList(ID3D12GraphicsCommandList4* const commandList);

		/*
		* ִ�ж��GPU����
		*/
		void ExecuteCommandLists(ID3D12GraphicsCommandList4* const commandLists, UINT count);

		/*
		* ���
		*/
		uint64_t GetTimestampFrequency() const;

		/*
		* ���õ�������
		*/
		void SetDebugName(const std::string& name) override;

		/*
		* ��ȡD3DObject�ĵ�������
		*/
		const std::string& GetDebugName() override;

		inline const auto* GetDevice()       const { return mDevice; }
		inline const auto& GetQueueType()    const { return mType; }
		inline auto* D3DCommandQueue() const { return mQueue.Get(); }

	private:
		const Device* mDevice{ nullptr };
		std::string mName;
		D3D12_COMMAND_LIST_TYPE mType;
		D3D12_COMMAND_QUEUE_DESC mDesc{};
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> mQueue;
	};

	class GraphicsQueue : public CommandQueue {
	public:
		GraphicsQueue(const Device* device);
	};

	class ComputeQueue : public CommandQueue {
	public:
		ComputeQueue(const Device* device);
	};

	class CopyQueue : public CommandQueue {
	public:
		CopyQueue(const Device* device);
	};

}