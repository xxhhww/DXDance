#pragma once
#include <stack>
#include "Tools/MemoryStream.h"
#include "UI/PanelWindow.h"

namespace App {
	/* Undo�ӿ��࣬���κ�֧��Undo��������Ļ��� */
	class ISupportUndoWindow : public UI::PanelWindow {
	public:
		ISupportUndoWindow(
			const std::string& name = "",
			bool opened = true,
			const UI::PanelWindowSettings& panelSettings = UI::PanelWindowSettings{}
		);

		inline ISupportUndoWindow() = default;
	protected:
		struct Undo {
			Tool::OutputMemoryStream blob;
		};

		/*
		* ��ջ��ѹ��һ��Undo����
		*/
		void PushUndo();

		/*
		* ��ջ�е���(����)һ��Undo����
		*/
		void PopUndo();

		/*
		* ע��Undo��ʼ��
		*/
		void RegisterOrigin(Tool::OutputMemoryStream& blob);
		
		/*
		* ����Undo��ʼ��
		*/
		void UpdateOrigin();

		/*
		* ��ȡUndo��ʼ��
		*/
		const auto& GetOriginBlob() const;

		virtual void Serialize(Tool::OutputMemoryStream& blob)	const = 0;
		virtual void Deserialize(Tool::InputMemoryStream& blob) = 0;
	private:
		std::stack<Undo> mUndos;
		Tool::OutputMemoryStream mOriginalBlob;
	};
}