#include "RuntimeConfig.hpp"
#include "Ini/ConfigIni.hpp"

namespace
{
	struct Config
	{
		GaugeType gaugeType = GaugeType::kNormalGauge;
        bool apsEnabled = false;
		TurnMode turnMode = TurnMode::kNormal;
		JudgmentPlayMode judgmentModeBT = JudgmentPlayMode::kOn;
		JudgmentPlayMode judgmentModeFX = JudgmentPlayMode::kOn;
		JudgmentPlayMode judgmentModeLaser = JudgmentPlayMode::kOn;
		double playbackSpeed = 1.0;
	};

	Config g_config;
}

namespace RuntimeConfig
{
	GaugeType GetGaugeType()
	{
		return g_config.gaugeType;
	}

	void SetGaugeType(GaugeType gaugeType)
	{
		g_config.gaugeType = gaugeType;
        g_config.apsEnabled = false;
	}

    bool GetApsEnabled() { return g_config.apsEnabled; }
    int32 GetGaugeSelection() { return static_cast<int32>(g_config.gaugeType) + (g_config.apsEnabled ? 1 : 0); }
    void SetGaugeSelection(int32 selection)
    {
        const int32 hard = static_cast<int32>(GaugeType::kHardGauge);
        g_config.gaugeType = static_cast<GaugeType>(Clamp(selection, static_cast<int32>(GaugeType::kEasyGauge), hard));
        g_config.apsEnabled = selection == hard + 1;
    }

    TurnMode GetTurnMode()
	{
		return g_config.turnMode;
	}

	void SetTurnMode(TurnMode turnMode)
	{
		g_config.turnMode = turnMode;
	}

	JudgmentPlayMode GetJudgmentPlayModeBT()
	{
		return g_config.judgmentModeBT;
	}

	void SetJudgmentPlayModeBT(JudgmentPlayMode mode)
	{
		g_config.judgmentModeBT = mode;
	}

	JudgmentPlayMode GetJudgmentPlayModeFX()
	{
		return g_config.judgmentModeFX;
	}

	void SetJudgmentPlayModeFX(JudgmentPlayMode mode)
	{
		g_config.judgmentModeFX = mode;
	}

	JudgmentPlayMode GetJudgmentPlayModeLaser()
	{
		return g_config.judgmentModeLaser;
	}

	void SetJudgmentPlayModeLaser(JudgmentPlayMode mode)
	{
		g_config.judgmentModeLaser = mode;
	}

	double GetPlaybackSpeed()
	{
		return g_config.playbackSpeed;
	}

	void SetPlaybackSpeed(double speed)
	{
		g_config.playbackSpeed = speed;
	}

	void RestoreJudgmentModesFromConfigIni()
	{
		g_config.judgmentModeBT = static_cast<JudgmentPlayMode>(ConfigIni::GetInt(ConfigIni::Key::kJudgmentModeBT, ConfigIni::Value::JudgmentMode::kOn));
		g_config.judgmentModeFX = static_cast<JudgmentPlayMode>(ConfigIni::GetInt(ConfigIni::Key::kJudgmentModeFX, ConfigIni::Value::JudgmentMode::kOn));
		g_config.judgmentModeLaser = static_cast<JudgmentPlayMode>(ConfigIni::GetInt(ConfigIni::Key::kJudgmentModeLaser, ConfigIni::Value::JudgmentMode::kOn));
	}
}

namespace RuntimeConfig {
FastSlowMode LoadFastSlowModeFromConfigIni(){return static_cast<FastSlowMode>(Clamp(ConfigIni::GetInt(ConfigIni::Key::kShowFastSlow,0),0,2));}
MusicGame::TimingHudSettings LoadTimingHudSettingsFromConfigIni(){return {
 .judgeColor=ConfigIni::GetInt(ConfigIni::Key::kJudgeColor,0)==1?JudgeColor::kFastRed:JudgeColor::kFastBlue,
 .position=static_cast<TimingPosition>(Clamp(ConfigIni::GetInt(ConfigIni::Key::kTimingPosition,0),0,3)),
 .display=static_cast<TimingMsDisplay>(Clamp(ConfigIni::GetInt(ConfigIni::Key::kTimingMsDisplay,3),0,3)),
 .color=static_cast<TimingMsColor>(Clamp(ConfigIni::GetInt(ConfigIni::Key::kTimingMsColor,2),0,3))};}
}
