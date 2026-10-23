#pragma once
#include "Graphics/GameRenderContext.hpp"
#include "Vertical/VerticalModConfig.hpp"

namespace MusicGame::Graphics
{
	constexpr int32 kTextureColumnMain = 0;
	constexpr int32 kTextureColumnSub = 1;
	constexpr int32 kNumTextureColumnsMainSub = 2;

	constexpr int32 kLaserXScaleNormal = 1;
	constexpr int32 kLaserXScaleWide = 2;

	constexpr Size kHighwayTextureSize = { 256, 1024 };
	constexpr Float2 kHighwayPlaneSize = { 304.0f * 2 / 13, 936.0f * 13 / 20 };

	constexpr float kJdglineYFromBottom = 14.0f;

    // Padding increases the texture AND plane by the same ratio: 768 px / 3 lane widths.
    constexpr int32 kHighwayRenderPadding = 128;
	constexpr Size kHighwayTextureSizeWide = { kHighwayTextureSize.x * kLaserXScaleWide + 2 * kHighwayRenderPadding, kHighwayTextureSize.y };
	constexpr Float2 kHighwayPlaneSizeWide = { kHighwayPlaneSize.x * (kLaserXScaleWide + 1), kHighwayPlaneSize.y };

	constexpr int32 kHighwayTextureWideCenterX = kHighwayTextureSizeWide.x / 2;

	constexpr int32 kHighwayTextureOffsetX = (kHighwayTextureSizeWide.x - kHighwayTextureSize.x) / 2;

	constexpr Vec2 kLanePositionOffset = { kHighwayTextureOffsetX + 44.0, 0.0 };
	constexpr Vec2 kBTLanePositionDiff = { 42.0, 0.0 };
	constexpr Vec2 kFXLanePositionDiff = { 84.0, 0.0 };

	constexpr Vec2 kLaserPositionOffsetNormal = { kHighwayTextureOffsetX, 0 };
	constexpr Vec2 kLaserPositionOffsetWide = { kHighwayRenderPadding, 0 };

	// アルファブレンド有効で描画
	// (描画先テクスチャはあらかじめ不透明にしておく必要があるので注意。不透明でない場合、見た目が暗くなる)
	constexpr BlendState kEnableAlphaBlend = BlendState(
		true,
		Blend::SrcAlpha,
		Blend::InvSrcAlpha,
		BlendOp::Add,
		Blend::One);

	// 反転・乗算描画
	// (HSP版ではhgimg3の「色減算(substract2)」を使用していたが、このモードはなぜか減算ではなく反転・乗算しているようだったのでこちらを使用)
	constexpr BlendState kInvMultiply = BlendState(
		true,
		Blend::Zero,
		Blend::InvSrcColor,
		BlendOp::Add,
		Blend::Zero,
		Blend::One,
		BlendOp::Add);

	constexpr ColorF kTransparent = ColorF{ 0.0, 0.0 };

	constexpr ColorF kSongInfoFontColor = Color{ 0, 32, 8 };

	constexpr double kOnePixelTextureSourceSize = 0.5; // 幅1pxのテクスチャを幅1pxとして使用すると縁に余白ができるので、描画元サイズは正確に1pxにせず小さめにする

	constexpr double kOnePixelTextureSourceOffset = (1.0 - kOnePixelTextureSourceSize) / 2; // kOnePixelTextureSourceSize使用時の座標オフセット

	constexpr Vec3 kHighwayTiltCenter = { 0.0, 42.0, 0.0 };

	inline Mat4x4 TiltTransformMatrix(double radians, const Vec3& center = kHighwayTiltCenter)
	{
		return Mat4x4::Rotate(Float3::Backward(), radians, center);
	}

    // Display-only portrait origin, fitted to the supplied SDVX silhouettes.
    // Conjugating the rotation changes its axis, never the authored angle/time.
    // USC Camera::GetOriginTransform uses a portrait-specific base orientation.
    constexpr double kVerticalGearAxisPitch = 2.7_deg;
    constexpr Vec3 kVerticalGearTiltCenter = {0.0, 24.4, 0.0};
    inline Mat4x4 GearTiltTransformMatrix(double radians)
    {
        if (!GameRender::VerticalEnabled()) return TiltTransformMatrix(radians);
        const Float3 axis{0.0f, static_cast<float>(Sin(kVerticalGearAxisPitch)),
            static_cast<float>(-Cos(kVerticalGearAxisPitch))};
        return Mat4x4::Rotate(axis, radians, kVerticalGearTiltCenter);
    }

	inline Mat4x4 JudgmentPlaneTransformMatrix(double tiltRadians)
	{
		return Mat4x4::Rotate(Float3::Right(), -60_deg) * TiltTransformMatrix(tiltRadians, kHighwayTiltCenter);
	}

	constexpr std::size_t kChipAnimMaxPlaying = 5U;

	constexpr std::size_t kLaserRippleAnimMaxPlaying = 5U;
}
