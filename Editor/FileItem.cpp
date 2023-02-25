#include "FileItem.h"
#include "UI/imgui.h"

namespace App {
	FileItem::FileItem(const std::string& name, const std::string& path)
	: BrowserItem(name, path, false) {}

	void FileItem::_Draw_Internal_Impl() {
		bool useless = false;

		if (ImGui::Selectable((name + mWidgetID).c_str(), &useless, ImGuiSelectableFlags_AllowDoubleClick)) {
			clickedEvent.Invoke();
		}
	}
}