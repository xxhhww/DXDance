#pragma once
#include "Node.h"

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
		~Float() = default;

		// From ImnodeObject
		bool Draw() override;

		// From Node
		inline bool DrawAttribute() override { return false; }
	};

	class Float2 : public VariableNode<Math::Vector2> {
	public:
		Float2(int id);
		~Float2() = default;

		// From ImnodeObject
		bool Draw() override;

		// From Node
		inline bool DrawAttribute() override { return false; }
	};
}

#include "VariableNode.inl"