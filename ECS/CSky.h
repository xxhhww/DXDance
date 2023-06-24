#pragma once
#include "ECS/IComponent.h"
#include "Math/Vector.h"
#include "Math/Spectrum.h"
#include "Math/ArHosekSkyModel.h"

namespace ECS {

	// Maximum spectral luminous efficacy of radiation for photoscopic vision
	// and its value is based on the definition of candela[Wikb] which is the SI unit for luminous intensity measurement. 
	// Its definition is: one candela is the luminous intensity, in a given direction, of a source
	// that emits monochromatic radiation at a frequency of 540THz(i.e a wavelength of 555nm) and whose
	// radiant intensity in that direction is 1 / 683 watts per steradian., meaning that Km = 683.
	inline static const float StandardLuminousEfficacy = 683.0f;

	inline static const float SunAngularRadius = 0.004712389f;		// Angular radius of the sun from Earth, in radians
	inline static const float SunSolidAngle    = 0.00006807f;		// Average solid angle as seen from Earth
	inline static const float SunDiskRadius    = 0.00471242378f;	// tan(SunAngularRadius). Disk is at a distance 1 from a surface.
	inline static const float SunDiskArea      = SunDiskRadius * SunDiskRadius * DirectX::XM_PI;

	class Sky : public ECS::IComponent {
	public:
		// See "SunDirection" In Transform Component "worldRotation"
		float         turbidity{ 4.0f };
		Math::Vector3 groundAlbedo{ 0.1f, 0.1f, 0.1f };
		Math::Vector3 sunIlluminance{ 1.0f, 1.0f, 1.0f };
		Math::Vector3 sunLuminance{ 1.0f, 1.0f, 1.0f };

		Math::SampledSpectrum skySpectrum{ 25u, 400.0f, 700.0f };
		Math::SampledSpectrum groundAlbedoSpectrum{ 25u, 400.0f, 700.0f };
		ArHosekSkyModelState* skyModelStateR{ nullptr };
		ArHosekSkyModelState* skyModelStateG{ nullptr };
		ArHosekSkyModelState* skyModelStateB{ nullptr };

		Sky() {
			skySpectrum.Init();
			groundAlbedoSpectrum.Init();
			groundAlbedoSpectrum.FromRGB(groundAlbedo);
		}

		void SerializeJson(Tool::JsonWriter& writer) const override {

		}

		void DeserializeJson(const Tool::JsonReader& reader) override {

		}

		void OnInspector(UI::IWidgetContainer* container) override {
			auto* group = &container->CreateWidget<UI::GroupCollapsable>("Sky");

			auto& albedoItem = group->CreateWidget<UI::DragFloat3>("Albedo", groundAlbedo, 0.0f, 1.0f);
			albedoItem.dataGatherer = [this]() -> Math::Vector3 {
				return groundAlbedo;
			};
			albedoItem.dataProvider = [this](Math::Vector3 newValue) {
				groundAlbedo = newValue;
			};

			auto& turbidityItem = group->CreateWidget<UI::DragFloat>("Turbidity", turbidity, 1.0f, 10.0f);
			turbidityItem.dataGatherer = [this]() -> float {
				return turbidity;
			};
			turbidityItem.dataProvider = [this](float newValue) {
				turbidity = newValue;
			};
		}
	};

}