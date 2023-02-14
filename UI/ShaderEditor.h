#pragma once
#include "IUndoEditor.h"
#include "IPanelTransformable.h"
#include "imnodes.h"
#include "Graph.h"

namespace UI {
	class ShaderEditor : public IUndoEditor, public IPanelTransformable {
	public:
	protected:
		void _Draw_Internal_Impl() override;
	private:
		uint32_t mNodeIDGenerator{ 0u };
		uint32_t mLinkIDGenerator{ 0u };
		Graph* mGraph{ nullptr };
		ImNodesMiniMapLocation mMinimapLocation{ ImNodesMiniMapLocation_BottomRight };
	};
}