#pragma once
#include "ECS/IComponent.h"

#include "Math/Vector.h"
#include "Math/Color.h"

namespace ECS {

	using Lumen = float;
	using Nit = float;

	// Maximum spectral luminous efficacy of radiation for photoscopic vision
	// and its value is based on the definition of candela[Wikb] which is the SI unit for luminous intensity measurement. 
	// Its definition is: one candela is the luminous intensity, in a given direction, of a source
	// that emits monochromatic radiation at a frequency of 540THz(i.e a wavelength of 555nm) and whose
	// radiant intensity in that direction is 1 / 683 watts per steradian., meaning that Km = 683.
	inline static const float StandardLuminousEfficacy = 683;

	inline static const float SunAngularRadius = 0.004712389;	// Angular radius of the sun from Earth, in radians
	inline static const float SunSolidAngle    = 0.00006807;	// Average solid angle as seen from Earth
	inline static const float SunDiskRadius    = 0.00471242378; // tan(SunAngularRadius). Disk is at a distance 1 from a surface.

	enum class LightType : uint32_t {
		DIRECTIONAL = 0,
		POINT = 1,
		SPOT  = 2,
		COUNT = 3
	};

	class Light : public ECS::IComponent {
	public:
		LightType mLightType{ LightType::DIRECTIONAL };
		Lumen mLuminousPower{ 0.0f };
		Nit mLuminance{ 0.0f };
		Math::Color mColor{ 1.0f, 1.0f, 1.0f };

	public:
		void SerializeJson(Tool::JsonWriter& writer) const override {

		}

		void DeserializeJson(const Tool::JsonReader& reader) override {

		}

		void OnInspector(UI::IWidgetContainer* container) override {

		}
	};

}