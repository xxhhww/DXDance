#pragma once
#include "Shader.h"

namespace Renderer {

	/*
	* �û��ɱ༭�Ĳ���
	*/
	class Material {
	public:
		/*
		* ���������ݣ�������Ϣ�ɼ���Shader�е�Uniform
		*/
		struct Uniform {
		public:
			union Var {
				float         v1;
				Math::Vector2 v2;
				Math::Vector3 v3;
				Math::Vector4 v4;
				Math::Color   color;
			};

		public:
			Var var;
		};

	public:
		inline const auto* GetShader()       const { return mShader; }
		inline const auto& GetTextures()     const { return mTextures; }
		inline const auto& GetUniforms()     const { return mUniforms; }
		inline const auto* GetUniformsData() const { return reinterpret_cast<const uint8_t*>(mUniforms.data()); }
		
	private:
		GraphicsShader* mShader{ nullptr };
		std::vector<Texture*> mTextures; // ������
		std::vector<Uniform> mUniforms;  // ������������
	};

}