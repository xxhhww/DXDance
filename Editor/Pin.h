#pragma once
#include "UI/IWidget.h"

namespace App {
	class Node;

	enum class PinType {
		Bool,
		SamplerState,	// ������
		Texture2D,		// 2D����
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
		bool	isLinked{ false };				// �Ƿ�������Pin����
		int		inLinkID{ -1 };					// ָ���Pin��Link��ID(Pin��������Ϊ1)
		PinType	type{ PinType::Float };	// �۵�����/�������
	private:
		std::string	mLabel{ "" };				// ��ǩ
	};
}