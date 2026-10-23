#pragma once
#include "Highway/Highway3DGraphics.hpp"
#include "Vertical/ProjectiveWall.hpp"
#include "Jdgline/Jdgline3DGraphics.hpp"
#include "Jdgline/Jdgoverlay3DGraphics.hpp"
#include "Jdgline/LaserCursor3DGraphics.hpp"
#include "HUD/GaugePanel.hpp"
#include "HUD/ComboOverlay.hpp"
#include "HUD/AchievementPanel.hpp"
#include "HUD/LaserApproachIndicator.hpp"
#include "HUD/MoviePanel.hpp"
#include "HUD/TimingOverlay.hpp"
#include "HUD/PlayJudgeIndicator.hpp"
#include "MusicGame/GameStatus.hpp"
#include "MusicGame/ViewStatus.hpp"
#include "MusicGame/Scroll/HighwayScroll.hpp"
#include "MusicGame/PlayOption.hpp"
#include "kson/Util/TimingUtils.hpp"

namespace MusicGame::Graphics
{
	/// @brief スコア表示のアニメーション補間
	class ScoreAnimator
	{
	private:
		int32 m_targetScore = 0;
		int32 m_startScore = 0;
		int32 m_displayedScore = 0;
		Stopwatch m_animationTimer;

		static constexpr Duration kAnimationDuration = 0.2s;

	public:
		void update(int32 score);

		[[nodiscard]]
		int32 displayedScore() const;
	};

	class GraphicsMain
	{
	private:
		BasicCamera3D m_camera;

		// TODO: separate into single class
		const Mesh m_bgBillboardMesh;
		const Mesh m_verticalLayerMesh;
        const bool m_vertical;
        const bool m_depthEnabled;
        const bool m_backgroundOnly;
        const bool m_warpDebug;
        const Texture m_uvTexture;
        Vertical::ProjectiveWall m_tunnelWarp;
        MeshData m_referenceGear{4,2};
        Mat4x4 layerTransform(const ViewStatus& view) const;
		std::array<Texture, 2> m_bgTextures;
		const std::array<Mesh,2> m_verticalBGMeshes;
		const Mat4x4 m_bgTransform;
		std::array<Array<Texture>, 2> m_layerFrameTextures;
		const Mat4x4 m_layerTransform;

		Highway3DGraphics m_highway3DGraphics;

		Jdgline3DGraphics m_jdgline3DGraphics;
		Jdgoverlay3DGraphics m_jdgoverlay3DGraphics;
		LaserCursor3DGraphics m_laserCursor3DGraphics;

		std::shared_ptr<noco::Canvas> m_hudCanvas;
		ScoreAnimator m_scoreAnimator;
        int32 m_exHudCaptureStage = -1;

		GaugePanel m_gaugePanel;
		ComboOverlay m_comboOverlay;
		AchievementPanel m_achievementPanel;
		LaserApproachIndicator m_laserApproachIndicator;
		MoviePanel m_moviePanel;
        TimingOverlay m_timingOverlay;
        PlayJudgeIndicator m_playJudgeIndicator;

		const PlayOption m_playOption;
        const bool m_layerTilt;
        const bool m_layerSpin;

		const double m_layerFrameOriginTimeSec;

		const kson::Pulse m_layerFrameOriginPulse;

		void drawBG(const ViewStatus& viewStatus) const;

		void drawLayer(const kson::ChartData& chartData, const GameStatus& gameStatus, const ViewStatus& viewStatus) const;

	public:
		explicit GraphicsMain(const kson::ChartData& chartData, const kson::TimingCache& timingCache, FilePathView parentPath, const PlayOption& playOption);

		void prepareMovie(double globalOffsetSec);

		void update(const GameStatus& gameStatus, const ViewStatus& viewStatus, const kson::TimingCache& timingCache);

		void draw(const kson::ChartData& chartData, const std::array<HashSet<kson::Pulse>, kson::kNumLaserLanesSZ>& laserCurvedPulses, const kson::TimingCache& timingCache, const GameStatus& gameStatus, const ViewStatus& viewStatus, const Scroll::HighwayScrollContext& highwayScrollContext, Duration bgmDuration) const;

		[[nodiscard]]
		bool hasMovie() const;
        Array<double> geometrySignature(const ViewStatus& view) const;
        void writeWarpReport() const {m_tunnelWarp.writeReport(U"vertical-warp-preview.csv");}

		void seekMoviePosSec(SecondsF posSec);
	};
}
