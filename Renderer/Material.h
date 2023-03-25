#pragma once
#include "Shader.h"

namespace Renderer {

	/*
	* 用户可编辑的材质
	*/
	class Material {
	public:
		/*
		* 仅保存数据，其余信息可见于Shader中的Uniform
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
		std::vector<Texture*> mTextures; // 纹理插槽
		std::vector<Uniform> mUniforms;  // 常量缓冲数据
	};

}