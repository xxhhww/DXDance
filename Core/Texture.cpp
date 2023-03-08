#include "Texture.h"
#include "TextureLoader.h"

namespace Core {
	Texture::Texture(IAssetManger<Texture>* manger)
	: mManger(manger) {}
	
	void Texture::Load(bool aSync) {
		TextureLoader::Create(mManger->GetRealPath(mPath), *this);
	}

	void Texture::Unload() {
		DirectX::ScratchImage::Release();
	}
}