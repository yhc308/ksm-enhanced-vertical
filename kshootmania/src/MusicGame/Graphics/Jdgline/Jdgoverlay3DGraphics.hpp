#pragma once
#include "MusicGame/GameStatus.hpp"
#include "MusicGame/ViewStatus.hpp"

namespace MusicGame::Graphics
{
	class Jdgoverlay3DGraphics
	{
	private:
		const RenderTexture m_renderTexture;

		const TiledTexture m_chipCriticalTexture;
        const TiledTexture m_chipPerfectCriticalTexture;
		const TiledTexture m_chipNearTexture;
		const TiledTexture m_chipErrorTexture;

		const TiledTexture m_longAnimTexture;

		const TiledTexture m_laserAnimTexture;

		const TiledTexture m_laserRippleAnimTexture;

		const Mat4x4 m_transform;
		const Mesh m_mesh;

		const TiledTexture& chipAnimTexture(Judgment::ChipAnimType type) const;

		void drawChipAnimCommon(const GameStatus& gameStatus, const ViewStatus& viewStatus, bool isBT) const;

		void drawChipAnimBT(const GameStatus& gameStatus, const ViewStatus& viewStatus) const;

		void drawChipAnimFX(const GameStatus& gameStatus, const ViewStatus& viewStatus) const;

		void drawLongAnimCommon(const GameStatus& gameStatus, const ViewStatus& viewStatus, bool isBT) const;

		void drawLongAnimBT(const GameStatus& gameStatus, const ViewStatus& viewStatus) const;

		void drawLongAnimFX(const GameStatus& gameStatus, const ViewStatus& viewStatus) const;

		void drawLaserAnim(const GameStatus& gameStatus) const;

		void drawLaserRippleAnim(const GameStatus& gameStatus) const;

	public:
		explicit Jdgoverlay3DGraphics(const BasicCamera3D& camera);

		void draw2D(const GameStatus& gameStatus, const ViewStatus& viewStatus) const;

		void draw3D(const ViewStatus& viewStatus,const BasicCamera3D& camera,Float3 displayAnchor) const;
	};
}
