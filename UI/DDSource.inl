#pragma once
#include "DDSource.h"

namespace UI {
	/*
	* Create the drag and drop source
	*/
	template<typename T>
	DDSource<T>::DDSource
	(
		const std::string& p_identifier,
		const std::string& p_tooltip,
		T p_data
	) : identifier(p_identifier), tooltip(p_tooltip), data(p_data) {}

	/*
	* Execute the behaviour of the drag and drop source
	*/
	template<typename T>
	void DDSource<T>::Execute() {
		ImGuiDragDropFlags src_flags = 0;
		src_flags |= ImGuiDragDropFlags_SourceNoDisableHover;     // Keep the source displayed as hovered
		src_flags |= ImGuiDragDropFlags_SourceNoHoldToOpenOthers; // Because our dragging is local, we disable the feature of opening foreign treenodes/tabs while dragging

		if (!hasTooltip)
			src_flags |= ImGuiDragDropFlags_SourceNoPreviewTooltip; // Hide the tooltip

		if (ImGui::BeginDragDropSource(src_flags)) {
			if (!mIsDragged)
				dragStartEvent.Invoke();

			mIsDragged = true;

			if (!(src_flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
				ImGui::Text(tooltip.c_str());
			ImGui::SetDragDropPayload(identifier.c_str(), &data, sizeof(data));
			ImGui::EndDragDropSource();
		}
		else {
			if (mIsDragged)
				dragStopEvent.Invoke();

			mIsDragged = false;
		}
	}

	/*
	* Returns true if the drag and drop source is dragged
	*/
	template<typename T>
	bool DDSource<T>::IsDragged() const
	{
		return mIsDragged;
	}
}