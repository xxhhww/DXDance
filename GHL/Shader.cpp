#include "Shader.h"

namespace GHL {

	void Shader::SetDesc(const ShaderDesc& desc) {
		mDesc = desc;
	}

	void Shader::SetBytecode(void* data, size_t size) {
		mBlob.resize(size);
		memcpy(mBlob.data(), data, size);
	}

}