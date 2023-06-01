#pragma once
#include "pbh.h"
#include "Resource.h"
#include <vector>

namespace GHL {
	/*
	* ��Դ����
	*/
	class ResourceBarrier {
	public:
		ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE barrierType);

		/*
		* Get����
		*/
		inline const auto& D3DBarrier() const { return mBarrier; }

	protected:
		D3D12_RESOURCE_BARRIER mBarrier{};
	};


	/*
	* ��Դת������
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
	* Split��Դת������
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
	* UAV����
	*/
	class UAVBarrier : public ResourceBarrier {
	public:
		UAVBarrier(Resource* resource);
	};

	/*
	* ��������
	*/
	class AliasingBarrier : public ResourceBarrier {
	public:
		AliasingBarrier(Resource* resourceBefore, Resource* resourceAfter);
	};


	/*
	* ��Դ���ϵ�Batch
	*/
	class ResourceBarrierBatch {
	public:
		ResourceBarrierBatch() = default;
		ResourceBarrierBatch(const ResourceBarrier& barrier);

		~ResourceBarrierBatch() = default;

		ResourceBarrierBatch& operator+=(const ResourceBarrierBatch& other);

		/*
		* �����Դ����
		*/
		void AddBarrier(const ResourceBarrier& barrier);
		void AddBarriers(const ResourceBarrierBatch& batch);

		inline bool Empty() const { return mD3DBarriers.size() == 0u; }

		inline auto Size() const { return mD3DBarriers.size(); }

		inline const auto* D3DBarriers() const { return mD3DBarriers.data(); }

		/*
		* �����Դ����
		*/
		void Clear();

	private:
		std::vector<D3D12_RESOURCE_BARRIER> mD3DBarriers;
	};
}