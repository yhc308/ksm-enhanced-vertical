#include "CommonOverlay.hpp"
#include "Input/KeyConfig.hpp"

namespace
{
	std::shared_ptr<noco::Canvas> LoadCommonOverlayCanvas()
	{
		const FilePath uiFilePath = FsUtils::GetResourcePath(U"ui/scene/common_overlay.noco");
		const auto canvas = noco::Canvas::LoadFromFile(uiFilePath);
		if (!canvas)
		{
			throw Error{ U"Failed to load '{}'"_fmt(uiFilePath) };
		}
		return canvas;
	}
}

CommonOverlay::CommonOverlay()
	: m_canvas(LoadCommonOverlayCanvas())
{
}

CommonOverlay::CommonOverlay(const Duration& backButtonClickHoldDuration)
	: m_canvas(LoadCommonOverlayCanvas())
	, m_backButtonClickHoldDuration(backButtonClickHoldDuration)
{
}

bool CommonOverlay::update(noco::HitTestEnabledYN hitTestEnabled,bool dispatchVirtualBack)
{
    bool clicked=false;
    // Reuse the native hover appearance for keyboard/controller focus without moving the mouse.
    if(const auto back=m_canvas->findByName(U"BackButton")){
        if(m_keyboardFocused){back->transform().color().setCurrentFrameOverride(Palette::White);back->transform().translate().setCurrentFrameOverride(Vec2::Zero());}
        else{back->transform().color().clearCurrentFrameOverride();back->transform().translate().clearCurrentFrameOverride();}
    }
    m_canvas->update(hitTestEnabled);

	if (m_canvas->isEventFiredWithTag(U"onPressStartBackButton"))
	{
		m_backButtonClickPressed = true;
	}
	if (m_canvas->isEventFiredWithTag(U"onPressEndBackButton"))
	{
		m_backButtonClickPressed = false;
	}

	if (m_backButtonClickHoldDuration.has_value())
	{
		// 長押しモードでは押下中にゲージが進行し、完了した場合のみBack扱いにする
		if (m_backButtonClickPressed && m_backButtonProgress < 1.0)
		{
			const double holdDurationSec = m_backButtonClickHoldDuration->count();
			m_backButtonClickHoldSec += Scene::DeltaTime();
			m_backButtonProgress = holdDurationSec <= 0.0 ? 1.0 : Min(m_backButtonClickHoldSec / holdDurationSec, 1.0);
			if (m_backButtonProgress >= 1.0)
			{
				clicked=true;
                if(dispatchVirtualBack)KeyConfig::RequestVirtualDown(kButtonBack);
			}
		}
		else if (!m_backButtonClickPressed)
		{
			// 途中で離した場合はゲージを戻す
			m_backButtonClickHoldSec = 0.0;
			m_backButtonProgress = 0.0;
		}
	}
	else
	{
		// クリックモードではゲージ表示は使用しない
		m_backButtonProgress = 0.0;

		// Backボタンのクリックをボタン入力と同じ効果にする
		if (m_canvas->isEventFiredWithTag(U"onClickBackButton"))
		{
			clicked=true;
                if(dispatchVirtualBack)KeyConfig::RequestVirtualDown(kButtonBack);
		}
	}

	m_canvas->setParamValue(U"backButtonProgress", m_backButtonProgress);
    return clicked;
}

void CommonOverlay::draw() const
{
	m_canvas->draw();
}

void CommonOverlay::setBackButtonVisible(bool visible)
{
	m_canvas->setParamValue(U"backButtonVisible", visible);
}

void CommonOverlay::setBackButtonClickHoldDuration(const Optional<Duration>& holdDuration)
{
	m_backButtonClickHoldDuration = holdDuration;

	// ゲージ状態をリセット
	m_backButtonClickHoldSec = 0.0;
	m_backButtonProgress = 0.0;
}

void CommonOverlay::setModScreenLayout(){
    // Match the original OPTION overlay: 48 logical pixels at scene-height / 480.
    const double scale=Scene::Height()/480.0;
    m_canvas->setReferenceSize(SizeF{Scene::Width()/scale,Scene::Height()/scale});
    m_canvas->setAutoFitMode(noco::AutoFitMode::FitHeightMatchWidth);
}
