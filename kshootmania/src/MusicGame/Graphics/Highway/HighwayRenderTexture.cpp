#include "Graphics/GameRenderContext.hpp"
#include "MusicGame/Graphics/ViewportGeometry.hpp"
#include "HighwayRenderTexture.hpp"
#include "MusicGame/Graphics/GraphicsDefines.hpp"
#include "Vertical/VerticalLaneGeometry.hpp"
#include "MusicGame/Camera/CameraMath.hpp"

namespace MusicGame::Graphics
{
	namespace
	{
		constexpr StringView kBaseTextureFilename = U"base.gif";

		RenderTexture CreateBaseTexture(int32 column, const Color& clearColor)
		{
			const Texture textureAsset = TextureAsset(kBaseTextureFilename);

			RenderTexture texture(kHighwayTextureSizeWide);
			texture.clear(clearColor);

			// 中央へ描画
			{
				const ScopedRenderTarget2D renderTarget(texture);
				const ScopedRenderStates2D samplerState(SamplerState::ClampNearest);
				if (!GameRender::VerticalEnabled()) {
                    textureAsset(kHighwayTextureSize.x * column,0,kHighwayTextureSize).draw(kHighwayTextureOffsetX,0);
                } else {
                    const std::array<double,4> from{0,44,212,256};
                    const double rail=256*Vertical::kLaserRailWidthRatio;
                    const std::array<double,4> to{0,rail,256-rail,256};
                    for(int i=0;i<3;++i)
                        textureAsset(column*256+from[i],0,from[i+1]-from[i],1024)
                            .resized(to[i+1]-to[i],1024).draw(kHighwayTextureOffsetX+to[i],0);
                }
			}

			return texture;
		}
	}

	HighwayRenderTexture::HighwayRenderTexture()
		: m_additiveTexture(kHighwayTextureSizeWide)
		, m_invMultiplyTexture(kHighwayTextureSizeWide)
		, m_additiveBaseTexture(CreateBaseTexture(kTextureColumnMain, Palette::Black))
		, m_invMultiplyBaseTexture(CreateBaseTexture(kTextureColumnSub, Palette::Black))
	{
	}

	void HighwayRenderTexture::draw3D(const DynamicMesh& mesh) const
	{
		{
			const ScopedRenderStates3D renderState(kInvMultiply);
			mesh.draw(m_invMultiplyTexture);
		}

		{
			const ScopedRenderStates3D renderState(BlendState::Additive);
			mesh.draw(m_additiveTexture);
		}
	}

	const RenderTexture& HighwayRenderTexture::additiveTexture() const
	{
		return m_additiveTexture;
	}

	const RenderTexture& HighwayRenderTexture::invMultiplyTexture() const
	{
		return m_invMultiplyTexture;
	}

    void HighwayRenderTexture::drawBaseTexture(double centerSplit) const
    {
        if (MathUtils::AlmostEquals(centerSplit, 0.0) && MathUtils::AlmostEquals(ViewportGeometry::RailScale(), 1.0))
        {
            Shader::Copy(m_additiveBaseTexture, m_additiveTexture);
            Shader::Copy(m_invMultiplyBaseTexture, m_invMultiplyTexture);
            return;
        }
        const double shift = Camera::CenterSplitShiftX(centerSplit);
        const double offset = kHighwayTextureOffsetX;
        const ScopedRenderStates2D sampler{SamplerState::ClampNearest};
        const auto drawParts = [&](const RenderTexture& source, const RenderTexture& target)
        {
            const ScopedRenderTarget2D renderTarget{target.clear(Palette::Black)};
            const auto part = [&](double from, double to, double split)
            {
                const double left=ViewportGeometry::RailX(from), right=ViewportGeometry::RailX(to);
                source(offset+from,0,to-from,kHighwayTextureSize.y)
                    .resized(right-left,kHighwayTextureSize.y).draw(offset+left+split,0);
            };
            // Fixed center lane, independently stretched rails, including both blend columns.
            const double left=ViewportGeometry::CoreLeft(),right=256-left;
            part(0,left,-shift);
            part(left,128,-shift);
            part(128,right,shift);
            part(right,256,shift);
        };
        drawParts(m_additiveBaseTexture,m_additiveTexture);
        drawParts(m_invMultiplyBaseTexture,m_invMultiplyTexture);
    }
}
