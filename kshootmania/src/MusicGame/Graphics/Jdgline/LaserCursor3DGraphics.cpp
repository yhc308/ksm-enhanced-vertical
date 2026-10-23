#include "Graphics/GameRenderContext.hpp"
#include "MusicGame/Graphics/ViewportGeometry.hpp"
#include "Jdgoverlay3DGraphics.hpp"
#include "MusicGame/Graphics/GraphicsDefines.hpp"
#include "MusicGame/Camera/CameraMath.hpp"
#include "kson/Common/Common.hpp"
#include "LaserCursor3DGraphics.hpp"
#include "Vertical/VerticalLaneGeometry.hpp"

namespace MusicGame::Graphics
{
	namespace
	{
		constexpr StringView kTextureFilename = U"laser_cur.png";
		constexpr Size kSourceSize = { 64, 64 };

		constexpr Float3 kPlaneCenter = { 0.0f, 3.5f, -kHighwayPlaneSize.y / 2 - 1.8f };
		constexpr Float2 kPlaneSize = { 4.5f, 4.5f };


	}

	LaserCursor3DGraphics::LaserCursor3DGraphics()
		: m_texture(kTextureFilename,
			{
				.row = kson::kNumLaserLanes,
				.sourceScale = SourceScale::kNoScaling,
				.sourceSize = kSourceSize,
			})
		, m_mesh(MeshData::Billboard())
	{
	}

    void LaserCursor3DGraphics::draw3D(const GameStatus& gameStatus,const ViewStatus& viewStatus,const BasicCamera3D& camera,Float3 displayAnchor) const
    {
        const ScopedRenderStates3D state{kEnableAlphaBlend,DepthStencilState{false,false}};
        if(!GameRender::VerticalEnabled()) {
            // Original KSM landscape cursor: its authored center is on the bar,
            // not the center of the highway note texture's judgment row.
            const auto& cam=viewStatus.camStatus;
            const double lineScale=cam.useLegacyJdgScale?Camera::LegacyJdglineScale(cam.zoomBottom):Camera::JdglineScale(cam.zoomBottom);
            const double overlayScale=cam.useLegacyJdgScale?Camera::LegacyJdgoverlayScale(cam.zoomBottom):Camera::JdgoverlayScale(cam.zoomBottom);
            for(int32 i=0;i<kson::kNumLaserLanes;++i) {
                const auto& lane=gameStatus.laserLaneStatus[i];const auto x=lane.cursorXForDraw();if(!x)continue;
                const Vec3 offset{(ViewportGeometry::LaserValue(*x)-.5)*282/8*(lane.cursorWide?2:1)*overlayScale,0,0};
                const Vec3 center=kPlaneCenter+offset+Vec3::Right(cam.shiftX)*overlayScale;
                const double radians=Math::ToRadians(cam.rotationDeg+cam.rotationDegHighway)+viewStatus.tiltRadians;
                const Transformer3D transform(Mat4x4::Scale(lineScale,center)*TiltTransformMatrix(radians));
                Float2 size=kPlaneSize;size.x*=static_cast<float>(ViewportGeometry::LaserScale());
                m_mesh.draw(camera.billboard(center,size),m_texture(i));
            }
            return;
        }
        const auto origin=Vertical::HighwayOrigin(viewStatus);
        const Transformer3D transform{origin};
        for(int32 i=0;i<kson::kNumLaserLanes;++i){
            const auto& lane=gameStatus.laserLaneStatus[i];
            const auto x=lane.cursorXForDraw();if(!x)continue;
            const Vec3 center=Vec3{displayAnchor}+Vec3{ViewportGeometry::LaserWorldX(*x,lane.cursorWide),0,0};
            Float2 size=kPlaneSize;
            if(GameRender::VerticalEnabled()){
                const auto unit=camera.billboard(center,Float2{1,1});
                const auto top=camera.worldToScreenPoint(origin.transformPoint(unit.transformPoint(Float3{0,.5f,0})));
                const auto bottom=camera.worldToScreenPoint(origin.transformPoint(unit.transformPoint(Float3{0,-.5f,0})));
                const double length=(top-bottom).length();
                const float height=static_cast<float>((80.0*GameRender::Height()/1280)/Max(length,1e-5));
                // Height stays readable, while width follows the physical laser plane through zoom.
                const auto neutralSpan=Vertical::NeutralLaneSpan(camera);
                const double farZ=kHighwayPlaneSize.y*Vertical::kJudgeTextureT/2;
                const Vec3 neutral{0,0,farZ+(-kHighwayPlaneSize.y/2-farZ)*neutralSpan.anchorT};
                const double pixelsPerWorld=(camera.worldToScreenPoint(neutral+Vec3{.5,0,0})-
                    camera.worldToScreenPoint(neutral-Vec3{.5,0,0})).length();
                const float width=static_cast<float>((80.0*GameRender::Height()/1280)/Max(pixelsPerWorld,1e-5));
                size={width,height};
            }
            size.x*=static_cast<float>(ViewportGeometry::RailScale());
            m_mesh.draw(camera.billboard(center,size),m_texture(i));
        }
    }
}
