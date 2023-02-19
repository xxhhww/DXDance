#include "Canvas.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include <algorithm>

namespace UI {
	void Canvas::DeleteAllPanels() {
		mPanels.clear();
	}

	void Canvas::SetDockspace(bool status) {
		mDockSpace = status;
	}

	bool Canvas::IsDockspace() const {
		return mDockSpace;
	}

	void Canvas::Draw() {
		if (!mPanels.empty()) {
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			if (true) {
				ImGuiViewport* viewport = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(viewport->Pos);
				ImGui::SetNextWindowSize(viewport->Size);
				ImGui::SetNextWindowViewport(viewport->ID);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

				ImGui::Begin("##dockspace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking);
				ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
				ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
				ImGui::SetWindowPos({ 0.f, 0.f });
				ImVec2 displaySize = ImGui::GetIO().DisplaySize;
				ImGui::SetWindowSize({ (float)displaySize.x, (float)displaySize.y });
				ImGui::End();

				ImGui::PopStyleVar(3);
			}

			for (auto& pair : mPanels) {
				pair.second->Draw();
			}
		}
	}
}