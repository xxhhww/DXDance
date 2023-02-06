#pragma once
#include "IAssetManger.h"
#include <variant>

namespace Core {
	using Var = std::variant<bool, int, float>;

	enum class ShaderVar {
		Bool, Int, Float, Vec2, Vec3, Vec4, Color
	};

	enum class ShaderVarUI {
		Drag, Slider, Input, Color
	};

	struct ShaderDataDesc {
		ShaderVar	var;
		ShaderVarUI ui;
	};

	class Shader : public IAsset {
	public:
	private:
		void* mD3DRootSignature{ nullptr };
		void* mD3DPipelineState{ nullptr };
		std::unordered_map<std::string, ShaderDataDesc> mDataLayout;	// 数据布局
	public:
		const auto& GetDataLayout() const { return mDataLayout; }
	};

	class ShaderManger : public IAssetManger<Shader> {
	public:
	private:
	};
}