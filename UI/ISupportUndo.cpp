#include "ISupportUndo.h"

namespace App {
	void ISupportUndo::PushUndo() {
		Undo undo;
		Serialize(undo.blob);
		mUndos.push(std::move(undo));
	}

	void ISupportUndo::PopUndo() {
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

	void ISupportUndo::RegisterOrigin(Tool::OutputMemoryStream& blob) { mOriginalBlob = std::move(blob); }

	void ISupportUndo::UpdateOrigin() {
		if (mUndos.empty()) {
			return;
		}
		Undo& undo = mUndos.top();
		mOriginalBlob = std::move(undo.blob);

		std::stack<Undo> tempStack;
		mUndos.swap(tempStack);
	}

	const auto& ISupportUndo::GetOriginBlob() const {
		return mOriginalBlob;
	}
}