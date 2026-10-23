#pragma once
#include "Common/CommonDefines.hpp"
#include "MusicGame/TimingFeedback.hpp"

// 実行中のみ保持する設定
// config.iniには保存されず、起動時にデフォルト値にリセットされる
namespace RuntimeConfig
{
    FastSlowMode LoadFastSlowModeFromConfigIni();
    MusicGame::TimingHudSettings LoadTimingHudSettingsFromConfigIni();
	[[nodiscard]]
	GaugeType GetGaugeType();

	void SetGaugeType(GaugeType gaugeType);
    bool GetApsEnabled();
    void SetGaugeSelection(int32 selection);
    int32 GetGaugeSelection();

	[[nodiscard]]
	TurnMode GetTurnMode();

	void SetTurnMode(TurnMode turnMode);

	[[nodiscard]]
	JudgmentPlayMode GetJudgmentPlayModeBT();

	void SetJudgmentPlayModeBT(JudgmentPlayMode mode);

	[[nodiscard]]
	JudgmentPlayMode GetJudgmentPlayModeFX();

	void SetJudgmentPlayModeFX(JudgmentPlayMode mode);

	[[nodiscard]]
	JudgmentPlayMode GetJudgmentPlayModeLaser();

	void SetJudgmentPlayModeLaser(JudgmentPlayMode mode);

	[[nodiscard]]
	double GetPlaybackSpeed();

	void SetPlaybackSpeed(double speed);

	// config.iniから判定モードを復元
	void RestoreJudgmentModesFromConfigIni();
}
