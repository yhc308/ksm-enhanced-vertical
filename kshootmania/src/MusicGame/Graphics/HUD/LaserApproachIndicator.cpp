#include "Graphics/GameRenderContext.hpp"
#include "LaserApproachIndicator.hpp"
#include "kson/Util/TimingUtils.hpp"
#include "Vertical/VerticalModConfig.hpp"
#include "Vertical/VerticalHUDLayout.hpp"

namespace MusicGame::Graphics
{
	namespace
	{
		constexpr StringView kTextureFilename = U"laser_p.png";

		constexpr Size kIconSize = { 60, 60 };

		// トリガー判定範囲(LASER開始の1.4秒～2.1秒前)
		constexpr Duration kScanAheadTime = 2.1s;
		constexpr Duration kTriggerThreshold = 1.4s;

		// 表示時間
		constexpr Duration kFadeInDuration = 0.3s;
		constexpr Duration kTotalDisplayDuration = 2.0s;
		constexpr Duration kFadeOutDuration = 0.3s;
		constexpr Duration kTotalDuration = kTotalDisplayDuration + kFadeOutDuration;

		// 点滅
		constexpr Duration kBlinkInterval = 0.12s;
		constexpr Duration kBlinkOnDuration = kBlinkInterval / 2;
		constexpr Duration kBlinkEnabledDuration = 0.48s;

		constexpr int32 kYPosition = 402;
		constexpr int32 kXOffsetFromCenter = 184;

		double CalculateAlpha(double elapsedMs)
		{
			if (elapsedMs < 0.0)
			{
				return 0.0;
			}
			else if (elapsedMs < kFadeInDuration.count() * 1000.0)
			{
				return elapsedMs / (kFadeInDuration.count() * 1000.0);
			}
			else if (elapsedMs < kTotalDisplayDuration.count() * 1000.0)
			{
				return 1.0;
			}
			else if (elapsedMs < kTotalDuration.count() * 1000.0)
			{
				const double fadeOutElapsed = elapsedMs - kTotalDisplayDuration.count() * 1000.0;
				return 1.0 - (fadeOutElapsed / (kFadeOutDuration.count() * 1000.0));
			}
			else
			{
				return 0.0;
			}
		}

		bool ShouldBlink(double elapsedMs)
		{
			if (elapsedMs >= kTotalDisplayDuration.count() * 1000.0)
			{
				return false;
			}

			if (elapsedMs >= kBlinkEnabledDuration.count() * 1000.0)
			{
				return false;
			}

			const double intervalMs = kBlinkInterval.count() * 1000.0;
			const double blinkOnMs = kBlinkOnDuration.count() * 1000.0;

			// 周期内の位置で点滅のON/OFFを判定
			const double positionInInterval = std::fmod(elapsedMs, intervalMs);
			return positionInInterval < blinkOnMs;
		}
	}

	LaserApproachIndicator::LaserApproachIndicator(const kson::ChartData& chartData)
		: m_texture(kTextureFilename,
			{
				.row = 2,
				.column = 2,
				.sourceSize = Size{ 240, 240 },
			})
		, m_textureRegionNormalL(m_texture(0, 0))
		, m_textureRegionBlinkL(m_texture(0, 1))
		, m_textureRegionNormalR(m_texture(1, 0))
		, m_textureRegionBlinkR(m_texture(1, 1))
		, m_chartData(chartData)
	{
	}

	void LaserApproachIndicator::update(const GameStatus& gameStatus, const kson::TimingCache& timingCache)
	{
		const double currentTimeSec = gameStatus.currentTimeSec;

		for (std::size_t laneIdx = 0; laneIdx < kson::kNumLaserLanesSZ; ++laneIdx)
		{
			auto& state = m_states[laneIdx];
			const auto& laserLane = m_chartData.note.laser[laneIdx];

			const double scanStartTimeSec = currentTimeSec + kTriggerThreshold.count();
			const double scanEndTimeSec = currentTimeSec + kScanAheadTime.count();

			// トリガー済みセクションが終了したらリセット
			if (m_triggeredSections[laneIdx].has_value())
			{
				const kson::Pulse triggeredPulse = m_triggeredSections[laneIdx].value();
				const auto it = laserLane.find(triggeredPulse);

				if (it != laserLane.end())
				{
					const auto& section = it->second;
					if (!section.v.empty())
					{
						const auto lastPoint = section.v.rbegin();
						const kson::Pulse endPulse = triggeredPulse + lastPoint->first;
						const double laserEndTimeSec = kson::PulseToSec(endPulse, m_chartData.beat, timingCache);

						if (currentTimeSec > laserEndTimeSec)
						{
							m_triggeredSections[laneIdx].reset();
						}
					}
				}
			}

			// 新しい予告のトリガー判定
			if (!state.isActive && !m_triggeredSections[laneIdx].has_value())
			{
				for (const auto& [pulse, section] : laserLane)
				{
					const double laserStartTimeSec = kson::PulseToSec(pulse, m_chartData.beat, timingCache);

					if (laserStartTimeSec > scanEndTimeSec)
					{
						break;
					}

					if (laserStartTimeSec < scanStartTimeSec)
					{
						continue;
					}

					// 予告表示開始
					state.startTimeSec = currentTimeSec;
					state.isActive = true;
					m_triggeredSections[laneIdx] = pulse;
					break;
				}
			}

			// 予告表示終了判定
			if (state.isActive)
			{
				const double elapsedSec = currentTimeSec - state.startTimeSec;
				if (elapsedSec >= kTotalDuration.count())
				{
					state.isActive = false;
				}
			}
		}
	}

	void LaserApproachIndicator::draw(double currentTimeSec, Optional<double> anchorY) const
	{
		for (std::size_t i = 0; i < kson::kNumLaserLanesSZ; ++i)
		{
			const auto& state = m_states[i];

			if (!state.isActive)
			{
				continue;
			}

			const double elapsedSec = currentTimeSec - state.startTimeSec;
			const double elapsedMs = elapsedSec * 1000.0;

			const double alpha = CalculateAlpha(elapsedMs);
			if (alpha <= 0.0)
			{
				continue;
			}

			const bool shouldBlinkNow = ShouldBlink(elapsedMs);

			const TextureRegion& textureRegion = (i == 0)
				? (shouldBlinkNow ? m_textureRegionBlinkL : m_textureRegionNormalL)
				: (shouldBlinkNow ? m_textureRegionBlinkR : m_textureRegionNormalR);

			const int32 xOffset = (i == 0) ? -kXOffsetFromCenter : kXOffsetFromCenter;
			Vec2 pos = {
				GameRender::Width() / 2 + Scaled(xOffset),
				Scaled(kYPosition)
			};

			double verticalSize = 0;
			if (GameRender::VerticalEnabled())
			{
				const auto viewport = Vertical::Viewport::Fit(GameRender::Width(), GameRender::Height());
				const auto layout = Vertical::HUDDefinition()[U"laserApproach"];
				pos = {viewport.x(layout[i == 0 ? U"leftX" : U"rightX"].get<double>()),
					anchorY.value_or(GameRender::Height() * Vertical::ModConfig::Load().layout.judgmentLineY) -
					layout[U"offsetY"].get<double>() * viewport.scale};
				verticalSize = layout[U"size"].get<double>() * viewport.scale;
			}
			const ScopedRenderStates2D samplerState(SamplerState::ClampLinear);
			textureRegion
				.resized(verticalSize > 0 ? SizeF{verticalSize, verticalSize} : SizeF{Scaled(kIconSize)})
				.drawAt(pos, ColorF(1.0, alpha));
		}
	}
}
