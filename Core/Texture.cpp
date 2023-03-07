#include "Texture.h"
#include "TextureLoader.h"

namespace Core {
	Texture::Texture(IAssetManger<Texture>* manger)
	: mManger(manger) {}
	
	void Texture::Load(const std::string& path, bool aSync) {
		TextureLoader::Create(path, *this);
	}

	void Texture::Unload(const std::string& path) {
		DirectX::ScratchImage::Release();
	}
}