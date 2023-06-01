#pragma once
#include "pbh.h"
#include "Resource.h"
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
			Resource* resource,
			EResourceState stateBefore,
			EResourceState stateAfter,
			UINT subResources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	};

	/*
	* Split资源转换屏障
	*/
	class SplitTransitionBarrier : public ResourceBarrier {
	public:
		SplitTransitionBarrier(
			Resource* resource,
			EResourceState stateBefore,
			EResourceState stateAfter,
			bool endFlag,
			UINT subResources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	};

	/*
	* UAV屏障
	*/
	class UAVBarrier : public ResourceBarrier {
	public:
		UAVBarrier(Resource* resource);
	};

	/*
	* 别名屏障
	*/
	class AliasingBarrier : public ResourceBarrier {
	public:
		AliasingBarrier(Resource* resourceBefore, Resource* resourceAfter);
	};


	/*
	* 资源屏障的Batch
	*/
	class ResourceBarrierBatch {
	public:
		ResourceBarrierBatch() = default;
		ResourceBarrierBatch(const ResourceBarrier& barrier);

		~ResourceBarrierBatch() = default;

		ResourceBarrierBatch& operator+=(const ResourceBarrierBatch& other);

		/*
		* 添加资源屏障
		*/
		void AddBarrier(const ResourceBarrier& barrier);
		void AddBarriers(const ResourceBarrierBatch& batch);

		inline bool Empty() const { return mD3DBarriers.size() == 0u; }

		inline auto Size() const { return mD3DBarriers.size(); }

		inline const auto* D3DBarriers() const { return mD3DBarriers.data(); }

		/*
		* 清空资源屏障
		*/
		void Clear();

	private:
		std::vector<D3D12_RESOURCE_BARRIER> mD3DBarriers;
	};
}