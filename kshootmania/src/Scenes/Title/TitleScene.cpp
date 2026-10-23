#include "ksmaudio/Diagnostics.hpp"
#include "TitleScene.hpp"

#define NO_IR_ENTRY_SCENE // Note: ソースコード公開用にIR関連処理は削除済み

#ifndef NO_IR_ENTRY_SCENE
#include "Scenes/IREntry/IREntryScene.hpp"
#endif
#include "Input/InputUtils.hpp"
#include "Scenes/Select/SelectScene.hpp"
#include "Scenes/Option/OptionScene.hpp"
#include "Scenes/Common/ShowLoadingOneFrame.hpp"
#include "Vertical/VerticalDisplay.hpp"
#include "Vertical/ModOptionsScene.hpp"
#include "Vertical/VerticalResources.hpp"
#include "Vertical/OptionsPresentation.hpp"
#include "ModMenuButtons.hpp"
#include "Diagnostics/EnhancedMenuFixture.hpp"
#include "Diagnostics/SkinPresetFixture.hpp"

namespace
{
	constexpr Duration kFadeDuration = 0.5s;
	constexpr Duration kFadeDurationExit = 0.8s;

	FilePath GetTitleSceneUIFilePath()
	{
		return Vertical::UIPath(U"ui/scene/title.noco");
	}

	std::shared_ptr<noco::Canvas> LoadTitleSceneCanvas()
	{
		const FilePath uiFilePath = GetTitleSceneUIFilePath();
		const auto canvas = noco::Canvas::LoadFromFile(uiFilePath);
		if (!canvas)
		{
			throw Error{ U"Failed to load '{}'"_fmt(uiFilePath) };
		}
		return canvas;
	}
}

TitleScene::TitleScene(TitleMenuItem defaultMenuitem)
	: m_bgmStream(std::make_shared<ksmaudio::Stream>("se/title_bgm.ogg", 1.0, false, false, true)), m_canvas(LoadTitleSceneCanvas()), m_menu(defaultMenuitem, m_canvas)
{
	Vertical::OptionsPresentation::Reset();
	ksmaudio::qa::Scope qaConstruct{ "TitleScene.construct", true };
}

TitleScene::TitleScene(TitleMenuItem defaultMenuitem, std::shared_ptr<ksmaudio::Stream> bgmStream)
	: m_bgmStream(std::move(bgmStream)), m_canvas(LoadTitleSceneCanvas()), m_menu(defaultMenuitem, m_canvas)
{
}

Co::Task<void> TitleScene::start()
{
	const auto updateRunner = Co::UpdaterTask([this] { update(); }).runScoped();

	if (!m_bgmStream->isPlaying())
	{
		m_bgmStream->play();
	}
	AutoMuteAddon::SetEnabled(true);

	// メニューが選択されるまで待機
	m_selectedMenuItem = co_await m_menu.selectedMenuItemAsync();

	// 効果音を鳴らす
#ifdef NO_IR_ENTRY_SCENE
	m_bgmStream->setVolume(0.0);
#else
	if (m_selectedMenuItem != TitleMenuItem::kStart)
	{
		m_bgmStream->setVolume(0.0);
	}
#endif
	CommonSEAddon::Play(CommonSEType::kTitleEnter);
}

void TitleScene::openSkinOverlay()
{
	m_skinBackdrop = RenderTexture{ Scene::Size() };
	{
		const ScopedRenderTarget2D target{ m_skinBackdrop.clear(Palette::Black) };
		m_canvas->draw();
		ModMenuButtons::Draw(true, m_menu.focused(TitleMenuItem::kVerticalOptions));
		ModMenuButtons::DrawJudge(m_menu.focused(TitleMenuItem::kJudgeSetting));
		ModMenuButtons::Draw(false, m_menu.focused(TitleMenuItem::kCustom));
		Graphics2D::Flush();
	}
	m_bgmStream->pause();
	m_skinOverlay = std::make_unique<SkinOverlay>();
}

void TitleScene::update()
{
	ksmaudio::qa::Scope qaScope{ "TitleScene.update", false };
	if (m_judgeOverlay)
	{
		m_judgeOverlay->update();
		if (m_judgeOverlay->finished())
		{
			m_judgeOverlay.reset();
			m_bgmStream->setFadeIn(2s);
			m_bgmStream->play();
		}
		return;
	}
	if (m_skinOverlay)
	{
		m_skinOverlay->update();
		if (m_skinOverlay->finished())
		{
			m_skinOverlay.reset();
			m_canvas = LoadTitleSceneCanvas();
			m_menu.setCanvas(m_canvas);
			// Restore the paused title position with a gentle rise from silence.
			m_bgmStream->setFadeIn(2s);
			m_bgmStream->play();
			if (SkinFixture::Smoke())
			{
				m_returnBgmClock.restart();
				m_returnBgmTestStage = 1;
				TextWriter log{ FsUtils::GetResourcePath(U"title-bgm-return.csv") };
				log.writeln(U"phase,playing,fading");
				log.writeln(U"start,{},{}"_fmt(m_bgmStream->isPlaying() ? 1 : 0, m_bgmStream->isFading() ? 1 : 0));
				SkinFixture::Check(m_bgmStream->isPlaying() && m_bgmStream->isFading(), U"title resumes playing with a fade");
			}
			m_paletteFocused = false;
		}
		return;
	}
	if (SkinFixture::Smoke() && m_returnBgmTestStage > 0)
	{
		const bool mid = m_returnBgmTestStage == 1 && m_returnBgmClock.sF() >= .75;
		const bool done = m_returnBgmTestStage == 2 && m_returnBgmClock.sF() >= 2.15;
		if (mid || done)
		{
			TextWriter{ FsUtils::GetResourcePath(U"title-bgm-return.csv"), OpenMode::Append }.writeln(U"{},{},{}"_fmt(mid ? U"middle" : U"done", m_bgmStream->isPlaying() ? 1 : 0, m_bgmStream->isFading() ? 1 : 0));
			SkinFixture::Check(m_bgmStream->isPlaying() && (mid ? m_bgmStream->isFading() : !m_bgmStream->isFading()), U"title BGM fades over two seconds and remains playing");
			m_returnBgmTestStage = mid ? 2 : 0;
		}
	}
	if (!isFadingIn() && !isFadingOut())
	{
		if (SkinFixture::Smoke())
		{
			if (!m_fixtureOpened)
			{
				m_fixtureOpened = true;
				openSkinOverlay();
				return;
			}
			if (m_fixtureClock.sF() > 41.0)
			{
				ScreenCapture::SaveCurrentFrame(U"title-restored.png");
				TextWriter{ FsUtils::GetResourcePath(U"smoke-passed.txt") }.writeln(U"Overlay closed, title restored");
				if (m_fixtureClock.sF() > 42.0)
					System::Exit();
			}
		}
	}
	if (SkinFixture::Smoke())
	{
		m_canvas->update(noco::HitTestEnabledYN::No);
		return;
	}
	m_menu.update();
	if (!isFadingIn() && !isFadingOut())
		EnhancedMenuFixture::Title(m_menu);
	if (m_menu.takeJudgeRequest())
	{
		m_bgmStream->pause();
		m_judgeOverlay = std::make_unique<JudgeSettingOverlay>();
		return;
	}
	if (m_menu.takeCustomRequest())
	{
		openSkinOverlay();
		return;
	}

	m_canvas->update(noco::HitTestEnabledYN{ !isFadingIn() && !isFadingOut() && InputUtils::IsUIMouseInputEnabled() });
}

void TitleScene::draw() const
{
	if (m_judgeOverlay)
	{
		m_judgeOverlay->draw();
		return;
	}
	if (m_skinOverlay)
	{
		m_skinBackdrop.draw();
		m_skinOverlay->draw();
	}
	else
	{
		m_canvas->draw();
		ModMenuButtons::Draw(false, m_menu.focused(TitleMenuItem::kCustom));
		ModMenuButtons::Draw(true, m_menu.focused(TitleMenuItem::kVerticalOptions));
		ModMenuButtons::DrawJudge(m_menu.focused(TitleMenuItem::kJudgeSetting));
	}
}

Co::Task<void> TitleScene::fadeIn()
{
	const auto canvasUpdateRunner = Co::UpdaterTask([this] { m_canvas->update(noco::HitTestEnabledYN::No); }).runScoped();

	co_await Co::ScreenFadeIn(kFadeDuration);
}

Co::Task<void> TitleScene::fadeOut()
{
	const auto canvasUpdateRunner = Co::UpdaterTask([this]
													{
														m_canvas->update(noco::HitTestEnabledYN::No);
													})
										.runScoped();

	// 次のシーンへ遷移
	switch (m_selectedMenuItem)
	{
	case TitleMenuItem::kStart:
		co_await Co::ScreenFadeOut(kFadeDuration);
#ifdef NO_IR_ENTRY_SCENE
		requestNextScene<SelectScene>();
		// SelectSceneはコンストラクタの処理に時間がかかるので、ローディングはここで出しておく
		co_await ShowLoadingOneFrame::Play(LoadingBgMode::kBlack);
#else
		requestNextScene<IREntryScene>(m_bgmStream);
#endif
		break;

	case TitleMenuItem::kOption:
		co_await Co::ScreenFadeOut(kFadeDuration);
		requestNextScene<OptionScene>();
		break;

	case TitleMenuItem::kInputGate:
		// Note: ソースコード公開用にINPUT GATE関連処理は削除済み
		co_await Co::ScreenFadeOut(kFadeDuration);
		MessageBoxUtils::ShowOK(U"This version does not support INPUT GATE.");
		requestNextScene<TitleScene>(TitleMenuItem::kInputGate);
		break;

	case TitleMenuItem::kExit:
		// 効果音が途中で切れないよう終了時は少し長めにフェードアウト
		co_await Co::ScreenFadeOut(kFadeDurationExit);
		break;

	case TitleMenuItem::kVerticalOptions:
		Vertical::OptionsPresentation::Backdrop() = RenderTexture{ Scene::Size() };
		{
			const ScopedRenderTarget2D target{ Vertical::OptionsPresentation::Backdrop().clear(Palette::Black) };
			draw();
			Graphics2D::Flush();
		}
		requestNextScene<ModOptionsScene>();
		break;

	default:
		throw Error{ U"Invalid menu item: {}"_fmt(static_cast<std::underlying_type_t<TitleMenuItem>>(m_selectedMenuItem)) };
	}
}
