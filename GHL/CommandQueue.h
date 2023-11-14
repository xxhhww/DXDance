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
		* 向GPU队列中插入Fence的Signal操作
		*/
		void SignalFence(const Fence& fence, std::optional<uint64_t> expectedValue = std::nullopt);

		/*
		* 向GPU队列中插入等待Fence的操作
		*/
		void WaitFence(const Fence& fence, std::optional<uint64_t> expectedValue = std::nullopt);

		/*
		* 执行GPU命令
		*/
		void ExecuteCommandList(ID3D12GraphicsCommandList4* const commandList);

		/*
		* 执行多个GPU命令
		*/
		void ExecuteCommandLists(ID3D12GraphicsCommandList4* const commandLists, UINT count);

		/*
		* 获得
		*/
		uint64_t GetTimestampFrequency() const;

		/*
		* 设置调试名称
		*/
		void SetDebugName(const std::string& name) override;

		/*
		* 获取D3DObject的调试名称
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