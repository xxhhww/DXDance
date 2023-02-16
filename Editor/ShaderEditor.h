#pragma once
#include "ISupportUndo.h"
#include "UI/PanelWindow.h"
#include "UI/MenuList.h"
#include "UI/Setting.h"
#include "Graph.h"

namespace App {
	class ShaderEditor : public ISupportUndoWindow {
	public:
		/*
		* ���캯��
		*/
		ShaderEditor(
			const std::string& name = "",
			bool opened = true,
			const UI::PanelWindowSettings& panelSettings = UI::PanelWindowSettings{}
		);

		/*
		* �����ݼ�����
		*/
		void HandleShortCut();

		/*
		* Ĭ����������
		*/
		inline ~ShaderEditor() = default;
	private:
		/*
		* ���������˵�
		*/
		void CreateFuncsMenu(UI::MenuBar& menuBar);

		/*
		* ����С���ڲ˵�
		*/
		void CreateMiniMapMenu(UI::MenuBar& menuBar);

		/*
		* �����ڵ�˵�
		*/
		void CreateNodesMenu(UI::MenuBar& menuBar);

		/*
		* ����ShaderGraph
		*/
		inline void Compile() {}
	protected:
		void _Draw_Internal_Impl() override;

		void Serialize(Tool::OutputMemoryStream& blob)  const;
		void Deserialize(const Tool::InputMemoryStream& blob);
	private:
		uint32_t mNodeIncID{ 0u };
		uint32_t mLinkIncID{ 0u };
		std::unique_ptr<Graph> mGraph{ nullptr };

		UI::MenuItem* mMiniMapMenuItem[4];
		ImNodesMiniMapLocation mMinimapLocation{ ImNodesMiniMapLocation_BottomRight };
	};
}