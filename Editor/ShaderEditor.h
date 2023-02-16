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
		* 构造函数
		*/
		ShaderEditor(
			const std::string& name = "",
			bool opened = true,
			const UI::PanelWindowSettings& panelSettings = UI::PanelWindowSettings{}
		);

		/*
		* 处理快捷键输入
		*/
		void HandleShortCut();

		/*
		* 默认析构函数
		*/
		inline ~ShaderEditor() = default;
	private:
		/*
		* 创建方法菜单
		*/
		void CreateFuncsMenu(UI::MenuBar& menuBar);

		/*
		* 创建小窗口菜单
		*/
		void CreateMiniMapMenu(UI::MenuBar& menuBar);

		/*
		* 创建节点菜单
		*/
		void CreateNodesMenu(UI::MenuBar& menuBar);

		/*
		* 编译ShaderGraph
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