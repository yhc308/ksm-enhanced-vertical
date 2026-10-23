#include "JudgmentMain.hpp"
#include "kson/Util/TimingUtils.hpp"

namespace MusicGame::Judgment
{

	JudgmentMain::JudgmentMain(const kson::ChartData& chartData, const kson::TimingCache& timingCache, const PlayOption& playOption, const Optional<CourseContinuation>& courseContinuation, GameMode gameMode)
		: m_playOption(playOption)
        , m_previewChipSequence(chartData,timingCache,playOption.skinPreview)
		, m_btLaneJudgments{
			ButtonLaneJudgment(playOption.effectiveBtJudgmentPlayMode(), playOption.gaugeType, playOption.fastSlowMode, kButtonBT_A, chartData.note.bt[0], chartData.beat, timingCache, playOption.skinPreview),
			ButtonLaneJudgment(playOption.effectiveBtJudgmentPlayMode(), playOption.gaugeType, playOption.fastSlowMode, kButtonBT_B, chartData.note.bt[1], chartData.beat, timingCache, playOption.skinPreview),
			ButtonLaneJudgment(playOption.effectiveBtJudgmentPlayMode(), playOption.gaugeType, playOption.fastSlowMode, kButtonBT_C, chartData.note.bt[2], chartData.beat, timingCache, playOption.skinPreview),
			ButtonLaneJudgment(playOption.effectiveBtJudgmentPlayMode(), playOption.gaugeType, playOption.fastSlowMode, kButtonBT_D, chartData.note.bt[3], chartData.beat, timingCache, playOption.skinPreview) }
		, m_fxLaneJudgments{
			ButtonLaneJudgment(playOption.effectiveFxJudgmentPlayMode(), playOption.gaugeType, playOption.fastSlowMode, kButtonFX_L, chartData.note.fx[0], chartData.beat, timingCache, playOption.skinPreview),
			ButtonLaneJudgment(playOption.effectiveFxJudgmentPlayMode(), playOption.gaugeType, playOption.fastSlowMode, kButtonFX_R, chartData.note.fx[1], chartData.beat, timingCache, playOption.skinPreview) }
		, m_laserLaneJudgments{
			LaserLaneJudgment(playOption.effectiveLaserJudgmentPlayMode(), 0, chartData.note.laser[0], chartData.beat, timingCache),
			LaserLaneJudgment(playOption.effectiveLaserJudgmentPlayMode(), 1, chartData.note.laser[1], chartData.beat, timingCache) }
		, m_judgmentHandler(chartData, m_btLaneJudgments, m_fxLaneJudgments, m_laserLaneJudgments, playOption, courseContinuation, gameMode)
	{
	}

	void JudgmentMain::update(const kson::ChartData& chartData, GameStatus& gameStatusRef, ViewStatus& viewStatusRef)
	{
        if(m_playOption.judgeSetting){
            for(auto& lane:m_btLaneJudgments)lane.setFastSlowMode(m_playOption.liveFastSlowMode());
            for(auto& lane:m_fxLaneJudgments)lane.setFastSlowMode(m_playOption.liveFastSlowMode());
            if(!m_playOption.judgeSetting->enabled()){
                skipSettings(chartData,gameStatusRef);m_judgmentHandler.applyToViewStatus(viewStatusRef,gameStatusRef.currentTimeSec,gameStatusRef.currentPulse);return;
            }
        }
        if(m_playOption.skinPreview){
            while(const auto* event=m_previewChipSequence.nextDue(gameStatusRef.currentTimeSecForButtonJudgment)){
                const auto outcome=kPreviewChipOutcomes[event->phase];
                auto& lane=event->lane<4?m_btLaneJudgments[event->lane]:m_fxLaneJudgments[event->lane-4];
                auto& status=event->lane<4?gameStatusRef.btLaneStatus[event->lane]:gameStatusRef.fxLaneStatus[event->lane-4];
                if(!lane.applyPreviewChip(event->pulse,outcome.result,outcome.exampleMs,gameStatusRef.currentTimeSecForButtonJudgment,gameStatusRef.currentTimeSec,status,m_judgmentHandler))
                    throw Error{U"Preview chip event was already judged or invalid"};
            }
        }
		// BTレーンの判定
		for (std::size_t i = 0U; i < kson::kNumBTLanesSZ; ++i)
		{
			m_btLaneJudgments[i].update(chartData, chartData.note.bt[i], gameStatusRef.currentPulseForButtonJudgment, gameStatusRef.currentPulse, gameStatusRef.currentTimeSecForButtonJudgment, gameStatusRef.currentTimeSec, gameStatusRef.btLaneStatus[i], m_judgmentHandler);
		}

		// FXレーンの判定
		for (std::size_t i = 0U; i < kson::kNumFXLanesSZ; ++i)
		{
			m_fxLaneJudgments[i].update(chartData, chartData.note.fx[i], gameStatusRef.currentPulseForButtonJudgment, gameStatusRef.currentPulse, gameStatusRef.currentTimeSecForButtonJudgment, gameStatusRef.currentTimeSec, gameStatusRef.fxLaneStatus[i], m_judgmentHandler);
		}

		// LASERレーンの判定
		for (std::size_t i = 0U; i < kson::kNumLaserLanesSZ; ++i)
		{
			m_laserLaneJudgments[i].update(chartData.note.laser[i], gameStatusRef.currentPulseForLaserJudgment, gameStatusRef.currentPulse, gameStatusRef.currentTimeSecForLaserJudgment, gameStatusRef.currentTimeSec, gameStatusRef.laserLaneStatus[i], m_judgmentHandler);
		}

		// 状態をViewStatusに反映
		m_judgmentHandler.applyToViewStatus(viewStatusRef, gameStatusRef.currentTimeSec, gameStatusRef.currentPulse);
        viewStatusRef.previewChipDemoCount=m_previewChipSequence.processed();
	}

    void JudgmentMain::resetSettingsLoop(const kson::ChartData& chart) {
        for(size_t i=0;i<4;++i)m_btLaneJudgments[i].resetSettingsLoop(chart.note.bt[i]);
        for(size_t i=0;i<2;++i){m_fxLaneJudgments[i].resetSettingsLoop(chart.note.fx[i]);m_laserLaneJudgments[i].resetSettingsLoop();}
        m_judgmentHandler.resetSettingsLoop();
    }
    void JudgmentMain::skipSettings(const kson::ChartData& chart,GameStatus& game) {
        for(size_t i=0;i<4;++i)m_btLaneJudgments[i].skipSettings(chart.note.bt[i],game.currentPulseForButtonJudgment,game.btLaneStatus[i]);
        for(size_t i=0;i<2;++i){
            m_fxLaneJudgments[i].skipSettings(chart.note.fx[i],game.currentPulseForButtonJudgment,game.fxLaneStatus[i]);
            m_laserLaneJudgments[i].skipSettings(chart.note.laser[i],game.currentPulseForLaserJudgment,game.currentPulse,game.currentTimeSecForLaserJudgment,game.currentTimeSec,game.laserLaneStatus[i]);
        }
        m_judgmentHandler.clearTimingFeedback();
    }

	void JudgmentMain::lockForExit()
	{
		// ERROR判定通知時にゲージが変動しないよう先にロック
		m_judgmentHandler.lockForExit();

		// 残りの未判定ノーツを全てERROR判定にする
		for (std::size_t i = 0U; i < kson::kNumBTLanesSZ; ++i)
		{
			m_btLaneJudgments[i].lockForExit(m_judgmentHandler);
		}
		for (std::size_t i = 0U; i < kson::kNumFXLanesSZ; ++i)
		{
			m_fxLaneJudgments[i].lockForExit(m_judgmentHandler);
		}
		for (std::size_t i = 0U; i < kson::kNumLaserLanesSZ; ++i)
		{
			m_laserLaneJudgments[i].lockForExit(m_judgmentHandler);
		}
	}

	PlayResult JudgmentMain::playResult(const kson::ChartData& chartData, const kson::TimingCache& timingCache, double currentTimeSec, IsHardFailedYN isHardFailed, bool isAborted) const
	{
		const kson::Pulse lastNoteEndY = kson::LastNoteEndY(chartData.note);
		const double chartEndTimeSec = kson::PulseToSec(lastNoteEndY, chartData.beat, timingCache);
		return m_judgmentHandler.playResult(currentTimeSec, chartEndTimeSec, isHardFailed, isAborted);
	}

	bool JudgmentMain::isFinished() const
	{
		return m_judgmentHandler.isFinished();
	}

	double JudgmentMain::timingAdjustOffsetSec() const
	{
		return m_judgmentHandler.timingAdjustOffsetSec();
	}

	void JudgmentMain::addTimingAdjustOffset(double offsetSec)
	{
		m_judgmentHandler.addTimingAdjustOffset(offsetSec);
	}
}
