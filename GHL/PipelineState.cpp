#include "PipelineState.h"
#include "Tools/StrUtil.h"
#include "Tools/Assert.h"

namespace GHL {

	PipelineState::PipelineState(const Device* device) : mDevice(device) {}

	void PipelineState::SetDebugName(const std::string& name) {
		mDebugName = name;
		if (mCompiled) {
			mPipelineState->SetName(Tool::StrUtil::UTF8ToWString(mDebugName).c_str());
		}
	}

	GraphicsPipelineState::GraphicsPipelineState(const Device* device)
	: PipelineState(device) {}

	void GraphicsPipelineState::Compile() {

		desc.NodeMask = mDevice->GetNodeMask();
#if defined(DEBUG) || defined(_DEBUG) 
		// desc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
#endif
		HRASSERT(mDevice->D3DDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPipelineState)));
		mCompiled = true;
		if (!mDebugName.empty()) {
			mPipelineState->SetName(Tool::StrUtil::UTF8ToWString(mDebugName).c_str());
		}

	}

	ComputePipelineState::ComputePipelineState(const Device* device)
	: PipelineState(device) {}

	void ComputePipelineState::Compile() {

		desc.NodeMask = mDevice->GetNodeMask();
#if defined(DEBUG) || defined(_DEBUG) 
		desc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
#endif
		HRASSERT(mDevice->D3DDevice()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&mPipelineState)));
		mCompiled = true;
		if (!mDebugName.empty()) {
			mPipelineState->SetName(Tool::StrUtil::UTF8ToWString(mDebugName).c_str());
		}

	}

}