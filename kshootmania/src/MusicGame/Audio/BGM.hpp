#pragma once
#include "ksmaudio/ksmaudio.hpp"
#include "kson/Audio/AudioEffect.hpp"

namespace MusicGame::Audio
{
	enum class LegacyAudioFPMode
	{
		kNone, // リアルタイムエフェクト
		kF, // 2ファイル指定
		kFP, // 4ファイル指定
	};

	LegacyAudioFPMode DetermineLegacyAudioFPMode(const kson::ChartData& chartData, const FilePath& parentPath);

	struct LegacyAudioFPStream
	{
		std::unique_ptr<ksmaudio::StreamWithEffects> streamF;
		std::unique_ptr<ksmaudio::StreamWithEffects> streamP;
		std::unique_ptr<ksmaudio::StreamWithEffects> streamFP;
		LegacyAudioFPMode mode = LegacyAudioFPMode::kNone;
		ksmaudio::AudioEffect::AudioEffectBus* pAudioEffectBusLaserForF = nullptr;

		void load(const kson::ChartData& chartData, const FilePath& parentPath, double volume, SecondsF offset, double playbackSpeed);

		void update(ksmaudio::StreamWithEffects& mainStream);

		void updateMute(ksmaudio::StreamWithEffects& mainStream, bool fxActive, bool laserActive);

		void play();

		void pause();

		void stop();

		void seekPosSec(SecondsF posSec);

		void updateManually();

		void emplaceAudioEffectLaser(
			const std::string& name,
			const kson::AudioEffectDef& def,
			const std::unordered_map<std::string, std::map<float, std::string>>& paramChanges,
			const std::set<float>& updateTriggerTiming);

		void updateAudioEffectLaser(
			bool bypass,
			const ksmaudio::AudioEffect::Status& status,
			const std::optional<std::size_t>& activeAudioEffectIdx,
			const ksmaudio::AudioEffect::AudioEffectBus& mainAudioEffectBusLaser);
	};

	struct SwitchAudioStream
	{
		std::string name;
		ksmaudio::StreamWithEffects stream;
		ksmaudio::AudioEffect::AudioEffectBus* pAudioEffectBusLaser;

		SwitchAudioStream(const std::string& name, const std::string& filePath, double volume, bool enableCompressor, bool preload, double playbackSpeed)
			: name(name)
			, stream(filePath, volume, enableCompressor, preload, playbackSpeed)
			, pAudioEffectBusLaser(stream.emplaceAudioEffectBusLaser())
		{
		}

		SwitchAudioStream(const SwitchAudioStream&) = delete;
		SwitchAudioStream& operator=(const SwitchAudioStream&) = delete;
		SwitchAudioStream(SwitchAudioStream&&) = delete;
		SwitchAudioStream& operator=(SwitchAudioStream&&) = delete;
	};

	class BGM
	{
	private:
        bool m_qaReportedNotPlaying = false;
		ksmaudio::StreamWithEffects m_stream;
		const double m_playbackSpeed;
		const Duration m_duration;
		SecondsF m_offset;
		SecondsF m_timeSec = 0s;
        ksmaudio::AudioClockSnapshot m_clock;
        Optional<std::uint64_t> m_tailOutputFrame;
        std::uint64_t m_tailGeneration=0;
        double m_tailRenderTimeSec = 0;
		bool m_isStreamStarted = false;
		bool m_isPaused = true;
		ksmaudio::AudioEffect::AudioEffectBus* const m_pAudioEffectBusFX;
		ksmaudio::AudioEffect::AudioEffectBus* const m_pAudioEffectBusLaser;
		Stopwatch m_stopwatch;
		Stopwatch m_manualUpdateStopwatch;

		// SwitchAudio音声のストリーム
		std::vector<std::unique_ptr<SwitchAudioStream>> m_switchAudioStreamsFX;
		std::vector<std::unique_ptr<SwitchAudioStream>> m_switchAudioStreamsLaser;
		Optional<std::size_t> m_activeSwitchAudioIdxFX;
		Optional<std::size_t> m_activeSwitchAudioIdxLaser;

		LegacyAudioFPStream m_legacyAudioFPStream;

		void emplaceAudioEffectImpl(
			bool isFX,
			const std::string& name,
			const kson::AudioEffectDef& def,
			const std::unordered_map<std::string, std::map<float, std::string>>& paramChanges,
			const std::set<float>& updateTriggerTiming);

	public:
		BGM(FilePathView filePath, double volume, SecondsF offset, LegacyAudioFPMode legacyMode, const kson::ChartData& chartData, const FilePath& parentPath, double playbackSpeed = 1.0);

		void update();

		void updateAudioEffectFX(bool bypass, const ksmaudio::AudioEffect::Status& status, const ksmaudio::AudioEffect::ActiveAudioEffectDict& activeAudioEffects);

		// Siv3D非依存のksmaudioに渡す必要があるため、ここではOptionalではなくstd::optionalを使用(これらは暗黙変換されない)
		void updateAudioEffectLaser(bool bypass, const ksmaudio::AudioEffect::Status& status, const std::optional<std::size_t>& activeAudioEffectIdx);

		void play();

		void pause();

		void seekPosSec(SecondsF timeSec);
        void restartAtChartTime(SecondsF timeSec);
        void setSettingsOffset(SecondsF offset);

		SecondsF posSec() const;
        ksmaudio::AudioClockSnapshot clockSnapshot() const;
        SecondsF renderPosSec() const;

		Duration duration() const;

		Duration latency() const;

		void emplaceAudioEffectFX(
			const std::string& name,
			const kson::AudioEffectDef& def,
			const std::unordered_map<std::string, std::map<float, std::string>>& paramChanges,
			const std::set<float>& updateTriggerTiming = {});

		void emplaceAudioEffectLaser(
			const std::string& name,
			const kson::AudioEffectDef& def,
			const std::unordered_map<std::string, std::map<float, std::string>>& paramChanges,
			const std::set<float>& updateTriggerTiming = {});

		const ksmaudio::AudioEffect::AudioEffectBus& audioEffectBusFX() const;

		const ksmaudio::AudioEffect::AudioEffectBus& audioEffectBusLaser() const;

		void setFadeOut(Duration duration);

		void emplaceSwitchAudioStream(
			bool isFX,
			const std::string& effectName,
			const std::string& filename,
			const FilePath& parentPath,
			double volume);

		void updateSwitchAudio(Optional<std::size_t> switchAudioIdxFX, Optional<std::size_t> switchAudioIdxLaser);

		void updateLegacyAudioMute(bool fxActive, bool laserActive);

		Optional<std::size_t> switchAudioIdxByNameFX(const std::string& name) const;
		Optional<std::size_t> switchAudioIdxByNameLaser(const std::string& name) const;

		void emplaceSwitchAudioLaserEffect(
			const std::string& name,
			const kson::AudioEffectDef& def,
			const std::unordered_map<std::string, std::map<float, std::string>>& paramChanges,
			const std::set<float>& updateTriggerTiming = {});

		[[nodiscard]]
		LegacyAudioFPStream& legacyAudioFPStream();

		[[nodiscard]]
		const LegacyAudioFPStream& legacyAudioFPStream() const;

		[[nodiscard]]
		LegacyAudioFPMode legacyAudioFPMode() const;

		[[nodiscard]]
		double playbackSpeed() const;
	};
}
