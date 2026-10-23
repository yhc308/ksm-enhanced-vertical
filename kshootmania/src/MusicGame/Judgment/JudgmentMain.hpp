#pragma once
#include "kson/ChartData.hpp"
#include "MusicGame/PlayOption.hpp"
#include "ButtonLaneJudgment.hpp"
#include "LaserLaneJudgment.hpp"
#include "JudgmentHandler.hpp"

namespace MusicGame::Judgment
{
    struct PreviewChipEvent {kson::Pulse pulse;int32 lane;double timeSec;size_t phase=0;};
    struct PreviewChipOutcome {JudgmentResult result;double exampleMs;};
    inline constexpr std::array<PreviewChipOutcome,7> kPreviewChipOutcomes{{
        {JudgmentResult::kError,150},{JudgmentResult::kNearFast,75},{JudgmentResult::kCritical,30},
        {JudgmentResult::kCritical,0},{JudgmentResult::kCritical,-35},{JudgmentResult::kNearSlow,-95},{JudgmentResult::kError,-150}}};
    class PreviewChipSequence {
        Array<PreviewChipEvent> m_events;
        size_t m_cursor=0;
    public:
        PreviewChipSequence(const kson::ChartData& chart,const kson::TimingCache& cache,bool enabled){
            if(!enabled)return;
            const auto add=[&](const auto& lane,int32 id){for(const auto& [pulse,note]:lane)if(note.length==0)m_events.push_back({pulse,id,kson::PulseToSec(pulse,chart.beat,cache)});};
            for(int32 i=0;i<4;++i)add(chart.note.bt[i],i);
            for(int32 i=0;i<2;++i)add(chart.note.fx[i],i+4);
            m_events.sort_by([](const auto& a,const auto& b){return a.pulse!=b.pulse?a.pulse<b.pulse:a.lane<b.lane;});
            for(size_t i=0;i<m_events.size();++i)m_events[i].phase=i%7;
        }
        const PreviewChipEvent* nextDue(double now){if(m_cursor==m_events.size() || now<m_events[m_cursor].timeSec)return nullptr;return &m_events[m_cursor++];}
        size_t processed()const{return m_cursor;}
        size_t size()const{return m_events.size();}
    };
	/// @brief ノーツ判定のメイン
	class JudgmentMain
	{
	private:
		const PlayOption m_playOption;
        PreviewChipSequence m_previewChipSequence;
		BTLaneJudgments m_btLaneJudgments;
		FXLaneJudgments m_fxLaneJudgments;
		LaserLaneJudgments m_laserLaneJudgments;

		JudgmentHandler m_judgmentHandler;

	public:
		explicit JudgmentMain(const kson::ChartData& chartData, const kson::TimingCache& timingCache, const PlayOption& playOption, const Optional<CourseContinuation>& courseContinuation, GameMode gameMode);

		void update(const kson::ChartData& chartData, GameStatus& gameStatusRef, ViewStatus& viewStatusRef);

		/// @brief プレイ終了のために判定処理をロックし、残りの未判定ノーツをERROR判定にする
        bool tryActivateAps(ViewStatus& view) { return m_judgmentHandler.tryActivateAps(view); }
		void lockForExit();
        void resetSettingsLoop(const kson::ChartData& chart);
        void skipSettings(const kson::ChartData& chart,GameStatus& game);

        void clearTimingFeedback() { m_judgmentHandler.clearTimingFeedback(); }

		/// @brief PlayResultを取得
		/// @param chartData 譜面データ
		/// @param timingCache タイミングキャッシュ
		/// @param currentTimeSec 現在時刻(秒)
		/// @param isHardFailed HARDゲージ/コースモードで途中落ちしたかどうか
		/// @param isAborted Backボタンで途中終了したかどうか
		/// @return PlayResult
		PlayResult playResult(const kson::ChartData& chartData, const kson::TimingCache& timingCache, double currentTimeSec, IsHardFailedYN isHardFailed, bool isAborted) const;

		/// @brief 全てのノーツが判定済みかどうか
		/// @return 全てのノーツが判定済みならtrue
		bool isFinished() const;

		/// @brief タイミング調整オフセットを取得(秒)
		[[nodiscard]]
		double timingAdjustOffsetSec() const;

		/// @brief タイミング調整オフセットを加算(秒)
		void addTimingAdjustOffset(double offsetSec);
	};
}
