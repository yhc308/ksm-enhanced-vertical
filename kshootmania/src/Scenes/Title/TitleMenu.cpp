#include "TitleMenu.hpp"
#include "ModMenuButtons.hpp"
#include "Vertical/VerticalDisplay.hpp"
#include "UI/MouseMenuUtils.hpp"

namespace
{
	// STARTボタンを無視する時間の長さ
	constexpr Duration kStartIgnoreDuration = 0.1s;

	// クリックイベントのタグとメニュー項目の対応関係
	const Array<std::pair<String, int32>> kClickTagToMenuItemPairs = {
		{ U"onClickStart", TitleMenuItem::kStart },
		{ U"onClickOption", TitleMenuItem::kOption },
		{ U"onClickInputGate", TitleMenuItem::kInputGate },
		{ U"onClickExit", TitleMenuItem::kExit },
	};
}

void TitleMenu::refreshCanvasMenuCursor()
{
	m_titleSceneCanvas->setParamValue(U"menuCursor", m_menu.cursor()-TitleMenuItem::kStart);
    if(const auto cursor=m_titleSceneCanvas->findByName(U"MenuCursor"))cursor->setActive(m_menu.cursor()>=TitleMenuItem::kStart);
}

TitleMenu::TitleMenu(TitleMenuItem defaultMenuitem, const std::shared_ptr<noco::Canvas>& titleSceneCanvas)
	: m_menu(
		LinearMenu::CreateInfoWithEnumCount
		{
			.cursorInputCreateInfo = {
				.type = CursorInput::Type::Vertical,
				.buttonFlags = CursorButtonFlags::kArrowOrBTAllOrLaserAll | CursorButtonFlags::kMouseWheel,
			},
			.enumCount = TitleMenuItem::kItemEnumCount,
			.cyclic = IsCyclicMenuYN::No,
			.defaultCursor = defaultMenuitem,
		})
	, m_titleSceneCanvas(titleSceneCanvas)
{
	refreshCanvasMenuCursor();
}

bool TitleMenu::hoverAt(const Vec2& point){
    const int32 next=ModMenuButtons::HoveredIndex(point);
    if(next==m_lastModHover)return false;
    m_lastModHover=next;
    if(next<0)return false;
    m_menu.setCursor(next);m_hoverSe.play();refreshCanvasMenuCursor();return true;
}

void TitleMenu::update()
{
    if(Vertical::IsDisplayChangePending()) return;
	// カーソル変化検出用
	// (LinearMenu::deltaCursor() != 0 だとsetCursorでの直接指定による変化を検出できないため、cursor値の比較を利用している)
	const auto beforeCursor = m_menu.cursor();

	const bool backPressed = !m_isAlreadySelected && KeyConfig::Down(kButtonBack);
	bool clickDecided = false;
    bool hoveredSelection=false;
	if (!m_isAlreadySelected && !backPressed)
	{
		m_menu.update();
        if(Window::GetState().focused&&InputUtils::IsUIMouseInputEnabled()&&!System::GetCommandLineArgs().contains(U"--enhanced-menu-test"))hoveredSelection=hoverAt(Cursor::PosF());

		// クリックされた項目へカーソルを移動して決定
		Optional<int32> clickedItem = MouseMenuUtils::FiredClickIndex(*m_titleSceneCanvas, kClickTagToMenuItemPairs);
		if(ModMenuButtons::JudgeClicked()) clickedItem=TitleMenuItem::kJudgeSetting;
        else if(ModMenuButtons::Clicked(true)) clickedItem=TitleMenuItem::kVerticalOptions;
        else if(ModMenuButtons::Clicked(false)) clickedItem=TitleMenuItem::kCustom;
        clickDecided = clickedItem.has_value();
		if (clickedItem.has_value())
		{
			m_menu.setCursor(*clickedItem);
		}

		if (m_stopwatch.elapsed() >= kStartIgnoreDuration && (KeyConfig::Down(kButtonStart) || clickDecided))
		{
			const auto selectedItem = m_menu.cursorAs<TitleMenuItem>();
			if(selectedItem==TitleMenuItem::kJudgeSetting){m_judgeRequested=true;m_stopwatch.restart();}
            else if(selectedItem==TitleMenuItem::kCustom){m_customRequested=true;m_stopwatch.restart();}
            else {m_selectedMenuItemSource.requestFinish(selectedItem);m_isAlreadySelected=true;}
		}
	}

	// BackボタンでEXITへフォーカス
	if (!m_isAlreadySelected && backPressed)
	{
		m_menu.setCursor(kExit);
	}

	if (m_menu.cursor() != beforeCursor)
	{
		// カーソル位置が変化した場合はCanvasへ反映し、クリック決定時以外は効果音を鳴らす
		if (!clickDecided && !hoveredSelection)
		{
			m_selectSe.play();
		}
		refreshCanvasMenuCursor();
	}
}
