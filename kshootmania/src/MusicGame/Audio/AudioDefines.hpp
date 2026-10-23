#pragma once
#include "ksmaudio/ksmaudio.hpp"
#include <cmath>
#include <algorithm>

namespace MusicGame::Audio
{
	// 効果音のレイテンシ補正用のオフセット
	// (Windowsは70ms、それ以外は35ms早めに再生する)
#ifdef _WIN32
	constexpr double kSELatencySec = 0.070;
#else
	constexpr double kSELatencySec = 0.035;
#endif

    inline double SELatencySec()
    {
        return ksmaudio::IsASIOActive() ? ksmaudio::GetAudioOutputStatus().latencySec : kSELatencySec;
    }

    struct AudioSchedulingContext {
        ksmaudio::AudioClockSnapshot clock;
        double sampleRate = 0, lookaheadSec = 0;
    };
    inline AudioSchedulingContext s_scheduling;
    inline void BeginScheduledAudioFrame(const ksmaudio::AudioClockSnapshot& clock, double frameSec) {
        const auto latency = ksmaudio::GetAudioLatencySnapshot();
        s_scheduling = {clock, latency.output.sampleRate,
            latency.output.latencySec + 2 * latency.blockSec + std::clamp(frameSec, 0.001, 0.25)};
    }
    inline bool IsScheduledAudioReady() { return ksmaudio::IsASIOActive() && s_scheduling.clock.valid; }
    inline double ScheduledAudioHorizon(double referenceTimeSec) {
        return referenceTimeSec + s_scheduling.lookaheadSec;
    }
    inline bool PlayScheduledSound(const ksmaudio::Sample& sound, double eventSec, double referenceSec, double volume = 1) {
        if (!ksmaudio::IsASIOActive()) { sound.play(volume); return true; }
        if (!IsScheduledAudioReady()) return false;
        const auto& clock = s_scheduling.clock;
        const double renderReference = clock.renderPositionSec + (referenceSec - clock.audiblePositionSec);
        const auto frameDelta = static_cast<std::int64_t>(std::llround((eventSec - renderReference) * s_scheduling.sampleRate));
        const auto target = static_cast<std::int64_t>(clock.outputFrame) + frameDelta;
        return sound.schedulePlay(static_cast<std::uint64_t>(std::max<std::int64_t>(target, 0)), volume);
    }

    inline bool PlayInputSound(const ksmaudio::Sample& sound, double currentSec, double audioReferenceSec, double volume) {
        if (!ksmaudio::IsASIOActive()) { sound.play(volume); return true; }
        // This is called only after a real judgment. Positive user delay is explicit;
        // an early/clamped note timestamp must not add an implicit wait to ASIO input feedback.
        const double userDelay = std::max(0.0, currentSec - audioReferenceSec);
        if (userDelay <= 0) { sound.play(volume); return true; }
        const auto clock = ksmaudio::GetAudioLatencySnapshot();
        return sound.schedulePlay(clock.outputFrame + static_cast<std::uint64_t>(std::llround(userDelay * clock.output.sampleRate)), volume);
    }

	// LASER直角音の最大同時再生数
	// (旧バージョンの譜面では別の値を使う)
	constexpr uint32 kLaserSlamSEMaxPolyphony = 1U;
	constexpr uint32 kLaserSlamSEMaxPolyphonyLegacy = 20U;
	constexpr int32 kLaserSlamSEMaxPolyphonyLegacyUntilKSHVersion = 171;

	// FXチップノーツの同一効果音の最大同時再生数
	// (旧バージョンの譜面では別の値を使う)
	constexpr uint32 kKeySoundMaxPolyphony = 1U;
	constexpr uint32 kKeySoundMaxPolyphonyLegacy = 10U;
	constexpr int32 kKeySoundMaxPolyphonyLegacyUntilKSHVersion = 171;

	// LASER直角音のデフォルト音量
	// (譜面データに音量指定が1つもない場合に使用)
	constexpr double kLaserSlamDefaultVolume = 0.5f;
}
