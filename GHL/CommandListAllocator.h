#pragma once
#include "CommandList.h"
#include "Tools/Pool.h"
#include <optional>

namespace GHL {

	class CommandListAllocator {
	public:
		struct SlotUserData {
			std::optional<uint32_t> index = std::nullopt;
		};

		using Pool = Tool::Pool<SlotUserData>;

	public:
		CommandListAllocator(const Device* device);
		CommandListAllocator(const CommandListAllocator& other) = delete;
		CommandListAllocator(CommandListAllocator&& other) = default;
		CommandListAllocator& operator=(const CommandListAllocator& other) = delete;
		CommandListAllocator& operator=(CommandListAllocator&& other) = default;
		~CommandListAllocator() = default;

		ID3D12GraphicsCommandList4* GetGraphicsCommandList();

		ID3D12GraphicsCommandList4* GetComputeCommandList();

		ID3D12GraphicsCommandList4* GetCopyCommandList();

	private:
		const Device* mDevice{ nullptr };

		/*
		* GraphicsCommandList
		*/
		Pool mGraphicsPool;
		std::vector<CommandList> mGraphicsCommandLists;
		std::vector<CommandAllocator> mGraphicsCommandAllocators;

		/*
		* ComputeCommandList
		*/
		Pool mComputePool;
		std::vector<CommandList> mComputeCommandLists;
		std::vector<CommandAllocator> mComputeCommandAllocators;

		/*
		* CopyCommandList
		*/
		Pool mCopyPool;
		std::vector<CommandList> mCopyCommandLists;
		std::vector<CommandAllocator> mCopyCommandAllocators;
	};

}