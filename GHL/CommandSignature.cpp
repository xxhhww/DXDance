#include "CommandSignature.h"
#include "Tools/StrUtil.h"
#include "Tools/Assert.h"

namespace GHL {
	
	CommandSignature::CommandSignature(const GHL::Device* device) 
	: mDevice(device) {}

	void CommandSignature::AddIndirectArgument(const IndirectArgument& argument) {
		mArguments.push_back(argument.D3DArgument());
	}

	void CommandSignature::SetByteStride(uint32_t stride) {
		mByteStride = stride;
	}

	void CommandSignature::SetRootSignature(ID3D12RootSignature* rootSignature) {
		mRootSignature = rootSignature;
	}

	void CommandSignature::Compile() {
		mCommandSignatureDesc = D3D12_COMMAND_SIGNATURE_DESC{};
		mCommandSignatureDesc.pArgumentDescs = mArguments.data();
		mCommandSignatureDesc.NumArgumentDescs = mArguments.size();
		mCommandSignatureDesc.ByteStride = mByteStride;
		mCommandSignatureDesc.NodeMask = mDevice->GetNodeMask();

		HRASSERT(mDevice->D3DDevice()->CreateCommandSignature(
			&mCommandSignatureDesc, mRootSignature, IID_PPV_ARGS(&mSignature))
		);
	}

	void CommandSignature::SetDebugName(const std::string& name) {
		if (mSignature) {
			HRASSERT(mSignature->SetName(Tool::StrUtil::UTF8ToWString(name).c_str()));
		}
	}

	const std::string& CommandSignature::GetDebugName() {
		return mName;
	}

}