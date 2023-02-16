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
		bool isExposed{ false };	// ָ���ýڵ��Ƿ���Ա���¶�����ʱ༭������
	protected:
		TData		mMin;
		TData		mMax;
		TData		mValue;
		std::string	mName{ "?" };	// ��������(��ڵ�ı�ǩ��ͬ)
	};

	class Float : public VariableNode<float> {
	public:
		Float(int id);
		inline ~Float() = default;

		/*
		* ��NodeAttribute�л���ͼ��
		*/
		inline bool DrawProperty() override { return false; }
	};

	class Float2 : public VariableNode<Math::Vector2> {
	public:
		Float2(int id);
		inline ~Float2() = default;

		/*
		* ��NodeAttribute�л���ͼ��
		*/
		inline bool DrawProperty() override { return false; }
	};

	class Color : public VariableNode<Math::Color> {
	public:
		Color(int id);
		inline ~Color() = default;

		/*
		* ��NodeAttribute�л���ͼ��
		*/
		inline bool DrawProperty() override { return false; }
	};
}

#include "VariableNode.inl"