#pragma once
#include "Node.h"

namespace App {
	class MathNode : public Node {
	public:
		inline MathNode(int id, NodeType nodeType, const std::string& label) : Node(id, nodeType, label) {}
		virtual ~MathNode() = default;

		bool OnInputPinTypeChanged(const std::vector<Pin*>& oppositePins) override;

		void Serialize(Tool::OutputMemoryStream& blob) override;
		void Deserialize(Tool::InputMemoryStream& blob) override;
	};

	class Add : public MathNode {
	public:
		Add(int id);
		~Add() = default;

		// From Node
		inline bool DrawProperty() override { return false; }
	};

	class Cos : public MathNode {
	public:
		Cos(int id);
		~Cos() = default;

		// From Node
		inline bool DrawProperty() override { return false; }
	};
}