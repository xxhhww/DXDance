#pragma once
#include "DirectXTex/DirectXTex.h"
#include "Renderer/ResourceFormat.h"

namespace Renderer {

	/*
	* ��ʽת����
	*/
	class FormatConverter {
	public:
		static Renderer::TextureDesc GetTextureDesc(const DirectX::TexMetadata& metadata);

		static DirectX::TexMetadata GetTexMetadata(const Renderer::TextureDesc& textureDesc);
	};

}