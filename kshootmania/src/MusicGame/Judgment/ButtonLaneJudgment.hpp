#pragma once
#include "MusicGame/GameDefines.hpp"
#include "MusicGame/GameStatus.hpp"
#include "MusicGame/Judgment/JudgmentHandler.hpp"
#include "kson/ChartData.hpp"
#include "kson/Util/TimingUtils.hpp"

namespace MusicGame::Judgment
{
	class ButtonLaneJudgment
	{
	public:
		struct LongNoteJudgment
		{
			kson::RelPulse length = kson::RelPulse{ 0 };

			JudgmentResult result = JudgmentResult::kUnspecified;
		};

	private:
		const JudgmentPlayMode m_judgmentPlayMode;
        const bool m_skinPreview;
		const GaugeType m_gaugeType;
		FastSlowMode m_fastSlowMode;
		const Button m_keyConfigButton;
		const std::map<kson::Pulse, double> m_pulseToSec;

		bool m_isLockedForExit = false;

		kson::ByPulse<JudgmentResult> m_chipJudgmentArray;
		kson::ByPulse<LongNoteJudgment> m_longJudgmentArray;

		kson::Pulse m_prevPulse = kPastPulse;

		kson::ByPulse<kson::Interval>::const_iterator m_passedNoteCursor;
        kson::ByPulse<kson::Interval>::const_iterator m_settingsSkipNoteCursor;
		kson::ByPulse<LongNoteJudgment>::iterator m_passedLongJudgmentCursor;

        void finishChipVisual(kson::Pulse pulse,ChipAnimType type,double time,double drawTime,ButtonLaneStatus& status);
		double errorWindowSec() const;

		void processKeyDown(const kson::ChartData& chartData, const kson::ByPulse<kson::Interval>& lane, kson::Pulse currentPulse, double currentTimeSec, double currentTimeSecForDraw, ButtonLaneStatus& laneStatusRef, JudgmentHandler& judgmentHandlerRef);

		void processKeyPressed(const kson::ByPulse<kson::Interval>& lane, kson::Pulse currentPulse, const ButtonLaneStatus& laneStatusRef, JudgmentHandler& judgmentHandlerRef);

		void processPassedNoteJudgment(const kson::ByPulse<kson::Interval>& lane, kson::Pulse currentPulse, double currentTimeSec, double currentTimeSecForDraw, ButtonLaneStatus& laneStatusRef, JudgmentHandler& judgmentHandlerRef, IsAutoPlayYN isAutoPlay);

	public:
		ButtonLaneJudgment(JudgmentPlayMode judgmentPlayMode, GaugeType gaugeType, FastSlowMode fastSlowMode, Button keyConfigButton, const kson::ByPulse<kson::Interval>& lane, const kson::BeatInfo& beatInfo, const kson::TimingCache& timingCache,bool skinPreview=false);

		void update(const kson::ChartData& chartData, const kson::ByPulse<kson::Interval>& lane, kson::Pulse currentPulse, kson::Pulse currentPulseForDraw, double currentTimeSec, double currentTimeSecForDraw, ButtonLaneStatus& laneStatusRef, JudgmentHandler& judgmentHandlerRef);

        bool applyPreviewChip(kson::Pulse pulse,JudgmentResult result,double exampleMs,double time,double drawTime,ButtonLaneStatus& status,JudgmentHandler& handler);
		void resetSettingsLoop(const kson::ByPulse<kson::Interval>& lane);
        void skipSettings(const kson::ByPulse<kson::Interval>& lane,kson::Pulse pulse,ButtonLaneStatus& status);
        void setFastSlowMode(FastSlowMode mode){m_fastSlowMode=mode;}
        std::size_t chipJudgmentCount() const;

		std::size_t longJudgmentCount() const;

		/// @brief プレイ終了のために判定処理をロックし、残りの未判定ノーツをERROR判定にする
		/// @param judgmentHandlerRef 判定ハンドラへの参照
		void lockForExit(JudgmentHandler& judgmentHandlerRef);
	};
}
