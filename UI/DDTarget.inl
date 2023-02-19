#pragma once
#include "DDTarget.h"

namespace UI {
	/*
	* Create the drag and drop target
	*/
	template<typename T>
	DDTarget<T>::DDTarget(const std::string& p_identifier) : identifier(p_identifier) {}

	/*
	* Execute the drag and drop target behaviour
	*/
	template<typename T>
	void DDTarget<T>::Execute() {
		if (ImGui::BeginDragDropTarget()) {
			if (!mIsHovered)
				hoverStartEvent.Invoke();

			mIsHovered = true;

			ImGuiDragDropFlags target_flags = 0;
			// target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something

			if (!showYellowRect)
				target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle

			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(identifier.c_str(), target_flags)) {
				T data = *(T*)payload->Data;
				dataReceivedEvent.Invoke(data);
			}
			ImGui::EndDragDropTarget();
		}
		else {
			if (mIsHovered)
				hoverEndEvent.Invoke();

			mIsHovered = false;
		}
	}

	/*
	* Returns true if the drag and drop target is hovered by a drag and drop source
	*/
	template<typename T>
	bool DDTarget<T>::IsHovered() const {
		return mIsHovered;
	}

}