#include "AssistTick.hpp"
#include "AudioDefines.hpp"

namespace MusicGame::Audio
{
	namespace
	{
		// Note: ByPulse<Interval>はstd::map<Pulse, Interval>の型エイリアス
		kson::Pulse CurrentNotePulse(const kson::ByPulse<kson::Interval>& lane, kson::Pulse currentPulse)
		{
			const auto nextNote = lane.upper_bound(currentPulse);
			if (nextNote == lane.begin())
			{
				return MusicGame::kPastPulse;
			}

			return std::prev(nextNote)->first;
		}
	}

	AssistTick::AssistTick(AssistTickMode mode)
		: m_mode(mode)
		, m_btTickSound("se/tick.wav", mode == AssistTickMode::kInput ? kson::kNumBTLanesSZ : 1)
		, m_fxTickSound("se/tick2.wav", mode == AssistTickMode::kInput ? kson::kNumFXLanesSZ : 1)
	{
	}

	void AssistTick::update(const kson::ChartData& chartData, const kson::TimingCache& timingCache, double currentTimeSec)
	{
		if (m_mode != AssistTickMode::kAuto)
		{
			return;
		}

        if (ksmaudio::IsASIOActive()) {
            if (!IsScheduledAudioReady()) return;
            const auto& clock = s_scheduling.clock;
            if (m_asioGeneration != clock.generation) {
                const auto restartPulse = kson::SecToPulse(currentTimeSec - 0.000001, chartData.beat, timingCache);
                m_btPlayedPulses.fill(restartPulse); m_fxPlayedPulses.fill(restartPulse);
                m_asioGeneration = clock.generation;
            }
            const auto horizon = kson::SecToPulse(ScheduledAudioHorizon(currentTimeSec), chartData.beat, timingCache);
            const auto schedule = [&](const auto& lanes, auto& last, const auto& sound, bool chipsOnly) {
                for (std::size_t i=0; i<lanes.size(); ++i) {
                    for (auto it=lanes[i].upper_bound(last[i]); it!=lanes[i].end() && it->first<=horizon; ++it) {
                        if ((!chipsOnly || it->second.length==0) && !PlayScheduledSound(sound,
                            kson::PulseToSec(it->first, chartData.beat, timingCache), currentTimeSec)) break;
                        last[i]=it->first;
                    }
                }
            };
            schedule(chartData.note.bt, m_btPlayedPulses, m_btTickSound, false);
            schedule(chartData.note.fx, m_fxPlayedPulses, m_fxTickSound, true);
            return;
        }

		const kson::Pulse currentPulseForAssistTick = kson::SecToPulse(currentTimeSec + SELatencySec(), chartData.beat, timingCache);

		// BT
		for (std::size_t i = 0; i < kson::kNumBTLanesSZ; ++i)
		{
			const kson::Pulse currentNotePulse = CurrentNotePulse(chartData.note.bt[i], currentPulseForAssistTick);
			if (currentNotePulse > m_btPlayedPulses[i])
			{
				// BTの場合はチップノーツとロングノーツの両方で鳴らす
				m_btTickSound.play();
				m_btPlayedPulses[i] = currentNotePulse;
			}
		}

		// FX
		for (std::size_t i = 0; i < kson::kNumFXLanesSZ; ++i)
		{
			const kson::Pulse currentNotePulse = CurrentNotePulse(chartData.note.fx[i], currentPulseForAssistTick);
			if (currentNotePulse > m_fxPlayedPulses[i])
			{
				if (chartData.note.fx[i].contains(currentNotePulse) && chartData.note.fx[i].at(currentNotePulse).length == 0)
				{
					// FXの場合はチップノーツのみ鳴らす
					m_fxTickSound.play();
				}
				m_fxPlayedPulses[i] = currentNotePulse;
			}
		}
	}
    void AssistTick::updateInput(const std::array<bool, kson::kNumBTLanesSZ>& btDown,
        const std::array<bool, kson::kNumFXLanesSZ>& fxDown, bool enabled)
    {
        if (m_mode != AssistTickMode::kInput || !enabled) return;
        // Batch simultaneous buttons into the same mixer block. No future note prediction.
        ksmaudio::OutputBatchLock lock;
        for (bool down : btDown) if (down) m_btTickSound.play();
        for (bool down : fxDown) if (down) m_fxTickSound.play();
    }

}
