#pragma once
#include "Shader.h"
#include "Device.h"

namespace GHL {

	class PipelineState : public D3DObject {
	public:
		PipelineState(const Device* device);
		~PipelineState() = default;

		inline const auto D3DPipelineState() const { return mPipelineState.Get(); }

		void SetDebugName(const std::string& name) override;

		virtual void Compile() = 0;

	protected:
		const Device* mDevice{ nullptr };
		bool mCompiled{ false };
		Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;
		std::string mDebugName;
	};

	class GraphicsPipelineState : public PipelineState {
	public:
		GraphicsPipelineState(const Device* device);
		~GraphicsPipelineState() = default;

		void Compile() override;

	public:
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	};

	class ComputePipelineState : public PipelineState {
	public:
		ComputePipelineState(const Device* device);
		~ComputePipelineState() = default;

		void Compile() override;

	public:
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
	};

}