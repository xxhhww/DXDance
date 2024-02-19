#include "Renderer/RuntimeVirtualTextureBackend.h"

namespace Renderer {

	RuntimeVirtualTextureBackend::RuntimeVirtualTextureBackend(TerrainRenderer* renderer, TerrainSetting& terrainSetting)
	: mRenderer(renderer)
	, mTerrainSetting(terrainSetting) {
		// Æô¶¯Ïß³Ì
		mThread = std::thread([this]() {
				this->BackendThread();
			}
		);
	}

	RuntimeVirtualTextureBackend::~RuntimeVirtualTextureBackend() {

	}

	void RuntimeVirtualTextureBackend::BackendThread() {

	}

}