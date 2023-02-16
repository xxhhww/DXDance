#pragma once
#include "UI/IWidget.h"

namespace App {
	class Node;

	enum class PinType {
		Bool,
		SamplerState,	// 采样器
		Texture2D,		// 2D纹理
		Float, Float2, Float3, Float4
	};

	class Pin : public UI::IWidget {
	public:
		Pin(Node* node, int id, bool output, PinType slotType, const std::string& label);
		inline ~Pin() = default;
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Node*	ownNode{ nullptr };
		int		objectID{ -1 };
		bool	isOutput{ false };
		bool	isLinked{ false };				// 是否与其他Pin连接
		int		inLinkID{ -1 };					// 指向该Pin的Link的ID(Pin的入度最大为1)
		PinType	type{ PinType::Float };	// 槽的输入/输出类型
	private:
		std::string	mLabel{ "" };				// 标签
	};
}