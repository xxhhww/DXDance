#pragma once
#include "pbh.h"
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
			ID3D12Resource* resource,
			D3D12_RESOURCE_STATES stateBefore,
			D3D12_RESOURCE_STATES stateAfter,
			UINT subResources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	};

	/*
	* Split��Դת������
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
	* UAV����
	*/
	class UAVBarrier : public ResourceBarrier {
	public:
		UAVBarrier(ID3D12Resource* resource);
	};

	/*
	* ��������
	*/
	class AliasingBarrier : public ResourceBarrier {
	public:
		AliasingBarrier(ID3D12Resource* resourceBefore, ID3D12Resource* resourceAfter);
	};


	/*
	* ��Դ���ϵ�Batch
	*/
	class ResourceBarrierBatch {
	public:
		ResourceBarrierBatch() = default;
		~ResourceBarrierBatch() = default;

		/*
		* �����Դ����
		*/
		void AddBarrier(const ResourceBarrier& barrier);

		inline auto Size() const { return mBarriers.size(); }

		inline auto* D3DBarriers() const { return mBarriers.data(); }

		/*
		* �����Դ����
		*/
		void Clear();

	private:
		std::vector<D3D12_RESOURCE_BARRIER> mBarriers;
	};
}