#pragma once
#include "TimingFeedback.hpp"
#include "JudgeSettingSession.hpp"
#include "Scroll/HispeedSetting.hpp"

namespace MusicGame
{
	// テストプレイ用オプション
	struct TestPlayOption
	{
		Optional<GaugeType> gaugeType;
		Optional<int32> startMeasure;

		bool hasStartMeasure() const
		{
			return startMeasure.has_value() && *startMeasure > 0;
		}
	};

	struct PlayOption
	{
		// Observation/replay sessions must never create normal score records.
		bool diagnosticSession = false;
        bool skinPreview = false;
        std::shared_ptr<JudgeSettingSession> judgeSetting;
        FastSlowMode liveFastSlowMode() const {return judgeSetting?judgeSetting->values.mode:fastSlowMode;}
        const TimingHudSettings& liveTimingHud() const {return judgeSetting?judgeSetting->values.hud:timingHud;}
		GameMode gameMode = GameMode::kNormal;

		IsAutoPlayYN isAutoPlay = IsAutoPlayYN::No;

		GaugeType gaugeType = GaugeType::kNormalGauge;
        bool apsEnabled = false;

		TurnMode turnMode = TurnMode::kNormal;

		double playbackSpeed = 1.0;

		JudgmentPlayMode btJudgmentPlayMode = JudgmentPlayMode::kOn;

		JudgmentPlayMode fxJudgmentPlayMode = JudgmentPlayMode::kOn;

		JudgmentPlayMode laserJudgmentPlayMode = JudgmentPlayMode::kOn;

		int32 globalOffsetMs = 0;

		int32 inputDelayMs = 0;

		int32 laserInputDelayMs = 0;

		int32 audioProcDelayMs = 0; // v1のsoundfx_delayにあたる

		int32 visualOffsetMs = 0;

		bool isAutoPlaySE = false; // FXチップ・直角の効果音のみオートプレイ

		NoteSkinType noteSkin = NoteSkinType::kDefault;

		AutoSyncMode autoSyncMode = AutoSyncMode::kOff;

		FastSlowMode fastSlowMode = FastSlowMode::kHide;

        TimingHudSettings timingHud;

		Array<HispeedType> availableHispeedTypes = { HispeedType::OMod };

		HispeedSetting hispeedSetting = HispeedSetting{ .type = HispeedType::OMod, .value = 500 };

		bool movieEnabled = true;

		bool showBG = true;

		bool showLayer = true;

		Optional<TestPlayOption> testPlayOption;

		// テストプレイで開始小節が指定されているか
		bool isTestPlayWithStartMeasure() const
		{
			return testPlayOption.has_value() && testPlayOption->hasStartMeasure();
		}

		// オートプレイを考慮したBT判定モードを取得
		JudgmentPlayMode effectiveBtJudgmentPlayMode() const
		{
			return isAutoPlay ? JudgmentPlayMode::kAuto : btJudgmentPlayMode;
		}

		// オートプレイを考慮したFX判定モードを取得
		JudgmentPlayMode effectiveFxJudgmentPlayMode() const
		{
			return isAutoPlay ? JudgmentPlayMode::kAuto : fxJudgmentPlayMode;
		}

		// オートプレイを考慮したLASER判定モードを取得
		JudgmentPlayMode effectiveLaserJudgmentPlayMode() const
		{
			return isAutoPlay ? JudgmentPlayMode::kAuto : laserJudgmentPlayMode;
		}

		// visualOffsetを考慮したglobalOffsetを取得
		int32 effectiveGlobalOffsetMs() const
		{
			return globalOffsetMs - (judgeSetting?judgeSetting->values.visual:visualOffsetMs);
		}

		// visualOffsetを考慮したinputDelayを取得(オートプレイ時は判定調整なし)
		int32 effectiveInputDelayMs() const
		{
			if (isAutoPlay)
			{
				return visualOffsetMs;
			}
			else
			{
				return judgeSetting ? judgeSetting->values.input+judgeSetting->values.visual : inputDelayMs+visualOffsetMs;
			}
		}

		// オートプレイ時は判定調整なし
		int32 effectiveLaserInputDelayMs() const
		{
			return isAutoPlay ? 0 : (judgeSetting?judgeSetting->values.laser:laserInputDelayMs);
		}

		// visualOffsetを考慮したオーディオエフェクト遅延を取得
		int32 effectiveAudioProcDelayMs() const
		{
			return audioProcDelayMs + (judgeSetting?judgeSetting->values.visual:visualOffsetMs);
		}

		bool isAssist() const
		{
			return btJudgmentPlayMode != JudgmentPlayMode::kOn ||
				fxJudgmentPlayMode != JudgmentPlayMode::kOn ||
				laserJudgmentPlayMode != JudgmentPlayMode::kOn;
		}

		// スコアを保存すべきかどうか
        bool effectiveApsEnabled() const
        {
            return apsEnabled && gaugeType == GaugeType::kHardGauge && gameMode == GameMode::kNormal &&
                !judgeSetting && !skinPreview && !diagnosticSession && !testPlayOption.has_value();
        }

        bool shouldSaveScore() const
		{
			return !judgeSetting && !skinPreview && !diagnosticSession && !isAutoPlay && !testPlayOption.has_value();
		}

		// playbackSpeed取得(ゼロ除算対策用に0の場合は1を返す)
		double nonZeroPlaybackSpeed() const
		{
			return playbackSpeed != 0.0 ? playbackSpeed : 1.0;
		}
	};
}
