#include "Graphics/GameRenderContext.hpp"
#include "KeyBeamGraphics.hpp"
#include "Common/SkinPreset.hpp"
#include "MusicGame/Graphics/GraphicsDefines.hpp"
#include "Vertical/VerticalLaneGeometry.hpp"
#include "MusicGame/Camera/CameraMath.hpp"

namespace MusicGame::Graphics
{
	namespace
	{
		constexpr StringView kKeyBeamTextureFilename = U"judge.gif";

		constexpr Size kBTKeyBeamTextureSize = { 40, 300 };
		constexpr Size kFXKeyBeamTextureSize = { 82, 300 };

		constexpr double kKeyBeamFullWidthSec = 0.075;
		constexpr double kKeyBeamEndSec = 0.155;
		constexpr Vec2 kKeyBeamPositionOffset = kLanePositionOffset + Vec2{ 0.0, kHighwayTextureSize.y - 300.0 };
	}

	KeyBeamGraphics::KeyBeamGraphics()
		: m_beamTexture(TextureAsset(kKeyBeamTextureFilename))
        , m_perfectBeamTexture(TextureAsset(U"judge_p.gif"))
	{
	}

	void KeyBeamGraphics::draw(const GameStatus& gameStatus, const ViewStatus& viewStatus, const HighwayRenderTexture& target) const
	{
		const ScopedRenderTarget2D renderTarget(target.additiveTexture());
        const Transformer2D layout{Mat3x2::Scale(GameRender::VerticalEnabled() ? Vertical::kButtonWidthScale : 1.0,1.0,Vec2{static_cast<double>(kHighwayTextureWideCenterX),0})};
		const ScopedRenderStates2D renderState(BlendState::Additive);

		for (std::size_t i = 0; i < kson::kNumBTLanesSZ + kson::kNumFXLanesSZ; ++i)
		{
			const bool isBT = (i < kson::kNumBTLanesSZ);
			const std::size_t numLanes = isBT ? kson::kNumBTLanesSZ : kson::kNumFXLanesSZ;
			const std::size_t laneIdx = isBT ? i : (i - kson::kNumBTLanesSZ);
			const double centerSplitShiftX = Camera::CenterSplitShiftX(viewStatus.camStatus.centerSplit) * ((laneIdx >= numLanes / 2) ? 1 : -1);
			const ButtonLaneStatus& laneStatus = isBT ? gameStatus.btLaneStatus[laneIdx] : gameStatus.fxLaneStatus[laneIdx];

			const double sec = gameStatus.currentTimeSec - laneStatus.keyBeamTimeSec;
			if (sec < 0.0 || kKeyBeamEndSec < sec)
			{
				continue;
			}

			double widthRate = 1.0;
			double alpha = 1.0;
			if (sec < kKeyBeamFullWidthSec)
			{
				widthRate = sec / kKeyBeamFullWidthSec;
			}
			else
			{
				alpha = 1.0 - (sec - kKeyBeamFullWidthSec) / (kKeyBeamEndSec - kKeyBeamFullWidthSec);
			}

            const auto displayType=SkinPreset::DisplayBeam(laneStatus.keyBeamType,SkinPreset::Current().keyBeamMode);
            if(!displayType)continue;
            const bool perfect = *displayType == Judgment::KeyBeamType::kPerfectCritical;
            const Texture& texture = perfect ? m_perfectBeamTexture : m_beamTexture;
            const int32 column = perfect ? 0 : static_cast<int32>(*displayType);
			const TextureRegion beamTextureRegion = texture(
				kBTKeyBeamTextureSize.x * (static_cast<double>(column) + 0.5 - widthRate / 2),
				0,
				kBTKeyBeamTextureSize.x * widthRate,
				kBTKeyBeamTextureSize.y);

			const double dLaneIdx = static_cast<double>(laneIdx);

			if (isBT)
			{
				beamTextureRegion
					.draw(kKeyBeamPositionOffset + Vec2::Right(centerSplitShiftX) + kBTLanePositionDiff * (dLaneIdx + 0.5 - widthRate / 2), ColorF{ 1.0, alpha });
			}
			else
			{
				beamTextureRegion
					.resized(kFXKeyBeamTextureSize.x * widthRate, kFXKeyBeamTextureSize.y)
					.draw(kKeyBeamPositionOffset + Vec2::Right(centerSplitShiftX) + kFXLanePositionDiff * dLaneIdx + kFXKeyBeamTextureSize * (0.5 - widthRate / 2), ColorF{ 1.0, alpha });
			}
		}
	}
}
