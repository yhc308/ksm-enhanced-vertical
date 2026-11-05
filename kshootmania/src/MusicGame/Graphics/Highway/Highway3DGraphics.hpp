#pragma once
#include "MusicGame/GameStatus.hpp"
#include "MusicGame/ViewStatus.hpp"
#include "MusicGame/PlayOption.hpp"
#include "MusicGame/Scroll/HighwayScroll.hpp"
#include "HighwayRenderTexture.hpp"
#include "KeyBeamGraphics.hpp"
#include "Note/ButtonNoteGraphics.hpp"
#include "Note/LaserNoteGraphics.hpp"
#include "Vertical/VerticalLaneGeometry.hpp"

namespace MusicGame::Graphics
{
	class Highway3DGraphics
	{
	private:
		const Texture m_shineEffectTexture;
		const Texture m_barLineTexture;

		const HighwayRenderTexture m_renderTexture;

		const ButtonNoteGraphics m_buttonNoteGraphics;
		const LaserNoteGraphics m_laserNoteGraphics;

		const KeyBeamGraphics m_keyBeamGraphics;

		MeshData m_meshData;
		DynamicMesh m_mesh;
		DynamicMesh m_verticalMesh;
		MeshData m_verticalMeshData{ MeshData::Grid(kHighwayPlaneSizeWide, 1, 2) };
		bool m_trianglesFlipped = false;
		bool m_verticalTrianglesFlipped = false;
		Vertical::LaneSpan m_displayLaneSpan{};
		Float3 m_displayNoteAnchor{ 0, 0, 0 };

	public:
		Highway3DGraphics();

		void update(const ViewStatus& viewStatus, const BasicCamera3D* displayCamera = nullptr);
		const Vertical::LaneSpan& displayLaneSpan() const { return m_displayLaneSpan; }
		Float3 displayNoteAnchor() const
		{
			return m_displayNoteAnchor;
		}
		const MeshData& displayMesh() const
		{
			return GameRender::VerticalEnabled() ? m_verticalMeshData : m_meshData;
		}
		const MeshData& sourceMesh() const
		{
			return m_meshData;
		}
		Float3 upperMidpoint() const
		{
			return (m_meshData.vertices[0].pos + m_meshData.vertices[1].pos) * .5f;
		}
		static Float3 neutralUpperMidpoint(bool legacy)
		{
			return { 0, 0, kHighwayPlaneSize.y * (legacy ? 1.0f : static_cast<float>(1 - 14.0 / 1024)) / 2 };
		}

		void draw2D(const kson::ChartData& chartData, const std::array<HashSet<kson::Pulse>, kson::kNumLaserLanesSZ>& laserCurvedPulses, const PlayOption& playOption, const kson::TimingCache& timingCache, const GameStatus& gameStatus, const ViewStatus& viewStatus, const Scroll::HighwayScrollContext& highwayScrollContext) const;

		void draw3D(const ViewStatus& viewStatus) const;
	};
}
