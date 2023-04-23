#pragma once
#include <unordered_map>
#include <memory>

namespace Renderer {

	class StreamTexture;
	class Texture;

	class StreamTextureStorage {
	public:


	private:
		std::unordered_map<std::string, std::unique_ptr<StreamTexture>> mTextureStorages;
	};

}