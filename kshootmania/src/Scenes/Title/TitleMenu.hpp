#pragma once
#include <CoTaskLib.hpp>
#include "TitleAssets.hpp"
#include "Graphics/TiledTexture.hpp"
#include "UI/LinearMenu.hpp"

enum TitleMenuItem : int32
{
	kJudgeSetting = 0,
    kVerticalOptions,
    kCustom,
    kStart,
	kOption,
	kInputGate,
	kExit,

	kItemEnumCount,
};

class TitleMenu
{
private:
	bool m_isAlreadySelected = false;
    bool m_customRequested=false,m_judgeRequested=false;

	LinearMenu m_menu;

	std::shared_ptr<noco::Canvas> m_titleSceneCanvas;

	Stopwatch m_stopwatch { StartImmediately::Yes };

	ksmaudio::Sample m_selectSe{ "se/title_sel.wav" };
    ksmaudio::Sample m_hoverSe{"se/hover.wav"};
    int32 m_lastModHover=-1;

	Co::TaskFinishSource<TitleMenuItem> m_selectedMenuItemSource;

	void refreshCanvasMenuCursor();

public:
	explicit TitleMenu(TitleMenuItem defaultMenuitem, const std::shared_ptr<noco::Canvas>& titleSceneCanvas);

	void update();
    bool hoverAt(const Vec2& point);
    bool focused(TitleMenuItem item) const {return m_menu.cursor()==item;}
    bool takeJudgeRequest(){return std::exchange(m_judgeRequested,false);}
    bool takeCustomRequest(){return std::exchange(m_customRequested,false);}
    void setCanvas(const std::shared_ptr<noco::Canvas>& canvas) { m_titleSceneCanvas = canvas; refreshCanvasMenuCursor(); }

	Co::Task<TitleMenuItem> selectedMenuItemAsync()
	{
		return m_selectedMenuItemSource.waitForResult();
	}
};
