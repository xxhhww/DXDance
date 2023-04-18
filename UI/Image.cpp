#include "Image.h"
#include "imgui.h"

namespace UI {

	Image::Image(uint64_t texID, const Math::Vector2& size) 
	: textureID(texID)
	, size(size) {}

	void Image::_Draw_Internal_Impl() {
		ImGui::Image((ImTextureID)textureID, ImVec2{ size.x, size.y }, ImVec2(0.f, 0.f), ImVec2(1.f, 1.f));
	}

}