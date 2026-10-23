#pragma once
#include "MusicGame/GameDefines.hpp"
#include "kson/ChartData.hpp"
#include "kson/Util/TimingUtils.hpp"
#include "ksmaudio/ksmaudio.hpp"

namespace MusicGame::Audio
{
	class AssistTick
	{
	private:
		AssistTickMode m_mode;
        std::uint64_t m_asioGeneration = 0;
		ksmaudio::Sample m_btTickSound;
		ksmaudio::Sample m_fxTickSound;

		std::array<kson::Pulse, kson::kNumBTLanesSZ> m_btPlayedPulses = { kPastPulse, kPastPulse, kPastPulse, kPastPulse };
		std::array<kson::Pulse, kson::kNumFXLanesSZ> m_fxPlayedPulses = { kPastPulse, kPastPulse };

	public:
        void resetPreviewPlayback(){m_btTickSound.stop();m_fxTickSound.stop();m_btPlayedPulses.fill(kPastPulse);m_fxPlayedPulses.fill(kPastPulse);m_asioGeneration=0;}
		explicit AssistTick(AssistTickMode mode);

		void update(const kson::ChartData& chartData, const kson::TimingCache& timingCache, double currentTimeSec);

        // Down edges, not held states. Input feedback never uses chart-time scheduling.
        void updateInput(const std::array<bool, kson::kNumBTLanesSZ>& btDown,
            const std::array<bool, kson::kNumFXLanesSZ>& fxDown, bool enabled);
	};
}
