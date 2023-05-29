#pragma once
#include "pbh.h"
#include "Device.h"
#include "IndirectArgument.h"

namespace GHL {
	class CommandSignature : public D3DObject {
	public:
		CommandSignature(const GHL::Device* device);
		~CommandSignature() = default;

		void AddIndirectArgument(const IndirectArgument& argument);

		void Compile(uint32_t stride, ID3D12RootSignature* rootSignature);

		inline auto* D3DCommandSignature() const { return mSignature.Get(); }

		void SetDebugName(const std::string& name) override;

	private:
		std::vector<D3D12_INDIRECT_ARGUMENT_DESC> mArguments;
		bool mCompiled{ false };
		const GHL::Device* mDevice{ nullptr };
		D3D12_COMMAND_SIGNATURE_DESC mCommandSignatureDesc;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> mSignature;
	};
}