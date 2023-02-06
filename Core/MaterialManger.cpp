#include "MaterialManger.h"
#include "ServiceLocator.h"

namespace Core {
	void Material::SetShader(int64_t shaderID) {
		if (CORESERVICE(ShaderManger).IsRegistered(shaderID)) {
			mShaderID = shaderID;
			OnShaderChanged();
		}
	}

	void Material::OnShaderChanged() {
		const auto& dataLayout = CORESERVICE(ShaderManger).GetResource(mShaderID)->GetDataLayout();
		// ����ʧ��keyɾ��
		std::vector<std::string> deletedKeys;
		for (const auto& pair : mMaterialDatas) {
			if (dataLayout.find(pair.first) == dataLayout.end()) {
				deletedKeys.push_back(pair.first);
			}
		}
		for (const auto& key : deletedKeys) {
			mMaterialDatas.erase(key);
		}
		// ����µ�Key
		for (const auto& pair : dataLayout) {
			if (mMaterialDatas.find(pair.first) == mMaterialDatas.end()) {
				mMaterialDatas[pair.first] = Var(0.0f);
			}
		}
	}
}