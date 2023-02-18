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
		bool		isExposed{ false };	// 指定该节点是否可以被暴露到材质编辑窗口中
		std::string	name{ "?" };		// 变量名称(与节点的标签不同)
	};

	class Float : public VariableNode {
	public:
		Float(int id);
		inline ~Float() = default;

		/*
		* 在NodeAttribute中绘制图形
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
		* 在NodeAttribute中绘制图形
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
		* 在NodeAttribute中绘制图形
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
		* 在NodeAttribute中绘制图形
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
		* 在NodeAttribute中绘制图形
		*/
		inline bool DrawProperty() override { return false; }

		void Serialize(Tool::OutputMemoryStream& blob) override;
		void Deserialize(Tool::InputMemoryStream& blob) override;

	private:
		UI::ColorEdit* mWidget;
	};
}