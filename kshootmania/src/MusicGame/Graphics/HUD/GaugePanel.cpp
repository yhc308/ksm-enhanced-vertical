#include "Graphics/GameRenderContext.hpp"
#include "GaugePanel.hpp"

#include "Vertical/VerticalHUDLayout.hpp"

namespace MusicGame::Graphics
{
	namespace
	{
		constexpr StringView kBaseTextureFilename = U"er.png";
		constexpr StringView kBarTextureFilename = U"er_g.gif";
		constexpr StringView kBarAnimTextureFilename = U"er_g_pattern.gif";
		constexpr StringView kPercentBaseTextureFilename = U"er_p.png";
		constexpr StringView kPercentNumberTextureFontFilename = U"num2.png";

		constexpr SizeF kBarSize = { 47.25, 434.0 };
		constexpr SizeF kBarAnimSize = { kBarSize.x, kBarSize.y * 2 };

		double AnimRate(kson::Pulse currentPulse)
		{
			constexpr kson::Pulse kIntervalPulse = kson::kResolution4 * 3 / 2; // 1.5小節
			return static_cast<double>(MathUtils::WrappedMod(currentPulse, kIntervalPulse)) / kIntervalPulse;
		}
	}

	GaugePanel::GaugePanel(GaugeCalcType gaugeCalcType)
		: m_gaugeCalcType(gaugeCalcType)
		, m_baseTexture(kBaseTextureFilename,
			{
				.column = kNumGaugeTypes * 2,
				.sourceScale = SourceScale::k2x,
				.sourceSize = { 192, 570 },
			})
		, m_barTexture(kBarTextureFilename,
			{
				.column = kNumGaugeTypes * 2,
				.sourceScale = SourceScale::k2x,
				.sourceSize = { 48, 434 },
			})
		, m_barAnimTexture(kBarAnimTextureFilename,
			{
				.column = kNumGaugeTypes * 2,
				.sourceScale = SourceScale::k2x,
				.sourceSize = { 48, 868 },
			})
		, m_percentBaseTexture(TextureAsset(kPercentBaseTextureFilename))
        , m_apsTexture(TextureAsset(U"er_aps.png"))
        , m_apsDodgeShader(PixelShaderAsset(U"ApsColorDodge"))
		, m_percentNumberTextureFont(kPercentNumberTextureFontFilename, { 20, 18 })
		, m_percentNumberLayout(Scaled2x(20, 18), TextureFontTextLayout::Align::Right)
	{
	}

    void GaugePanel::drawAps(const RectF& rect, const GaugeVisual& visual) const
    {
        if(visual.bannerOpacity<=0) return;
        const ScopedRenderStates2D alpha(BlendState::Default2D);
        if(visual.dodge<=0 && visual.burn<=0) m_apsTexture.resized(rect.size).draw(rect.pos,ColorF{1,visual.bannerOpacity});
        else {
            const ScopedCustomShader2D shader(m_apsDodgeShader);
            m_apsTexture.resized(rect.size).draw(rect.pos,ColorF{visual.dodge,visual.burn,0,visual.bannerOpacity});
        }
    }

    void GaugePanel::drawVertical(double percent, kson::Pulse currentPulse, bool apsProtected, Optional<GaugeType> effectiveGauge, GaugeVisual visual) const
	{
        const double actualPercent=percent;
        if(visual.enabled) percent=visual.percent;
        else visual={true,percent,0,apsProtected?1.0:0.0};
        const auto gaugeCalcType = effectiveGauge && !IsCourseGaugeCalcType(m_gaugeCalcType) ? ToGaugeCalcType(*effectiveGauge, GameMode::kNormal) : m_gaugeCalcType;
		const ScopedRenderStates2D samplerState(SamplerState::ClampAniso);
		const GaugeType barType = ToGaugeType(gaugeCalcType);
		const GaugeType baseType = IsCourseGaugeCalcType(gaugeCalcType) ? GaugeType::kHardGauge : barType;
		const int threshold = baseType == GaugeType::kHardGauge ? kGaugePercentageThresholdHardWarning : kGaugePercentageThreshold;
		const int baseColumn = static_cast<int>(baseType) * 2 + (static_cast<int>(actualPercent) < threshold ? 0 : 1);
		const int barColumn = static_cast<int>(barType) * 2 + (static_cast<int>(actualPercent) < threshold ? 0 : 1);
		const auto base = TextureAsset(kBaseTextureFilename);
		const auto header = Vertical::HUDRect(U"GaugeHeader");
		const auto frame = Vertical::HUDRect(U"GaugeFrame");
		const auto fill = Vertical::HUDRect(U"GaugeFill");
		const double apsScale = frame.w / 55.0;
        // Full-canvas APS uses the same 192x570 source coordinates as er.png.
        // Keep the original compact canvas anchor for existing user skins.
        const bool compactAps = m_apsTexture.size() == Size{89, 130};
        const Vec2 apsAnchor = compactAps ? Vec2{28, 4} : Vec2{29, 122};
        const RectF apsRect{frame.pos - apsAnchor * apsScale, SizeF{m_apsTexture.size()} * apsScale};
        base(baseColumn * 192, 0, 192, 118).resized(header.size).draw(header.pos);
		base(baseColumn * 192 + 29, 122, 55, 442).resized(frame.size).draw(frame.pos);
		const auto viewport = Vertical::Viewport::Fit(GameRender::Width(), GameRender::Height());
		base(baseColumn * 192, 122, 29, 442).resized(22 * viewport.scale, frame.h).draw(frame.pos - Vec2{22 * viewport.scale, 0});
		const double fraction = Clamp(percent / 100, 0.0, 1.0);
		const RectF clip{fill.x, fill.y + fill.h * (1 - fraction), fill.w, fill.h * fraction};
		{
			const ScopedRenderStates2D blend(BlendState::Additive);
			m_barTexture(0, barColumn).resized(fill.size).drawClipped(fill.pos, clip);
			const double rate = AnimRate(currentPulse);
			const SizeF animSize{fill.w, fill.h * 2};
			m_barAnimTexture(0, barColumn).resized(animSize).drawClipped(fill.pos - Vec2{0, animSize.y * rate}, clip);
			m_barAnimTexture(0, barColumn).resized(animSize).drawClipped(fill.pos - Vec2{0, animSize.y * (rate - 1)}, clip);
		}
        // Match the landscape er_p source geometry; only the portrait gauge
        // length determines travel, so the pointer follows the displayed fill.
        const Vec2 percentBasePosition{
            frame.x + 43 * apsScale,
            fill.y - 20 * apsScale + fill.h * (431.0 / kBarSize.y) * (1 - fraction)
        };
        m_percentBaseTexture.resized(SizeF{m_percentBaseTexture.size()} * apsScale).draw(percentBasePosition);
        const TextureFontTextLayout numberLayout{{20 * apsScale, 18 * apsScale}, TextureFontTextLayout::Align::Right};
        m_percentNumberTextureFont.draw(numberLayout, percentBasePosition + Vec2{72, 13} * apsScale,
            static_cast<int>(percent), ZeroPaddingYN::No);
        drawAps(apsRect,visual);
	}

	void GaugePanel::draw(double percent, kson::Pulse currentPulse, bool apsProtected, Optional<GaugeType> effectiveGauge, GaugeVisual visual) const
	{
        const double actualPercent=percent;
        if(visual.enabled) percent=visual.percent;
        else visual={true,percent,0,apsProtected?1.0:0.0};
        const auto gaugeCalcType = effectiveGauge && !IsCourseGaugeCalcType(m_gaugeCalcType) ? ToGaugeCalcType(*effectiveGauge, GameMode::kNormal) : m_gaugeCalcType;
		const ScopedRenderStates2D samplerState(SamplerState::ClampAniso);

		const bool isCourseMode = IsCourseGaugeCalcType(gaugeCalcType);
		const GaugeType gaugeTypeForBar = ToGaugeType(gaugeCalcType);
		const GaugeType gaugeTypeForBase = isCourseMode ? GaugeType::kHardGauge : gaugeTypeForBar;

		const int32 percentThreshold = gaugeTypeForBase == GaugeType::kHardGauge ? kGaugePercentageThresholdHardWarning : kGaugePercentageThreshold;
		const int32 percentInt = static_cast<int32>(actualPercent);
		const int32 baseColumn = static_cast<int32>(gaugeTypeForBase) * 2 + (percentInt < percentThreshold ? 0 : 1);
		const int32 barColumn = static_cast<int32>(gaugeTypeForBar) * 2 + (percentInt < percentThreshold ? 0 : 1);

		const Vec2 basePosition = { GameRender::Width() / 2 + Scaled(155), Scaled(88) };
		const bool compactAps = m_apsTexture.size() == Size{89, 130};
        const Vec2 apsOffset = compactAps ? Vec2{1, 118} : Vec2{0, 0};
        const RectF apsRect{basePosition + Scaled2x(apsOffset), Scaled2x(SizeF{m_apsTexture.size()})};
        m_baseTexture(0, baseColumn).draw(basePosition);

		{
			const ScopedRenderStates2D renderState(BlendState::Additive);

			const double barHeight = kBarSize.y * percent / 100;
			const Vec2 barBasePosition = basePosition + Scaled2x(34, 126);
			const RectF barClipRect = { barBasePosition + Vec2::Down(Scaled2x(kBarSize.y - barHeight)), Scaled2x(SizeF{ kBarSize.x, barHeight }) };
			m_barTexture(0, barColumn).resized(Scaled2x(kBarSize)).drawClipped(barBasePosition, barClipRect);

			const double animRate = AnimRate(currentPulse);
			m_barAnimTexture(0, barColumn).resized(Scaled2x(kBarAnimSize)).drawClipped(barBasePosition + Vec2::Up(Scaled2x(kBarAnimSize.y) * animRate), barClipRect);
			m_barAnimTexture(0, barColumn).resized(Scaled2x(kBarAnimSize)).drawClipped(barBasePosition + Vec2::Up(Scaled2x(kBarAnimSize.y) * (animRate - 1.0)), barClipRect);
		}

        const Vec2 percentBasePosition = basePosition + Scaled2x(Vec2{ 72, 106 + 431 * (1.0 - percent / 100) });
		Scaled2x(m_percentBaseTexture).draw(percentBasePosition);

		const Vec2 percentNumberBasePosition = percentBasePosition + Scaled2x(Vec2{ 72, 13 });
		m_percentNumberTextureFont.draw(m_percentNumberLayout, percentNumberBasePosition, static_cast<int>(percent), ZeroPaddingYN::No);
        drawAps(apsRect,visual);
	}
}
