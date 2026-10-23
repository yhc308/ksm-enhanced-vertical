#pragma once

/// @brief 全画面共通のオーバーレイ表示(Backボタン)
class CommonOverlay
{
private:
	std::shared_ptr<noco::Canvas> m_canvas;

	/// @brief Backボタンクリックを長押しで反応させる場合の必要時間(noneの場合は即時反応)
	Optional<Duration> m_backButtonClickHoldDuration = none;

	bool m_backButtonClickPressed = false;
    bool m_keyboardFocused=false;

	/// @brief Backボタンクリックの押下継続時間(秒)
	double m_backButtonClickHoldSec = 0.0;

	/// @brief Backボタンのゲージ進捗(0〜1)
	double m_backButtonProgress = 0.0;

public:
	CommonOverlay();

	explicit CommonOverlay(const Duration& backButtonClickHoldDuration);

	bool update(noco::HitTestEnabledYN hitTestEnabled,bool dispatchVirtualBack=true);

    void setModScreenLayout();
    void setBackButtonFocused(bool focused){m_keyboardFocused=focused;}
    bool backButtonFocused() const{return m_keyboardFocused;}

	void draw() const;

	// Match the original 640x480 selection canvas, including mouse hit testing.
	void setContainLayout() { m_canvas->setAutoFitMode(noco::AutoFitMode::Contain); }

	void setBackButtonVisible(bool visible);

	/// @brief Backボタンクリックの長押し必要時間を切り替える(noneの場合は長押しなし)
	void setBackButtonClickHoldDuration(const Optional<Duration>& holdDuration);
};
