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
	// 编译生成的Shader带有常量缓冲区与纹理插槽数组

	// 标记Slot为输入槽或者输出槽
	enum SlotFlag { InputFlag = 0, OutputFlag = 1 << 30 };
	enum class SlotType {
		Bool,
		SamplerState,	// 采样器
		Texture2D,		// 2D纹理
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

		// 返回值表示是否被编辑
		virtual bool Draw() = 0;
	public:
		int objectID{ -1 };
	};

	// 节点槽位
	// objectID的第31位用来标记输出/输入Slot
	// objectID的第17位用来标记Slot在对应Node的对应位置上的索引
	// objectID的前16位与该Slot对应的Node的ID相同
	class Slot : public ImnodeObject {
	public:
		Slot(Node* node, int id, const std::string& label, SlotType slotType);
		~Slot() = default;

		bool Draw() override;
	public:
		Node*	ownNode{ nullptr };
		bool	isOutput{ false };
		bool	isLinked{ false };				// 是否与其他Slot连接
		int		inLinkID{ -1 };					// 指向该Slot的Link的ID(Slot的入度最大为1)
		SlotType	type{ SlotType::Float };	// 槽的输入/输出类型
	private:
		std::string	mLabel{ "" };				// 标签
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
		
		// 节点属性绘制
		virtual bool DrawAttribute() = 0;
		// 输入槽位的节点类型可能发生改变
		// 一般作用于Add, Mul等算术类型节点
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
		std::vector<Slot> mInputSlots;		// 输入槽
		std::vector<Slot> mOutputSlots;		// 输出槽
		IWidgetContainer mWidgetContainer;	// 中间控件
	};
}