#pragma once
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "UI/imnodes.h"
#include "UI/IWidgetContainer.h"
#include "Math/Vector.h"
#include "Tools/MemoryStream.h"

namespace UI {
	class Node;
	// �������ɵ�Shader���г���������������������

	// ���SlotΪ����ۻ��������
	enum SlotFlag { InputFlag = 0, OutputFlag = 1 << 30 };
	enum class SlotType {
		Bool,
		SamplerState,	// ������
		Texture2D,		// 2D����
		Float, Float2, Float3, Float4
	};
	enum class NodeType {
		Float, Float2, Float3, Float4, Bool, Color,
		SamplerState, Texture2D,
		Add, Cos,
		BRDF,
	};


	class ImnodeObject {
	public:
		ImnodeObject(int id);
		virtual ~ImnodeObject() = default;

		// ����ֵ��ʾ�Ƿ񱻱༭
		virtual bool Draw() = 0;
	public:
		int objectID{ -1 };
	};

	// �ڵ��λ
	// objectID�ĵ�31λ����������/����Slot
	// objectID�ĵ�17λ�������Slot�ڶ�ӦNode�Ķ�Ӧλ���ϵ�����
	// objectID��ǰ16λ���Slot��Ӧ��Node��ID��ͬ
	class Slot : public ImnodeObject {
	public:
		Slot(Node* node, int id, const std::string& label, SlotType slotType);
		~Slot() = default;

		bool Draw() override;
	public:
		Node*	ownNode{ nullptr };
		bool	isOutput{ false };
		bool	isLinked{ false };				// �Ƿ�������Slot����
		int		inLinkID{ -1 };					// ָ���Slot��Link��ID(Slot��������Ϊ1)
		SlotType	type{ SlotType::Float };	// �۵�����/�������
	private:
		std::string	mLabel{ "" };				// ��ǩ
	};

	class Link : public ImnodeObject {
	public:
		using Ptr = std::shared_ptr<Link>;
	public:
		inline Link() : ImnodeObject(-1) {}
		inline Link(int id) :ImnodeObject(id) {}
		~Link() = default;

		bool Draw() override;

		int FromNodeID()	const;
		int ToNodeID()		const;
		int FromSlotIndex() const;
		int ToSlotIndex()	const;

		void Serialize(Tool::OutputMemoryStream& blob);
		void Deserialize(Tool::InputMemoryStream& blob);
	public:
		int fromSlot{ -1 };
		int toSlot{ -1 };
	};

	class Node : public ImnodeObject {
	public:
		using Ptr = std::shared_ptr<Node>;
	public:
		Node(int id, NodeType nodeType, const std::string& label);
		virtual ~Node() = default;
		
		// �ڵ����Ի���
		virtual bool DrawAttribute() = 0;
		// �����λ�Ľڵ����Ϳ��ܷ����ı�
		// һ��������Add, Mul���������ͽڵ�
		virtual bool OnInputSlotTypeChanged(const std::vector<Slot*>& oppositeSlots);

		void SetInputSlotLinked(int index, int inLinkID);
		void SetInputSlotBroken(int index);
		void SetOutputSlotLinked(int index);
		void SetOutputSlotBroken(int index);

		void SetPosition(float x, float y);
		inline const auto& GetPosition() const { return mPosition; }

		inline auto& GetInputSlots()	{ return mInputSlots; }
		inline auto& GetOutputSlots()	{ return mOutputSlots; }

		virtual void Serialize(Tool::OutputMemoryStream& blob) = 0;
		virtual void Deserialize(Tool::InputMemoryStream& blob) = 0;
	public:
		float width{ 100.0f };
	protected:
		NodeType mNodeType;
		std::string mLabel;
		Math::Vector2 mPosition;
		std::vector<Slot> mInputSlots;		// �����
		std::vector<Slot> mOutputSlots;		// �����
		IWidgetContainer mWidgetContainer;	// �м�ؼ�
	};
}