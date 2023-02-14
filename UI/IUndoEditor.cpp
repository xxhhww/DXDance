#include "IUndoEditor.h"

namespace UI {
	void IUndoEditor::PushUndo() {
		Undo undo;
		Serialize(undo.blob);
		mUndos.push(std::move(undo));
	}

	void IUndoEditor::PopUndo() {
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

	void IUndoEditor::RegisterOrigin(Tool::OutputMemoryStream& blob) { mOriginalBlob = std::move(blob); }

	void IUndoEditor::UpdateOrigin() {
		if (mUndos.empty()) {
			return;
		}
		Undo& undo = mUndos.top();
		mOriginalBlob = std::move(undo.blob);

		std::stack<Undo> tempStack;
		mUndos.swap(tempStack);
	}
}