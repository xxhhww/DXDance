#pragma once
#include "Renderer/Texture.h"

namespace Renderer {

	class Terrain {
	public:

	private:
		std::unique_ptr<Renderer::Texture> mHeightMap;
		std::unique_ptr<Renderer::Texture> mNormalMap;
	};

}