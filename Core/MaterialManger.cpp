#include "MaterialManger.h"
#include "ServiceLocator.h"

namespace Core {
	void Material::SetShader(int64_t shaderID) {
		if (CORESERVICE(ShaderManger).IsRegistered(shaderID)) {
			mShaderID = shaderID;
		}
	}
}