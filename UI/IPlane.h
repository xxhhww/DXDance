#pragma once
#include "IDrawable.h"
#include "IWidgetContainer.h"

namespace UI {
	class IPlane : public IDrawable, public IWidgetContainer {
	public:
		IPlane();
		virtual ~IPlane() = default;

		void Draw() override;
	protected:
		virtual void _Draw_Internal_Impl() = 0;
	protected:
		bool mEnable{ true };
		std::string mPlaneID{ "?" };
		static int64_t smPlaneIDInc;
	public:
		inline const auto& GetPlaneID() const { return mPlaneID; }
		inline auto IsEnable() const { return mEnable; }
	};
}