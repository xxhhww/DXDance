#pragma once
#include <stack>
#include "Tools/MemoryStream.h"

namespace UI {
	class IUndoEditor {
	public:
		virtual void Serialize(Tool::OutputMemoryStream& blob) = 0;
		virtual void Deserialize(Tool::InputMemoryStream& blob) = 0;
	protected:
		struct Undo {
			Tool::OutputMemoryStream blob;
		};

		void PushUndo();
		void PopUndo();

		void RegisterOrigin(Tool::OutputMemoryStream& blob);
		void UpdateOrigin();
	private:
		std::stack<Undo> mUndos;
		Tool::OutputMemoryStream mOriginalBlob;
	};
}