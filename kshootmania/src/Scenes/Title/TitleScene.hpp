#pragma once
#include <CoTaskLib.hpp>
#include "TitleMenu.hpp"
#include "TitleAssets.hpp"
#include "Scenes/Title/SkinOverlay.hpp"
#include "JudgeSettingOverlay.hpp"
#include "ksmaudio/ksmaudio.hpp"

class TitleScene : public Co::SceneBase
{
private:
	std::shared_ptr<ksmaudio::Stream> m_bgmStream;

	std::shared_ptr<noco::Canvas> m_canvas;

    TitleMenu m_menu;
    std::unique_ptr<SkinOverlay> m_skinOverlay;
    std::unique_ptr<JudgeSettingOverlay> m_judgeOverlay;
    RenderTexture m_skinBackdrop;
    bool m_paletteFocused = false;
    bool m_fixtureOpened = false;
    Stopwatch m_fixtureClock{StartImmediately::Yes};
    Stopwatch m_returnBgmClock;
    int m_returnBgmTestStage=0;
    void openSkinOverlay();

	TitleMenuItem m_selectedMenuItem = TitleMenuItem::kStart;

public:
	explicit TitleScene(TitleMenuItem defaultMenuitem);

	TitleScene(TitleMenuItem defaultMenuitem, std::shared_ptr<ksmaudio::Stream> bgmStream);

	virtual Co::Task<void> start() override;

	void update();

	virtual void draw() const override;

	virtual Co::Task<void> fadeIn() override;

	virtual Co::Task<void> fadeOut() override;
};
