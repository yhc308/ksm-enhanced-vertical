#pragma once
#include "MusicGame/GameDefines.hpp"
#include "MusicGame/GaugePresentation.hpp"
#include "Graphics/NumberTextureFont.hpp"

namespace MusicGame::Graphics
{
	class GaugePanel
	{
	private:
		const GaugeCalcType m_gaugeCalcType;
		const TiledTexture m_baseTexture;
		const TiledTexture m_barTexture;
		const TiledTexture m_barAnimTexture;
		const Texture m_percentBaseTexture;
        const Texture m_apsTexture;
        const PixelShader m_apsDodgeShader;
        void drawAps(const RectF& rect, const GaugeVisual& visual) const;
		const NumberTextureFont m_percentNumberTextureFont;
		const TextureFontTextLayout m_percentNumberLayout;

	public:
		explicit GaugePanel(GaugeCalcType gaugeCalcType);

		void draw(double percent, kson::Pulse currentPulse, bool apsProtected = false, Optional<GaugeType> effectiveGauge = none, GaugeVisual visual = {}) const;
		void drawVertical(double percent, kson::Pulse currentPulse, bool apsProtected = false, Optional<GaugeType> effectiveGauge = none, GaugeVisual visual = {}) const;
	};
}
