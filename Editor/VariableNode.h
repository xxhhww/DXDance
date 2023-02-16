#pragma once
#include "Node.h"
#include "Math/Color.h"
#include "UI/imnodes.h"

namespace App {

	template<typename TData>
	class VariableNode : public Node {
	public:
		inline VariableNode(int id, NodeType nodeType, const std::string& label) : Node(id, nodeType, label) {}
		virtual ~VariableNode() = default;

		void Serialize(Tool::OutputMemoryStream& blob) override;
		void Deserialize(Tool::InputMemoryStream& blob) override;
	public:
		bool isExposed{ false };	// 指定该节点是否可以被暴露到材质编辑窗口中
	protected:
		TData		mMin;
		TData		mMax;
		TData		mValue;
		std::string	mName{ "?" };	// 变量名称(与节点的标签不同)
	};

	class Float : public VariableNode<float> {
	public:
		Float(int id);
		inline ~Float() = default;

		/*
		* 在NodeAttribute中绘制图形
		*/
		inline bool DrawProperty() override { return false; }
	};

	class Float2 : public VariableNode<Math::Vector2> {
	public:
		Float2(int id);
		inline ~Float2() = default;

		/*
		* 在NodeAttribute中绘制图形
		*/
		inline bool DrawProperty() override { return false; }
	};

	class Color : public VariableNode<Math::Color> {
	public:
		Color(int id);
		inline ~Color() = default;

		/*
		* 在NodeAttribute中绘制图形
		*/
		inline bool DrawProperty() override { return false; }
	};
}

#include "VariableNode.inl"