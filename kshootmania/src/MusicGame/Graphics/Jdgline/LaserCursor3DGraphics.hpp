#pragma once
#include "MusicGame/GameStatus.hpp"
#include "MusicGame/ViewStatus.hpp"

namespace MusicGame::Graphics
{
	class LaserCursor3DGraphics
	{
	private:
		const TiledTexture m_texture;

		const Mesh m_mesh;

	public:
		LaserCursor3DGraphics();

		void draw3D(const GameStatus& gameStatus, const ViewStatus& viewStatus, const BasicCamera3D& camera, Float3 displayAnchor = Float3{0,0,0}) const;
	};
}
