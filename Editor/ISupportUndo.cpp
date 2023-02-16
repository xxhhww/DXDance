#include "ISupportUndo.h"

namespace App {
	ISupportUndoWindow::ISupportUndoWindow(
		const std::string& name,
		bool opened,
		const UI::PanelWindowSettings& panelSettings)
	: PanelWindow(name, opened, panelSettings) {}


	void ISupportUndoWindow::PushUndo() {
		Undo undo;
		Serialize(undo.blob);
		mUndos.push(std::move(undo));
	}

	void ISupportUndoWindow::PopUndo() {
		if (mUndos.empty()) {
			return;
		}
		mUndos.pop();
		if (!mUndos.empty()) {
			Undo& undo = mUndos.top();
			Tool::InputMemoryStream inputBlob(undo.blob);
			Deserialize(inputBlob);
		}
		else {
			Tool::InputMemoryStream inputBlob(mOriginalBlob);
			Deserialize(inputBlob);
		}
	}

	void ISupportUndoWindow::RegisterOrigin(Tool::OutputMemoryStream& blob) { mOriginalBlob = std::move(blob); }

	void ISupportUndoWindow::UpdateOrigin() {
		if (mUndos.empty()) {
			return;
		}
		Undo& undo = mUndos.top();
		mOriginalBlob = std::move(undo.blob);

		std::stack<Undo> tempStack;
		mUndos.swap(tempStack);
	}

	const auto& ISupportUndoWindow::GetOriginBlob() const {
		return mOriginalBlob;
	}
}