#pragma once
#include "pbh.h"

namespace GHL {
	/*
	* 间接参数
	*/
	class IndirectArgument {
	public:
		IndirectArgument(D3D12_INDIRECT_ARGUMENT_TYPE argumentType);
		virtual ~IndirectArgument() = default;

		inline const auto& D3DArgument()  const { return mArgument; }

	protected:
		D3D12_INDIRECT_ARGUMENT_DESC mArgument;
	};

	class IndirectDispatchArgument : public IndirectArgument {
	public:
		IndirectDispatchArgument();
	};

	class IndirectDrawArgument : public IndirectArgument {
	public:
		IndirectDrawArgument();
	};

	class IndirectDrawIndexedArgument : public IndirectArgument {
	public:
		IndirectDrawIndexedArgument();
	};

	class IndirectConstantBufferViewArgument : public IndirectArgument {
	public:
		IndirectConstantBufferViewArgument(uint32_t rootParameterIndex);
	};
}
