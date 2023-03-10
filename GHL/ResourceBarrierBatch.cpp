#include "ResourceBarrierBatch.h"

namespace GHL {
	ResourceBarrier::ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE barrierType) {
		mBarrier.Type = barrierType;
	}

	TransitionBarrier::TransitionBarrier(
		ID3D12Resource* resource, 
		D3D12_RESOURCE_STATES stateBefore, 
		D3D12_RESOURCE_STATES stateAfter, 
		UINT subResources)
	: ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE_TRANSITION) {
		mBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		mBarrier.Transition.pResource = resource;
		mBarrier.Transition.StateBefore = stateBefore;
		mBarrier.Transition.StateAfter = stateAfter;
		mBarrier.Transition.Subresource = subResources;
	}

	SplitTransitionBarrier::SplitTransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, bool endFlag, UINT subResources)
	: ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE_TRANSITION) {
		mBarrier.Flags = endFlag ? D3D12_RESOURCE_BARRIER_FLAG_END_ONLY : D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
		mBarrier.Transition.pResource = resource;
		mBarrier.Transition.StateBefore = stateBefore;
		mBarrier.Transition.StateAfter = stateAfter;
		mBarrier.Transition.Subresource = subResources;
	}

	UAVBarrier::UAVBarrier(ID3D12Resource* resource)
	: ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE_UAV){
		mBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		mBarrier.UAV.pResource = resource;
	}

	AliasingBarrier::AliasingBarrier(ID3D12Resource* resourceBefore, ID3D12Resource* resourceAfter)
	: ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE_ALIASING) {
		mBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		mBarrier.Aliasing.pResourceBefore = resourceBefore;
		mBarrier.Aliasing.pResourceAfter = resourceAfter;
	}

	void ResourceBarrierBatch::AddBarrier(const ResourceBarrier& barrier) {
		mBarriers.push_back(barrier.D3DBarrier());
	}

	void ResourceBarrierBatch::Clear() {
		mBarriers.clear();
	}

}