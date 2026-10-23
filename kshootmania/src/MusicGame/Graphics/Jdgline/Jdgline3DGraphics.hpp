#pragma once
#include "MusicGame/GameStatus.hpp"
#include "MusicGame/ViewStatus.hpp"

namespace MusicGame::Graphics
{
	class Jdgline3DGraphics
	{
	private:
		const Texture m_jdglineTexture;

		const Mesh m_mesh;

	public:
		Jdgline3DGraphics();

		void draw3D(const ViewStatus& viewStatus, double widthScale = 1.0) const;
	};
}
