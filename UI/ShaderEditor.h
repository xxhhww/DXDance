#pragma once
#include "ISupportUndo.h"
#include "UI/PanelWindow.h"
#include "Graph.h"

namespace App {
	class ShaderEditor : public ISupportUndo, public UI::PanelWindow {
	public:
		/*
		* 构造函数
		*/
		ShaderEditor();

		/*
		* 默认析构函数
		*/
		inline ShaderEditor() = default;
	protected:
		void _Draw_Internal_Impl() override;
	private:
		uint32_t mNodeIncID{ 0u };
		uint32_t mLinkIncID{ 0u };
		Graph* mGraph{ nullptr };
		ImNodesMiniMapLocation mMinimapLocation{ ImNodesMiniMapLocation_BottomRight };
	};
}