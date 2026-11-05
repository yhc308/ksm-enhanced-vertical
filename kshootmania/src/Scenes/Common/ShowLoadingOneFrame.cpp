#include "ShowLoadingOneFrame.hpp"
#include "Vertical/VerticalModConfig.hpp"
#include "Vertical/VerticalLayout.hpp"

ShowLoadingOneFrame::ShowLoadingOneFrame(LoadingBgMode bgMode)
	: m_bgMode(bgMode), m_vertical(Vertical::ModConfig::Load().enabled),
	  m_bgTexture(TextureAsset(m_vertical ? U"bg2_vertical.png" : CommonTexture::kMainBG))
{
}

Co::Task<void> ShowLoadingOneFrame::start()
{
	co_await Co::NextFrame();
}

void ShowLoadingOneFrame::draw() const
{
	switch (m_bgMode)
	{
	case LoadingBgMode::kMainBg:
		if (m_vertical)
		{
			const auto view = Vertical::Viewport::Fit(Scene::Width(), Scene::Height());
			const Transformer2D transform{ Mat3x2::Identity(), Transformer2D::Target::SetLocal };
			Scene::Rect().draw(Palette::Black);
			m_bgTexture.fitted(720 * view.scale, 1280 * view.scale).drawAt(Scene::Center());
		}
		else
		{
			FitToHeight(m_bgTexture).drawAt(Scene::Center());
		}
		break;
	case LoadingBgMode::kBlack:
		{
			const Transformer2D transform{ Mat3x2::Identity(), Transformer2D::Target::SetLocal };
			Scene::Rect().draw(Palette::Black);
		}
		break;
	case LoadingBgMode::kNone:
		break;
	}

	m_loadingTexture.resized(Scaled(200, 30)).draw(Scene::Width() - Scaled(208), Scaled(442));
}

Co::Task<void> ShowLoadingOneFrame::Play(LoadingBgMode bgMode)
{
	return Co::Play<ShowLoadingOneFrame>(bgMode);
}
