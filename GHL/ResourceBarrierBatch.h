#pragma once
#include "pbh.h"
#include <vector>

namespace GHL {
	/*
	* 资源屏障
	*/
	class ResourceBarrier {
	public:
		ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE barrierType);

		/*
		* Get方法
		*/
		inline const auto& D3DBarrier() const { return mBarrier; }

	protected:
		D3D12_RESOURCE_BARRIER mBarrier{};
	};


	/*
	* 资源转换屏障
	*/
	class TransitionBarrier : public ResourceBarrier {
	public:
		TransitionBarrier(
			ID3D12Resource* resource,
			D3D12_RESOURCE_STATES stateBefore,
			D3D12_RESOURCE_STATES stateAfter,
			UINT subResources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	};

	/*
	* Split资源转换屏障
	*/
	class SplitTransitionBarrier : public ResourceBarrier {
	public:
		SplitTransitionBarrier(
			ID3D12Resource* resource,
			D3D12_RESOURCE_STATES stateBefore,
			D3D12_RESOURCE_STATES stateAfter,
			bool endFlag,
			UINT subResources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	};

	/*
	* UAV屏障
	*/
	class UAVBarrier : public ResourceBarrier {
	public:
		UAVBarrier(ID3D12Resource* resource);
	};

	/*
	* 别名屏障
	*/
	class AliasingBarrier : public ResourceBarrier {
	public:
		AliasingBarrier(ID3D12Resource* resourceBefore, ID3D12Resource* resourceAfter);
	};


	/*
	* 资源屏障的Batch
	*/
	class ResourceBarrierBatch {
	public:
		ResourceBarrierBatch() = default;
		~ResourceBarrierBatch() = default;

		/*
		* 添加资源屏障
		*/
		void AddBarrier(const ResourceBarrier& barrier);

		inline auto Size() const { return mBarriers.size(); }

		inline auto* D3DBarriers() const { return mBarriers.data(); }

		/*
		* 清空资源屏障
		*/
		void Clear();

	private:
		std::vector<D3D12_RESOURCE_BARRIER> mBarriers;
	};
}