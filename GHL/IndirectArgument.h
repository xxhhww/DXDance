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

	class IndirectShaderResourceViewArgument : public IndirectArgument {
	public:
		IndirectShaderResourceViewArgument(uint32_t rootParameterIndex);
	};

	class IndirectVertexBufferViewArgument : public IndirectArgument {
	public:
		IndirectVertexBufferViewArgument(uint32_t slot = 0u);
	};

	class IndirectIndexBufferViewArgument : public IndirectArgument {
	public:
		IndirectIndexBufferViewArgument();
	};

}
