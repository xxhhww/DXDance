#pragma once
#include <string>
#include <memory>
#include "UI/IWidgetContainer.h"
#include "Tools/MemoryStream.h"
#include "Math/Vector.h"
#include "Pin.h"

namespace App {
	/*
	* ���PinΪ����ۻ��������
	*/
	enum { InputFlag = 0, OutputFlag = 1 << 30 };

	/*
	* �ڵ�����
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
		* �ڵ����
		*/
		bool Draw();

		/*
		* �ڵ����Ի���
		*/
		virtual bool DrawProperty() = 0;

		/*
		* �����λ�Ľڵ����Ϳ��ܷ����ı䣬һ��������Add, Mul���������ͽڵ�
		*/
		virtual bool OnInputPinTypeChanged(const std::vector<Pin*>& oppositePins);

		/*
		* ����Ӧ������Pin����Ϊ������״̬
		*/
		void SetInputPinLinked(int index, int inLinkID);

		/*
		* ����Ӧ������Pin����Ϊ�ѶϿ�״̬
		*/
		void SetInputPinBroken(int index);

		/*
		* ����Ӧ�����Pin����Ϊ������״̬
		*/
		void SetOutputPinLinked(int index);

		/*
		* ����Ӧ�����Pin����Ϊ������״̬
		*/
		void SetOutputPinBroken(int index);

		/*
		* ���õ�ǰ�ڵ��ڱ༭���ռ�(Editor Space)�е�λ��
		*/
		void SetPosition(float x, float y);

		inline const auto& GetPosition() const { return mPosition; }
		inline auto& GetInputPins()	{ return mInputPins; }
		inline auto& GetOutputPins()	{ return mOutputPins; }

		virtual void Serialize(Tool::OutputMemoryStream& blob) = 0;
		virtual void Deserialize(Tool::InputMemoryStream& blob) = 0;

		/*
		* ������Ӧ�ڵ�
		*/
		static std::unique_ptr<Node> CreateNode(int id, NodeType nodeType);
	protected:
		/*
		* ��������Pin
		*/
		void EmplaceInputPin(UI::IWidgetContainer* container, PinType slotType, const std::string& label);

		/*
		* �������Pin
		*/
		void EmplaceOutputPin(UI::IWidgetContainer* container, PinType slotType, const std::string& label);
	public:
		int					objectID{ -1 };
		float				width{ 100.0f };
	protected:
		NodeType			mNodeType;
		std::string			mLabel;
		Math::Vector2		mPosition;
		std::vector<Pin*>	mInputPins;				// �����
		std::vector<Pin*>	mOutputPins;			// �����
		bool				mIsEdited{ false };		// �Ƿ񱻱༭
	};
}