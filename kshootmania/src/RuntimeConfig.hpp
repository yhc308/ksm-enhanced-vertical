#pragma once
#include "Common/CommonDefines.hpp"
#include "MusicGame/TimingFeedback.hpp"

// 実行中のみ保持する設定
// ゲージ選択のみ、保存設定が有効な場合はconfig.iniから復元する
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
	void RestoreGaugeSelectionFromConfigIni();
	void StoreGaugeSelectionToConfigIni();
	void SetRememberEffRate(bool enabled);

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
