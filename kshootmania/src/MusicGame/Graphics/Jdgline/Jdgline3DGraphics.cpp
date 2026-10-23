#include "Graphics/GameRenderContext.hpp"
#include "Jdgline3DGraphics.hpp"
#include "MusicGame/Graphics/GraphicsDefines.hpp"
#include "MusicGame/Camera/CameraMath.hpp"
#include "Vertical/VerticalModConfig.hpp"

namespace MusicGame::Graphics
{
	namespace
	{
		constexpr StringView kJdglineTextureFilename = U"cline.png";
		constexpr Float3 kPlaneCenter = { 0.0f, 2.6f, -kHighwayPlaneSize.y / 2 - 0.4f };
		constexpr Float2 kPlaneSize = { 344.0f / 8, 26.0f / 8 };
	}

	Jdgline3DGraphics::Jdgline3DGraphics()
		: m_jdglineTexture(TextureAsset(kJdglineTextureFilename))
		, m_mesh(MeshData::Grid(Float3::Zero(), kPlaneSize, 2, 2,
			GameRender::VerticalEnabled() ? Float2{334.0f / 344, 1} : Float2{1, 1},
			GameRender::VerticalEnabled() ? Float2{6.0f / 344, 0} : Float2{0, 0}))
	{
	}

	void Jdgline3DGraphics::draw3D(const ViewStatus& viewStatus, double widthScale) const
	{
		// 3Dの板に判定ラインのテクスチャを描画
		const ScopedRenderStates3D blendState(BlendState::NonPremultiplied);
		const double jdgoverlayScale = viewStatus.camStatus.useLegacyJdgScale
			? Camera::LegacyJdgoverlayScale(viewStatus.camStatus.zoomBottom)
			: Camera::JdgoverlayScale(viewStatus.camStatus.zoomBottom);
		const double jdglineScale = viewStatus.camStatus.useLegacyJdgScale
			? Camera::LegacyJdglineScale(viewStatus.camStatus.zoomBottom)
			: Camera::JdglineScale(viewStatus.camStatus.zoomBottom);
		const double shiftX = viewStatus.camStatus.shiftX;
		const Vec3 shiftXVec = Vec3::Right(shiftX * jdgoverlayScale);
		const double radians = Math::ToRadians(viewStatus.camStatus.rotationDeg + viewStatus.camStatus.rotationDegJdgline) + viewStatus.tiltRadians;
        const auto origin = GameRender::VerticalEnabled()
            ? Mat4x4::Rotate(Float3::Right(), -60_deg) * Mat4x4::Translate(kPlaneCenter) * GearTiltTransformMatrix(radians)
            : JudgmentPlaneTransformMatrix(radians) * Mat4x4::Translate(kPlaneCenter);
        const Transformer3D transform(Mat4x4::Scale(Float3{static_cast<float>(widthScale), 1.0f, 1.0f}) * Mat4x4::Scale(jdglineScale) * Mat4x4::Translate(shiftXVec) * origin);
		m_mesh.draw(m_jdglineTexture);
	}
}
