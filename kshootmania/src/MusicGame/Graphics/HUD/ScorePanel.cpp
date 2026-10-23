#include "Graphics/GameRenderContext.hpp"
#include "ScorePanel.hpp"

namespace MusicGame::Graphics
{
	namespace
	{
		constexpr StringView kCaptionTextureFilename = U"score_header.png";
		constexpr StringView kNumberTextureFontFilename = U"result_scorenum.png";
	}

	ScorePanel::ScorePanel()
		//: m_captionTexture(TextureAsset(kCaptionTextureFilename))
		: m_captionTexture(SkinPreset::LoadTexture(kCaptionTextureFilename))
		, m_numberTextureFont(kNumberTextureFontFilename, { 64, 64 })
		, m_numberLayout(Scaled(24, 24), TextureFontTextLayout::Align::Right, 8, Scaled(22))
		, m_animationTimer(kAnimationDuration, StartImmediately::No)
	{
	}

	void ScorePanel::update(int32 score)
	{
		if (m_targetScore != score)
		{
			m_startScore = m_displayedScore;
			m_targetScore = score;
			m_animationTimer.restart();
		}

		if (m_animationTimer.isRunning() && m_animationTimer < kAnimationDuration)
		{
			const double progress = m_animationTimer.sF() / kAnimationDuration.count();
			m_displayedScore = static_cast<int32>(m_startScore + static_cast<double>(m_targetScore - m_startScore) * progress);
		}
		else
		{
			m_displayedScore = m_targetScore;
		}
	}

	void ScorePanel::draw() const
	{
		m_captionTexture.resized(Scaled(240, 24)).draw(GameRender::Width() / 2 + Scaled(60), Scaled(16));
		m_numberTextureFont.draw(m_numberLayout, { GameRender::Width() / 2 + Scaled(92), Scaled(42) }, m_displayedScore, ZeroPaddingYN::Yes);
	}
}
