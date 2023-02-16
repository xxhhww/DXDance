#pragma once
#include <string>
#include <memory>
#include "UI/IWidgetContainer.h"
#include "Tools/MemoryStream.h"
#include "Math/Vector.h"
#include "Pin.h"

namespace App {
	/*
	* 标记Pin为输入槽或者输出槽
	*/
	enum { InputFlag = 0, OutputFlag = 1 << 30 };

	/*
	* 节点类型
	*/
	enum class NodeType {
		Float, Float2, Float3, Float4, Bool, Color,
		SamplerState, Texture2D,
		Add, Cos,
		BRDF,
	};

	class Node : public UI::IWidgetContainer {
	public:
		Node(int id, NodeType nodeType, const std::string& label);
		virtual ~Node() = default;
		
		/*
		* 节点绘制
		*/
		bool Draw();

		/*
		* 节点属性绘制
		*/
		virtual bool DrawProperty() = 0;

		/*
		* 输入槽位的节点类型可能发生改变，一般作用于Add, Mul等算术类型节点
		*/
		virtual bool OnInputPinTypeChanged(const std::vector<Pin*>& oppositePins);

		/*
		* 将对应的输入Pin设置为已连接状态
		*/
		void SetInputPinLinked(int index, int inLinkID);

		/*
		* 将对应的输入Pin设置为已断开状态
		*/
		void SetInputPinBroken(int index);

		/*
		* 将对应的输出Pin设置为已连接状态
		*/
		void SetOutputPinLinked(int index);

		/*
		* 将对应的输出Pin设置为已连接状态
		*/
		void SetOutputPinBroken(int index);

		/*
		* 设置当前节点在编辑器空间(Editor Space)中的位置
		*/
		void SetPosition(float x, float y);

		inline const auto& GetPosition() const { return mPosition; }
		inline auto& GetInputPins()	{ return mInputPins; }
		inline auto& GetOutputPins()	{ return mOutputPins; }

		virtual void Serialize(Tool::OutputMemoryStream& blob) = 0;
		virtual void Deserialize(Tool::InputMemoryStream& blob) = 0;

		/*
		* 创建对应节点
		*/
		static std::unique_ptr<Node> CreateNode(int id, NodeType nodeType);
	protected:
		/*
		* 创建输入Pin
		*/
		void EmplaceInputPin(UI::IWidgetContainer* container, PinType slotType, const std::string& label);

		/*
		* 创建输出Pin
		*/
		void EmplaceOutputPin(UI::IWidgetContainer* container, PinType slotType, const std::string& label);
	public:
		int					objectID{ -1 };
		float				width{ 100.0f };
	protected:
		NodeType			mNodeType;
		std::string			mLabel;
		Math::Vector2		mPosition;
		std::vector<Pin*>	mInputPins;				// 输入槽
		std::vector<Pin*>	mOutputPins;			// 输出槽
		bool				mIsEdited{ false };		// 是否被编辑
	};
}