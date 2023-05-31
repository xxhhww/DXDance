#include "IndirectArgument.h"

namespace GHL {

	IndirectArgument::IndirectArgument(D3D12_INDIRECT_ARGUMENT_TYPE argumentType) {
		mArgument.Type = argumentType;
	}

	IndirectDispatchArgument::IndirectDispatchArgument()
	: IndirectArgument(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH) {}

	IndirectDrawArgument::IndirectDrawArgument()
	: IndirectArgument(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW) {}

	IndirectDrawIndexedArgument::IndirectDrawIndexedArgument()
	: IndirectArgument(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED) {}

	IndirectConstantBufferViewArgument::IndirectConstantBufferViewArgument(uint32_t rootParameterIndex)
	: IndirectArgument(D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW) {
		mArgument.ConstantBufferView.RootParameterIndex = rootParameterIndex;
	}

}