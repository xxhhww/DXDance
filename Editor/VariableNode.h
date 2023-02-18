#pragma once
#include "Node.h"
#include "Math/Color.h"
#include "UI/imnodes.h"
#include "UI/DragFloat.h"
#include "UI/DragFloat2.h"
#include "UI/DragFloat3.h"
#include "UI/DragFloat4.h"
#include "UI/ColorEdit.h"

namespace App {

	class VariableNode : public Node {
	public:
		inline VariableNode(int id, NodeType nodeType, const std::string& label) : Node(id, nodeType, label) {}
		virtual ~VariableNode() = default;

	public:
		bool		isExposed{ false };	// ָ���ýڵ��Ƿ���Ա���¶�����ʱ༭������
		std::string	name{ "?" };		// ��������(��ڵ�ı�ǩ��ͬ)
	};

	class Float : public VariableNode {
	public:
		Float(int id);
		inline ~Float() = default;

		/*
		* ��NodeAttribute�л���ͼ��
		*/
		inline bool DrawProperty() override { return false; }

		void Serialize(Tool::OutputMemoryStream& blob) override;
		void Deserialize(Tool::InputMemoryStream& blob) override;

	private:
		UI::DragFloat* mWidget;
	};

	class Float2 : public VariableNode {
	public:
		Float2(int id);
		~Float2() = default;


		/*
		* ��NodeAttribute�л���ͼ��
		*/
		inline bool DrawProperty() override { return false; }

		void Serialize(Tool::OutputMemoryStream& blob) override;
		void Deserialize(Tool::InputMemoryStream& blob) override;

	private:
		UI::DragFloat2Split* mWidget;
	};

	class Float3 : public VariableNode {
	public:
		Float3(int id);
		~Float3() = default;

		/*
		* ��NodeAttribute�л���ͼ��
		*/
		inline bool DrawProperty() override { return false; }

		void Serialize(Tool::OutputMemoryStream& blob) override;
		void Deserialize(Tool::InputMemoryStream& blob) override;

	private:
		UI::DragFloat3Split* mWidget;
	};

	class Float4 : public VariableNode {
	public:
		Float4(int id);
		~Float4() = default;

		/*
		* ��NodeAttribute�л���ͼ��
		*/
		inline bool DrawProperty() override { return false; }

		void Serialize(Tool::OutputMemoryStream& blob) override;
		void Deserialize(Tool::InputMemoryStream& blob) override;

	private:
		UI::DragFloat4Split* mWidget;
	};

	class Color : public VariableNode {
	public:
		Color(int id);
		~Color() = default;

		/*
		* ��NodeAttribute�л���ͼ��
		*/
		inline bool DrawProperty() override { return false; }

		void Serialize(Tool::OutputMemoryStream& blob) override;
		void Deserialize(Tool::InputMemoryStream& blob) override;

	private:
		UI::ColorEdit* mWidget;
	};
}