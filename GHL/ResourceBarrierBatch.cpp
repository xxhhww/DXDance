#include "ResourceBarrierBatch.h"

namespace GHL {
	ResourceBarrier::ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE barrierType) {
		mBarrier.Type = barrierType;
	}

	TransitionBarrier::TransitionBarrier(
		Resource* resource,
		EResourceState stateBefore, 
		EResourceState stateAfter,
		UINT subResources)
	: ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE_TRANSITION) {
		mBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		mBarrier.Transition.pResource = resource->D3DResource();
		mBarrier.Transition.StateBefore = GetD3DResourceStates(stateBefore);
		mBarrier.Transition.StateAfter = GetD3DResourceStates(stateAfter);
		mBarrier.Transition.Subresource = subResources;
	}

	SplitTransitionBarrier::SplitTransitionBarrier(Resource* resource, EResourceState stateBefore, EResourceState stateAfter, bool endFlag, UINT subResources)
	: ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE_TRANSITION) {
		mBarrier.Flags = endFlag ? D3D12_RESOURCE_BARRIER_FLAG_END_ONLY : D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
		mBarrier.Transition.pResource = resource->D3DResource();
		mBarrier.Transition.StateBefore = GetD3DResourceStates(stateBefore);
		mBarrier.Transition.StateAfter = GetD3DResourceStates(stateAfter);
		mBarrier.Transition.Subresource = subResources;
	}

	UAVBarrier::UAVBarrier(Resource* resource)
	: ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE_UAV){
		mBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		mBarrier.UAV.pResource = resource->D3DResource();
	}

	AliasingBarrier::AliasingBarrier(Resource* resourceBefore, Resource* resourceAfter)
	: ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE_ALIASING) {
		mBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		mBarrier.Aliasing.pResourceBefore = resourceBefore->D3DResource();
		mBarrier.Aliasing.pResourceAfter = resourceAfter->D3DResource();
	}

	ResourceBarrierBatch::ResourceBarrierBatch(const ResourceBarrier& barrier) {
		AddBarrier(barrier);
	}

	void ResourceBarrierBatch::AddBarrier(const ResourceBarrier& barrier) {
		mD3DBarriers.push_back(barrier.D3DBarrier());
	}

	void ResourceBarrierBatch::AddBarriers(const ResourceBarrierBatch& batch) {
		for (const auto& d3dBarrier : batch.mD3DBarriers) {
			mD3DBarriers.push_back(d3dBarrier);
		}
	}

	void ResourceBarrierBatch::Clear() {
		mD3DBarriers.clear();
	}

}