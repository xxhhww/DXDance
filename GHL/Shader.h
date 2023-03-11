#pragma once
#include "pbh.h"
#include "ShaderDesc.h"

namespace GHL {

	class Shader {
	public:

		/*
		* Get∑Ω∑®
		*/
		inline const auto& GetShaderDesc() const { return mDesc; }
		inline const auto  GetPointer()    const { return (void*)mBlob.data(); }
		inline const auto  GetLength()     const { return mBlob.size(); }

		/*
		* ÷ÿ‘ÿ
		*/
		inline operator D3D12_SHADER_BYTECODE() const { return { GetPointer(), GetLength() }; }

	private:
		ShaderDesc mDesc{};
		std::vector<uint8_t> mBlob;
	};
}