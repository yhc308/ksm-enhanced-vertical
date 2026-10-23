#pragma once
#include "MusicGame/GameDefines.hpp"
#include "MusicGame/GameStatus.hpp"
#include "kson/ChartData.hpp"
#include "kson/Util/TimingUtils.hpp"
#include "ksmaudio/ksmaudio.hpp"

namespace MusicGame::Audio
{
	class LaserSlamSE
	{
	private:
        std::uint64_t m_asioGeneration = 0, m_manualGeneration = 0;
        double m_asioRestartTime = -1e100;
		ksmaudio::Sample m_defaultSlamSound;

		std::unordered_map<std::string, ksmaudio::Sample> m_slamSounds;

		std::array<double, kson::kNumLaserLanesSZ> m_lastPlayedTimeSecs = { kPastTimeSec, kPastTimeSec };

		bool m_isAutoPlaySE;

		double m_folderVolumeScale;

		std::array<kson::Pulse, kson::kNumLaserLanesSZ> m_autoPlaySELastPulses = { 0, 0 };

		std::map<kson::Pulse, double> m_pulseToSec;

		void updateByNoteTime(const kson::ChartData& chartData, const GameStatus& gameStatus);

		void updateByJudgment(const kson::ChartData& chartData, const GameStatus& gameStatus);

	public:
        void resetPreviewPlayback() { m_defaultSlamSound.stop();for(const auto& [_,sound]:m_slamSounds)sound.stop(); m_asioGeneration=0;m_manualGeneration=0;m_asioRestartTime=-1e100; m_lastPlayedTimeSecs = {kPastTimeSec, kPastTimeSec}; m_autoPlaySELastPulses = {0, 0}; }
		explicit LaserSlamSE(const kson::ChartData& chartData, const kson::TimingCache& timingCache, FilePathView parentPath, bool isAutoPlaySE, double folderVolumeScale);

		void update(const kson::ChartData& chartData, const GameStatus& gameStatus);
	};
}
