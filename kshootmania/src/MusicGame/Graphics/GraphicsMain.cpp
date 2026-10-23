#include "Graphics/GameRenderContext.hpp"
#include "GraphicsMain.hpp"
#include "Vertical/VerticalLaneGeometry.hpp"
#include "GraphicsMain.hpp"
#include "GraphicsDefines.hpp"
#include "Diagnostics/TimingHudCapture.hpp"
#include "Diagnostics/PlayJudgeIndicatorFixture.hpp"
#include "Graphics/ImageUtils.hpp"
#include "MusicGame/GameDefines.hpp"
#include "MusicGame/HispeedUtils.hpp"
#include "kson/Util/GraphUtils.hpp"
#include "Vertical/VerticalCameraProfile.hpp"
#include "Vertical/VerticalHUDLayout.hpp"
#include "MusicGame/Camera/CameraMath.hpp"

namespace MusicGame::Graphics
{
	void ScoreAnimator::update(int32 score)
	{
		if (m_targetScore != score)
		{
			m_startScore = m_displayedScore;
			m_targetScore = score;
			m_animationTimer.restart();
		}

		if (m_animationTimer.isRunning() && m_animationTimer < kAnimationDuration)
		{
			const double progress = m_animationTimer.sF() / kAnimationDuration.count();
			m_displayedScore = static_cast<int32>(m_startScore + static_cast<double>(m_targetScore - m_startScore) * progress);
		}
		else
		{
			m_displayedScore = m_targetScore;
		}
	}

	int32 ScoreAnimator::displayedScore() const
	{
		return m_displayedScore;
	}

	namespace
	{
		constexpr double kCameraVerticalFOV = 45_deg;
		constexpr Vec3 kCameraPosition = { 0.0, 45.0, -366.0 };

		constexpr double kSin15Deg = 0.2588190451;
		constexpr double kCos15Deg = 0.9659258263;
		constexpr Vec3 kCameraLookAt = kCameraPosition + Vec3{ 0.0, -100.0 * kSin15Deg, 100.0 * kCos15Deg };

		constexpr Float3 kBGBillboardPosition = Float3{ 0, -52.5f, 0 };
		constexpr Float2 kBGBillboardSize = Float2{ 900.0f, 800.0f } *0.65f;

		constexpr Size kLayerFrameTextureSize = { 600, 480 };
		constexpr Float3 kLayerBillboardPosition = Float3{ 0, -41.0f, 0 };
		constexpr Float2 kLayerBillboardSize = Float2{ 880.0f, 704.0f } * 0.65f;

		Mat4x4 BackgroundBillboard(const BasicCamera3D& camera, Float3 center, Float2 size)
		{
			const auto original = camera.billboard(center, size);
			if (!GameRender::VerticalEnabled()) return original;
			const auto projectedCenter = camera.worldToScreenPoint(center);
			const double halfWidth = Abs(camera.worldToScreenPoint(original.transformPoint(Float3{0.5f, 0, 0})).x - projectedCenter.x);
			const double halfHeight = Abs(camera.worldToScreenPoint(original.transformPoint(Float3{0, 0.5f, 0})).y - projectedCenter.y);
			const double cover = Max(Max(Abs(projectedCenter.x), Abs(GameRender::Width() - projectedCenter.x)) / Max(halfWidth, 1.0),
				Max(Abs(projectedCenter.y), Abs(GameRender::Height() - projectedCenter.y)) / Max(halfHeight, 1.0));
			return camera.billboard(center, size * static_cast<float>(Max(1.0, cover * 1.2)));
		}

		constexpr FilePathView kDefaultLayerFilePath = U"imgs/bg/arrow.gif";

		// Display geometry only. Chart selection, frame timing and rotation remain upstream.
		Mat4x4 VerticalBackdrop(const BasicCamera3D& camera, Float3 center)
		{
			const auto basis = camera.billboard(center, Float2{1, 1});
			const auto p = camera.worldToScreenPoint(center);
			const auto right = basis.transformPoint(Float3{1,0,0}) - center;
			const auto up = basis.transformPoint(Float3{0,1,0}) - center;
			const double dx = camera.worldToScreenPoint(center + right).x - p.x;
			const double dy = camera.worldToScreenPoint(center + up).y - p.y;
			const auto shifted = center + right * static_cast<float>((GameRender::Width()*0.5-p.x)/dx)
				+ up * static_cast<float>((GameRender::Height()*0.5-p.y)/dy);
			return camera.billboard(shifted, Float2{static_cast<float>(GameRender::Width()/Abs(dx)), static_cast<float>(GameRender::Height()/Abs(dy))});
		}

        Float3 VerticalBackdropPivot(const BasicCamera3D& camera, Float3 depthCenter)
        {
            // Keep the texture intact. Its wall opening rotates about a point
            // on the relocated billboard, not the old off-screen world center.
            const auto basis = camera.billboard(depthCenter, Float2{1,1});
            const auto p = camera.worldToScreenPoint(depthCenter);
            const auto target = camera.worldToScreenPoint(Vertical::NeutralFarAnchor());
            const auto right = basis.transformPoint(Float3{1,0,0}) - depthCenter;
            const auto up = basis.transformPoint(Float3{0,1,0}) - depthCenter;
            const double dx = camera.worldToScreenPoint(depthCenter + right).x - p.x;
            const double dy = camera.worldToScreenPoint(depthCenter + up).y - p.y;
            return depthCenter + right * static_cast<float>((target.x-p.x)/dx)
                + up * static_cast<float>((target.y-p.y)/dy);
        }

		MeshData VerticalBackdropMesh(bool layer, Size textureSize = Size{720,640}, const BasicCamera3D* camera=nullptr)
		{
			auto result = MeshData::Billboard();
			if (!GameRender::VerticalEnabled()) return result;
			const auto definition = Vertical::HUDDefinition()[U"backgroundDisplay"];
			RectF rect;
			if (layer) {
				const auto data=definition[U"layerRect"];
				rect={data[0].get<double>(),data[1].get<double>(),data[2].get<double>(),data[3].get<double>()};
                // Only two logical pixels of neutral overscan, preserving image aspect.
                if(camera) {
                    const double scale=(rect.w+4)/rect.w;
                    const auto anchor=camera->worldToScreenPoint(Vertical::NeutralFarAnchor());
                    const Vec2 center{anchor.x*720/camera->getSceneSize().x,anchor.y*1280/camera->getSceneSize().y-58.5};
                    rect={center.x+(rect.x-center.x)*scale,center.y+(rect.y-center.y)*scale,rect.w*scale,rect.h*scale};
                }
			} else {
				const auto center=definition[U"baseCenter"];
				const double cx=center[0].get<double>(), cy=center[1].get<double>();
				double width=definition[U"baseWidth"].get<double>();
				double height=width*textureSize.y/Max(textureSize.x,1);
				const double cover=Max(1.0,2*Max(cy,1280-cy)/Max(height,1.0));
				width*=cover; height*=cover;
				rect={cx-width/2,cy-height/2,width,height};
			}
			// Original full UVs and topology: only one uniform image rectangle.
			for(auto& vertex:result.vertices) {
				vertex.pos.x=static_cast<float>((rect.x+rect.w*vertex.tex.x)/720-.5);
				vertex.pos.y=static_cast<float>(.5-(rect.y+rect.h*vertex.tex.y)/1280);
			}
			return result;
		}
		std::array<Texture, 2> LoadBGTextures(const kson::ChartData& chartData, FilePathView parentPath)
		{
			std::array<Texture, 2> textures;

			for (size_t i = 0; i < chartData.bg.legacy.bg.size(); ++i)
			{
				const String filename = Unicode::FromUTF8(chartData.bg.legacy.bg[i].filename);

				if (filename.isEmpty())
				{
					if (i == 0)
					{
						// bg[0]が空の場合はデフォルトの背景
						textures[i] = SkinPreset::LoadTexture(U"bg/desert{}.jpg"_fmt(i));
					}
					else
					{
						// bg[1]が空の場合はbg[0]と同じテクスチャを使用
						textures[i] = textures[0];
					}
					continue;
				}

				if (FileSystem::Extension(filename).empty())
				{
					// 標準の背景
					const String filePath = SkinPreset::Resolve(U"bg/{}{}.jpg"_fmt(filename, i));
					if (FileSystem::Exists(filePath))
					{
						textures[i] = Texture(filePath);
					}
					else
					{
						// 存在しない場合はデフォルトの背景
						textures[i] = SkinPreset::LoadTexture(U"bg/desert{}.jpg"_fmt(i));
					}
				}
				else
				{
					// 標準の背景でなければ、譜面ファイルと同じディレクトリのファイル名として参照
					const String filePath = FileSystem::PathAppend(parentPath, filename);
					if (FileSystem::Exists(filePath))
					{
						textures[i] = Texture(filePath);
					}
					else
					{
						// 存在しない場合は、デフォルトの背景
						textures[i] = SkinPreset::LoadTexture(U"bg/desert{}.jpg"_fmt(i));
					}
				}
			}

			return textures;
		}

		FilePath LayerFilePath(const kson::ChartData& chartData, FilePathView parentPath)
		{
			const String filename = Unicode::FromUTF8(chartData.bg.legacy.layer.filename);
			if (FileSystem::Extension(filename).empty())
			{
				// 標準のレイヤーアニメーション
				return SkinPreset::Resolve(U"bg/{}.gif"_fmt(filename));
			}

			// 標準のレイヤーアニメーションでなければ、譜面ファイルと同じディレクトリのファイル名として参照
			const String filePath = FileSystem::PathAppend(parentPath, filename);
			if (!FileSystem::Exists(filePath))
			{
				// 存在しない場合は、デフォルトのレイヤーアニメーション(arrow)を返す
				return SkinPreset::Resolve(kDefaultLayerFilePath);
			}
			return filePath;
		}

		FilePath MovieFilePath(const kson::ChartData& chartData, FilePathView parentPath)
		{
			const String filename = Unicode::FromUTF8(chartData.bg.legacy.movie.filename);
			if (filename.isEmpty())
			{
				return U"";
			}

			// 動画は譜面ファイルと同じディレクトリから読み込む
			return FileSystem::PathAppend(parentPath, filename);
		}

		std::array<Array<Texture>, 2> LayerFrameTexturesFromTileGrid(const Grid<Image>& tiles)
		{
			std::array<Array<Texture>, 2> frameTextures;
			const size_t rowCount = Min<size_t>(tiles.height(), frameTextures.size());
			for (size_t row = 0; row < rowCount; ++row)
			{
				frameTextures[row].reserve(tiles.width());
				for (size_t col = 0; col < tiles.width(); ++col)
				{
					frameTextures[row].emplace_back(tiles[row][col]);
				}
			}
			return frameTextures;
		}

		std::array<Array<Texture>, 2> LoadLayerFrameTextures(FilePathView layerFilePath)
		{
			// 通常サイズの画像はImageで直接デコード
			if (const Image image{ layerFilePath })
			{
				return LayerFrameTexturesFromTileGrid(ImageUtils::SplitIntoTiles(image, kLayerFrameTextureSize));
			}

			// Image単体のサイズ制限を超える画像はstb_image経由でタイル単位に分割して読み込む
			const Grid<Image> tiles = ImageUtils::LoadAsTileGrid(layerFilePath, kLayerFrameTextureSize);
			if (!tiles.isEmpty())
			{
				return LayerFrameTexturesFromTileGrid(tiles);
			}

			Logger << U"[ksm warning] Failed to load layer image: " << layerFilePath;

			// 読み込みに失敗した場合はデフォルトのレイヤーアニメーションへフォールバック
			if (layerFilePath != kDefaultLayerFilePath)
			{
				Logger << U"[ksm info] Falling back to default layer: " << kDefaultLayerFilePath;
				return LoadLayerFrameTextures(kDefaultLayerFilePath);
			}
			return {};
		}

		/// @brief BPMを文字列にフォーマット(小数部分がある場合のみ小数を表示)
		String FormatBPM(double bpm)
		{
			const int32 bpmInt = static_cast<int32>(bpm * 1000);
			const int32 integerPart = bpmInt / 1000;
			const int32 decimalPart = bpmInt % 1000;
			if (decimalPart == 0)
			{
				return Format(integerPart);
			}

			// 末尾のゼロを除去
			String result = U"{}.{:03d}"_fmt(integerPart, decimalPart);
			while (result.back() == U'0')
			{
				result.pop_back();
			}
			return result;
		}

		std::shared_ptr<noco::Canvas> LoadHUDCanvas()
		{
			const FilePath uiFilePath = FsUtils::GetResourcePath(GameRender::VerticalEnabled()
				? U"mods/vertical/ui/play_hud.noco" : U"mods/enhanced/ui/play_hud.noco");
			const auto canvas = noco::Canvas::LoadFromFile(uiFilePath);
			if (!canvas)
			{
				throw Error{ U"Failed to load '{}'"_fmt(uiFilePath) };
			}
			return canvas;
		}

		constexpr double kPreStartOffsetSec = 3.4;
		constexpr double kPreStartOffsetWithMovieSec = 4.4;
	}

	void GraphicsMain::drawBG(const ViewStatus& viewStatus) const
	{
		const ScopedRenderStates3D samplerState(GameRender::VerticalEnabled() ? SamplerState::ClampLinear : SamplerState::ClampNearest);

		// ゲージパーセンテージに応じてBGテクスチャのインデックスを決定
		const int32 percentThreshold = (viewStatus.effectiveGaugeType == GaugeType::kHardGauge || m_playOption.gameMode == GameMode::kCourseMode) ? kGaugePercentageThresholdHardWarning : kGaugePercentageThreshold;
		const int32 bgTextureIndex = viewStatus.gaugePercentageInt >= percentThreshold ? 1 : 0;

		double bgTiltRadians = viewStatus.tiltRadiansForBgLayer / 3;
		(GameRender::VerticalEnabled() ? m_verticalBGMeshes[bgTextureIndex] : m_bgBillboardMesh).draw(m_bgTransform * TiltTransformMatrix(bgTiltRadians, GameRender::VerticalEnabled() ? VerticalBackdropPivot(m_camera,kBGBillboardPosition) : kBGBillboardPosition), m_bgTextures[bgTextureIndex],ColorF{SkinPreset::Current().background1Brightness/100.0});

	}

    Mat4x4 GraphicsMain::layerTransform(const ViewStatus& view) const
    {
        const double angle=(m_layerTilt ? view.tiltRadiansForBgLayer*.8 : 0)+
            (m_layerSpin ? Math::ToRadians(view.camStatus.rotationDegLayer) : 0);
        return m_layerTransform*TiltTransformMatrix(angle,m_vertical ? VerticalBackdropPivot(m_camera,kLayerBillboardPosition) : kLayerBillboardPosition);
    }

	void GraphicsMain::drawLayer(const kson::ChartData& chartData, const GameStatus& gameStatus, const ViewStatus& viewStatus) const
	{
		const ScopedRenderStates3D samplerState(GameRender::VerticalEnabled() ? SamplerState::ClampLinear : SamplerState::ClampNearest);
		const ScopedRenderStates3D renderState(BlendState::Additive);

		// ゲージパーセンテージに応じてレイヤーテクスチャのインデックスを決定
		const int32 percentThreshold = (viewStatus.effectiveGaugeType == GaugeType::kHardGauge || m_playOption.gameMode == GameMode::kCourseMode) ? kGaugePercentageThresholdHardWarning : kGaugePercentageThreshold;
		const int32 layerTextureIndex = viewStatus.gaugePercentageInt >= percentThreshold ? 1 : 0;

		if (!m_layerFrameTextures[layerTextureIndex].empty())
		{
			// レイヤーアニメーション速度の計算
			int32 layerFrame = 0;
			const int32 duration = chartData.bg.legacy.layer.duration;
			if (duration == 0)
			{
				// duration == 0の場合、テンポ同期(1フレーム = 0.035小節)
				const kson::Pulse relativePulse = gameStatus.currentPulse - m_layerFrameOriginPulse;
				layerFrame = MathUtils::WrappedMod(static_cast<int32>(relativePulse * 1000 / 35 / kson::kResolution4), static_cast<int32>(m_layerFrameTextures[layerTextureIndex].size()));
			}
			else
			{
				// duration != 0の場合、固定速度(ミリ秒単位)
				const double absDuration = std::abs(duration);
				const double frameTimeMs = (gameStatus.currentTimeSec - m_layerFrameOriginTimeSec) * 1000.0;
				const int32 frameCount = static_cast<int32>(m_layerFrameTextures[layerTextureIndex].size());

				if (duration > 0)
				{
					// 正再生
					layerFrame = MathUtils::WrappedMod(static_cast<int32>(frameTimeMs * frameCount / absDuration), frameCount);
				}
				else
				{
					// 逆再生
					layerFrame = MathUtils::WrappedMod(frameCount - static_cast<int32>(frameTimeMs * frameCount / absDuration) - 1, frameCount);
				}
			}

			const auto transform=layerTransform(viewStatus);
            const auto& texture=m_uvTexture ? m_uvTexture : m_layerFrameTextures[layerTextureIndex].at(layerFrame);
            if(!m_vertical || !m_depthEnabled || !m_tunnelWarp.draw(transform,texture,ColorF{SkinPreset::Current().background2Opacity/100.0}))
                (m_vertical ? m_verticalLayerMesh : m_bgBillboardMesh).draw(transform,texture,ColorF{SkinPreset::Current().background2Opacity/100.0});
		}
	}

	GraphicsMain::GraphicsMain(const kson::ChartData& chartData, const kson::TimingCache& timingCache, FilePathView parentPath, const PlayOption& playOption)
		: m_camera(GameRender::VerticalEnabled()
			? Vertical::MakeCamera(GameRender::Size(), Vertical::ModConfig::Load().layout)
			: BasicCamera3D{GameRender::Size(), kCameraVerticalFOV, kCameraPosition, kCameraLookAt})
		, m_bgBillboardMesh(MeshData::Billboard())
		, m_verticalLayerMesh(VerticalBackdropMesh(true,Size{720,640},&m_camera))
        , m_vertical(GameRender::VerticalEnabled())
        , m_depthEnabled(Vertical::EffectiveDepthConfig().backgroundDepthEnabled && !System::GetCommandLineArgs().contains(U"--vertical-preview-warp-off"))
        , m_backgroundOnly(System::GetCommandLineArgs().contains(U"--vertical-preview-background-only"))
        , m_warpDebug(System::GetCommandLineArgs().contains(U"--vertical-preview-warp-mesh"))
        , m_uvTexture([]() {
            if(!System::GetCommandLineArgs().contains(U"--vertical-preview-warp-uv")) return Texture{};
            Image image{600,480,Color{12,22,32}};
            for(int y=0;y<480;++y) for(int x=0;x<600;++x) {
                image[y][x]=(x%25==0||y%30==0) ? Color{40,220,240} :
                    ((x/25+y/30)%2 ? Color{20,45,55} : Color{50,73,78});
            }
            return Texture{image};
        }())
        , m_tunnelWarp(VerticalBackdropMesh(true,Size{720,640},&m_camera),m_vertical && m_depthEnabled)
		, m_bgTextures(LoadBGTextures(chartData, parentPath))
		, m_verticalBGMeshes{Mesh{VerticalBackdropMesh(false,m_bgTextures[0].size())},Mesh{VerticalBackdropMesh(false,m_bgTextures[1].size())}}
		, m_bgTransform(GameRender::VerticalEnabled() ? VerticalBackdrop(m_camera,kBGBillboardPosition) : BackgroundBillboard(m_camera, kBGBillboardPosition, kBGBillboardSize))
		, m_layerFrameTextures(LoadLayerFrameTextures(LayerFilePath(chartData, parentPath)))
		, m_layerTransform(GameRender::VerticalEnabled() ? VerticalBackdrop(m_camera,kLayerBillboardPosition) : m_camera.billboard(kLayerBillboardPosition, kLayerBillboardSize))
		, m_jdgoverlay3DGraphics(m_camera)
		, m_hudCanvas(LoadHUDCanvas())
		, m_gaugePanel(ToGaugeCalcType(playOption.gaugeType, playOption.gameMode))
		, m_laserApproachIndicator(chartData)
		, m_moviePanel(MovieFilePath(chartData, parentPath), chartData.bg.legacy.movie.offset / 1000.0 / playOption.nonZeroPlaybackSpeed(), playOption.playbackSpeed, playOption.movieEnabled)
		, m_playOption(playOption)
        , m_layerTilt(chartData.bg.legacy.layer.rotation.tilt && !System::GetCommandLineArgs().contains(U"--vertical-preview-layer-static"))
        , m_layerSpin(chartData.bg.legacy.layer.rotation.spin && !System::GetCommandLineArgs().contains(U"--vertical-preview-layer-static"))
		, m_layerFrameOriginTimeSec(-TimeSecBeforeStart(m_moviePanel.isEnabled()).count())
		, m_layerFrameOriginPulse(static_cast<kson::Pulse>(kson::SecToPulseDouble(m_layerFrameOriginTimeSec, chartData.beat, timingCache)))
	{
        const String layerName=Unicode::FromUTF8(chartData.bg.legacy.layer.filename);
        const FilePath logicalLayer=FileSystem::Extension(layerName).isEmpty()?U"imgs/bg/{}.gif"_fmt(layerName):FileSystem::PathAppend(parentPath,layerName);
        m_tunnelWarp.setProfile(logicalLayer);
        if(m_vertical && m_playOption.showLayer && !m_layerFrameTextures[0].empty())
            m_tunnelWarp.warmUp(m_camera,m_layerTransform,Highway3DGraphics::neutralUpperMidpoint(false),m_layerFrameTextures[0][0]);
		// HUDのCanvasパラメータを初期設定
		const double startBPM = chartData.beat.bpm.contains(0) ? chartData.beat.bpm.at(0) : kDefaultBPM;

		String title = Unicode::FromUTF8(chartData.meta.title);
		String artist = Unicode::FromUTF8(chartData.meta.artist);
		FilePath titleImgPath;
		FilePath artistImgPath;

		if (!chartData.meta.titleImgFilename.empty())
		{
			titleImgPath = FileSystem::PathAppend(parentPath, Unicode::FromUTF8(chartData.meta.titleImgFilename));
			if (FileSystem::IsFile(titleImgPath))
			{
				title = U"";
			}
			else
			{
				titleImgPath.clear();
			}
		}

		if (!chartData.meta.artistImgFilename.empty())
		{
			artistImgPath = FileSystem::PathAppend(parentPath, Unicode::FromUTF8(chartData.meta.artistImgFilename));
			if (FileSystem::IsFile(artistImgPath))
			{
				artist = U"";
			}
			else
			{
				artistImgPath.clear();
			}
		}

		m_hudCanvas->setParamValues({
			{ U"title", title },
			{ U"titleImgFilePath", titleImgPath },
			{ U"artist", artist },
			{ U"artistImgFilePath", artistImgPath },
			{ U"levelNumber", chartData.meta.level },
			{ U"bpmNumberText", FormatBPM(startBPM) },
			{ U"difficultyIndex", chartData.meta.difficulty.idx },
			{ U"hispeedValueText", HispeedUtils::ToDisplayString(playOption.hispeedSetting) },
			{ U"hispeedValueTextEffective", Format(playOption.hispeedSetting.value) },
			{ U"jacketFilePath", FsUtils::ResolveJacketPath(parentPath, Unicode::FromUTF8(chartData.meta.jacketFilename)) },
		});
	}

    Array<double> GraphicsMain::geometrySignature(const ViewStatus& view) const {
        Array<double> result{double(GameRender::Width()),double(GameRender::Height())};
        const auto append=[&](Float3 point){const auto p=m_camera.worldToScreenPoint(point);result.push_back(p.x);result.push_back(p.y);};
        const auto& source=m_highway3DGraphics.sourceMesh();
        const auto mesh=GameRender::VerticalEnabled()?Vertical::DisplayHighwayMesh(source,Vertical::DisplayLaneSpan(source,m_camera,view)):source;
        const double angle=Math::ToRadians(view.camStatus.rotationDeg+view.camStatus.rotationDegHighway)+view.tiltRadians;
        const auto origin=Camera::CamShiftXMatrix(view.camStatus.shiftX)*GearTiltTransformMatrix(angle);
        for(const auto& vertex:mesh.vertices)append(origin.transformPoint(vertex.pos));
        const auto anchor=m_highway3DGraphics.displayNoteAnchor();
        append(origin.transformPoint(anchor));
        append(origin.transformPoint(anchor+Float3{-4,0,0}));append(origin.transformPoint(anchor+Float3{4,0,0}));
        if(GameRender::VerticalEnabled())append(Vertical::BarWorldCenter(view));
        return result;
    }

	void GraphicsMain::prepareMovie(double globalOffsetSec)
	{
		m_moviePanel.prepare(globalOffsetSec);
	}

	void GraphicsMain::update(const GameStatus& gameStatus, const ViewStatus& viewStatus, const kson::TimingCache& timingCache)
	{
		m_comboOverlay.update(viewStatus, gameStatus.currentTimeSec);
		m_scoreAnimator.update(viewStatus.score);
		m_highway3DGraphics.update(viewStatus);
        if(m_vertical && m_playOption.showLayer && m_depthEnabled) {
            auto referenceView=viewStatus;
            referenceView.camStatus.zoomTop=0; referenceView.camStatus.zoomBottom=0;
            const auto& source=m_highway3DGraphics.sourceMesh();
            const float farZ=Highway3DGraphics::neutralUpperMidpoint(viewStatus.camStatus.useLegacyZoomTop).z;
            for(int i=0;i<4;++i) m_referenceGear.vertices[i].pos={source.vertices[i].pos.x,0,i<2 ? farZ : -kHighwayPlaneSize.y/2};
            const auto quad=[&](const MeshData& mesh,const ViewStatus& view) {
                Vertical::ProjectiveWall::GearQuad result{};
                const auto span=Vertical::DisplayLaneSpan(mesh,m_camera,view);
                const auto origin=Vertical::HighwayOrigin(view);
                const float half=kHighwayPlaneSize.x/2/static_cast<float>(Vertical::kHighwayUVScaleX);
                const Float3 upper=(mesh.vertices[0].pos+mesh.vertices[1].pos)*.5f;
                const Float3 front=(mesh.vertices[2].pos+mesh.vertices[3].pos)*.5f;
                const Float3 lower=upper+(front-upper)*static_cast<float>(span.anchorT);
                for(int i=0;i<4;++i) {
                    auto point=i<2 ? upper : lower;point.x+=(i%2 ? half : -half);
                    result[i]=m_camera.worldToScreenPoint(origin.transformPoint(point));
                }
                return result;
            };
            const double scaled=Camera::ScaledCamZoomBottomValue(viewStatus.camStatus.zoomBottom);
            const double baseline=Abs(Camera::ScaledCamZoomBottomValue(scaled<0 ? -100.0 : 100.0));
            m_tunnelWarp.updateGear(m_camera,layerTransform(viewStatus),quad(source,viewStatus),quad(m_referenceGear,referenceView),scaled/baseline);
        }
		m_laserApproachIndicator.update(gameStatus, timingCache);
        if (TimingHudCapture::Enabled() && System::GetCommandLineArgs().contains(U"--play-judge-indicator-fixture")) {
            PlayJudgeIndicatorFixture::Configure();
            m_playJudgeIndicator.update(PlayJudgeIndicatorFixture::View(viewStatus),m_playOption.liveTimingHud().judgeColor);
        }
        else if(!m_playOption.judgeSetting)m_playJudgeIndicator.update(viewStatus,m_playOption.liveTimingHud().judgeColor);
		m_moviePanel.update(gameStatus.currentTimeSec, gameStatus.isPaused);
        m_timingOverlay.update(m_playOption.liveFastSlowMode(),m_playOption.liveTimingHud(),viewStatus.timingFeedback,
            gameStatus.currentTimeSec,!gameStatus.isPaused && !gameStatus.playFinishStatus.has_value());
        if(TimingHudCapture::Enabled() && !TimingHudCapture::IsMenu()) {
            TimingHudCapture::Begin();
            m_timingOverlay.update(TimingHudCapture::Stage()==8 ? FastSlowMode::kOrigin : FastSlowMode::kNew,
                TimingHudCapture::Settings(),TimingHudCapture::Event(gameStatus.currentTimeSec),gameStatus.currentTimeSec);
        }

		// EX display reads the existing confirmed total; no interpolation or score recomputation.
        int64 hudExScore=viewStatus.exScore;
        if(TimingHudCapture::Enabled() && !TimingHudCapture::IsMenu() && System::GetCommandLineArgs().contains(U"--ex-hud-fixture"))
            hudExScore=std::array<int64,10>{0,5,42,999,12345,19999,19999,21,123,0}[TimingHudCapture::Stage()];
        // HUDのCanvasパラメータを更新
		m_hudCanvas->setParamValues({
			{ U"scoreNumber", (m_playOption.skinPreview ? viewStatus.score : m_scoreAnimator.displayedScore()) },
			{ U"exScoreNumber", Format(hudExScore) },
			{ U"fpsNumber", static_cast<int32>(Profiler::FPS()) },
			{ U"bpmNumberText", FormatBPM(gameStatus.currentBPM) },
			{ U"timingAdjustVisible", viewStatus.timingAdjustMs != 0 },
			{ U"timingAdjustSignIndex", viewStatus.timingAdjustMs > 0 ? 1 : 0 },
			{ U"timingAdjustNumber", Abs(viewStatus.timingAdjustMs) },
		});

		m_hudCanvas->update(GameRender::Size(), noco::HitTestEnabledYN::No);
        if(TimingHudCapture::Enabled() && !TimingHudCapture::IsMenu() && TimingHudCapture::Fraction()>.85 && m_exHudCaptureStage!=TimingHudCapture::Stage()){
            m_exHudCaptureStage=TimingHudCapture::Stage();JSON report;
            report[U"exScore"]=Format(hudExScore);report[U"actualExScore"]=viewStatus.exScore;report[U"score"]=viewStatus.score;report[U"preview"]=m_playOption.skinPreview;
            report[U"font"]=SkinPreset::Resolve(U"result_scorenum.png");
            for(const auto name:{U"ScoreCaption",U"ScoreNumber",U"ExScoreCaption",U"ExScoreNumber"}){
                if(const auto node=m_hudCanvas->findByName(name,noco::RecursiveYN::Yes,noco::IncludeSubCanvasYN::Yes)){
                    const auto rect=node->transformedQuad().boundingRect();
                    report[name][U"x"]=rect.x;report[name][U"y"]=rect.y;report[name][U"width"]=rect.w;report[name][U"height"]=rect.h;
                }
            }
            report.save(FsUtils::GetResourcePath(U"ex-hud-{}.json"_fmt(m_exHudCaptureStage)));
        }
	}

	void GraphicsMain::draw(const kson::ChartData& chartData, const std::array<HashSet<kson::Pulse>, kson::kNumLaserLanesSZ>& laserCurvedPulses, const kson::TimingCache& timingCache, const GameStatus& gameStatus, const ViewStatus& viewStatus, const Scroll::HighwayScrollContext& highwayScrollContext, Duration bgmDuration) const
	{
		// Background-only diagnostics avoid note/HUD work entirely.
        if(!m_backgroundOnly) {
        // 各レンダーテクスチャを用意
		m_highway3DGraphics.draw2D(chartData, laserCurvedPulses, m_playOption, timingCache, gameStatus, viewStatus, highwayScrollContext);
		m_jdgoverlay3DGraphics.draw2D(gameStatus, viewStatus);
		Graphics2D::Flush();
        }

		// 3D空間を描画
		Graphics3D::SetCameraTransform(m_camera);
		if (m_playOption.showBG)
		{
			drawBG(viewStatus);
		}
		if (m_playOption.showLayer)
		{
			drawLayer(chartData, gameStatus, viewStatus);
		}
		if(m_backgroundOnly) {
            Graphics3D::Flush();
            if(m_warpDebug) m_tunnelWarp.drawDebug();
            return;
        }
        m_highway3DGraphics.draw3D(viewStatus);
		m_jdgline3DGraphics.draw3D(viewStatus, GameRender::VerticalEnabled()
			? Vertical::JudgmentBarWidthScale(m_camera, GameRender::Width()) : 1.0);
		m_jdgoverlay3DGraphics.draw3D(viewStatus,m_camera,m_highway3DGraphics.displayNoteAnchor());
		m_laserCursor3DGraphics.draw3D(gameStatus,viewStatus,m_camera,m_highway3DGraphics.displayNoteAnchor());

		// 手前に表示する2DのHUDを描画
        if (m_playOption.skinPreview || m_playOption.judgeSetting) Graphics3D::Flush();
        if(m_playOption.judgeSetting){m_timingOverlay.draw();return;}
		{
			const double preStartOffset = m_moviePanel.isEnabled() ? kPreStartOffsetWithMovieSec : kPreStartOffsetSec;
			const double totalDurationSec = bgmDuration.count() + preStartOffset;
			const double progress = Clamp(gameStatus.currentTimeSec / totalDurationSec, 0.0, 1.0);

			m_hudCanvas->setParamValues({
				{ U"positionMarkerProgress", progress },
				{ U"hispeedValueText", HispeedUtils::ToDisplayString(highwayScrollContext.highwayScroll().hispeedSetting()) },
				{ U"hispeedValueTextEffective", Format(highwayScrollContext.currentHispeedWithScrollSpeed()) },
			});
		}

		m_hudCanvas->draw();
        m_playJudgeIndicator.draw();
		if (GameRender::VerticalEnabled())
		{
			const auto viewport = Vertical::Viewport::Fit(GameRender::Width(), GameRender::Height());
			const double legacyScale = GameRender::Height() / 480.0;
			m_gaugePanel.drawVertical(viewStatus.gaugePercentage, gameStatus.currentPulse, viewStatus.apsProtected, viewStatus.effectiveGaugeType, viewStatus.gaugeVisual);
			{
				const Vec2 source{GameRender::Width() / 2.0, Scaled(313.0)};
				const auto position = Vertical::HUDDefinition()[U"comboAnchor"];
				const Vec2 target{viewport.x(position[0].get<double>()), viewport.y(position[1].get<double>())};
				const double factor = 1.4 * viewport.scale / legacyScale;
				const Transformer2D transform{Mat3x2::Scale(factor).translated(target - source * factor), TransformCursor::No};
				m_comboOverlay.draw(gameStatus.currentTimeSec);
			}
			{
				const Vec2 source = Vec2{GameRender::Center()} - Vec2{Scaled(0, 40)};
				const Vec2 target = Vertical::HUDRect(U"Achievement").center();
				const double factor = 360 * viewport.scale / (270 * legacyScale);
				const Transformer2D transform{Mat3x2::Scale(factor).translated(target - source * factor), TransformCursor::No};
				m_achievementPanel.draw(gameStatus);
			}
			const double progress = Clamp(gameStatus.currentTimeSec / (bgmDuration.count() +
				(m_moviePanel.isEnabled() ? kPreStartOffsetWithMovieSec : kPreStartOffsetSec)), 0.0, 1.0);
			const auto progressRect = Vertical::HUDRect(U"Progress");
			const ScopedRenderStates2D sampler{SamplerState::ClampLinear};
			const Texture detail=TextureAsset(U"minfo_detail.png");
			const Texture marker=TextureAsset(U"minfo_cur.png");
			detail(12,106,456,14).resized(progressRect.size).draw(progressRect.pos);
			const double markerSize=24*viewport.scale;
			marker.resized(markerSize).drawAt(progressRect.x+markerSize/2+(progressRect.w-markerSize)*progress,progressRect.center().y);
		}
		else
		{
			m_gaugePanel.draw(viewStatus.gaugePercentage, gameStatus.currentPulse, viewStatus.apsProtected, viewStatus.effectiveGaugeType, viewStatus.gaugeVisual);
			m_comboOverlay.draw(gameStatus.currentTimeSec);
			m_achievementPanel.draw(gameStatus);
		}
		Optional<double> approachAnchorY;
		if (GameRender::VerticalEnabled())
		{
			const auto matrix = Vertical::HUDGearOrigin(viewStatus);
			approachAnchorY = m_camera.worldToScreenPoint(matrix.transformPoint(m_highway3DGraphics.displayNoteAnchor())).y;
		}
		m_laserApproachIndicator.draw(gameStatus.currentTimeSec, approachAnchorY);
		m_moviePanel.draw();
        m_timingOverlay.draw();

		// HARDゲージ落ち時の赤色オーバーレイ
		if (gameStatus.playFinishStatus.has_value() && gameStatus.playFinishStatus->isHardFailed)
		{
			constexpr double kFadeTimeSec = 0.75;
			const double elapsedSec = gameStatus.currentTimeSec - gameStatus.playFinishStatus->finishTimeSec;
			const double t = Clamp(elapsedSec / kFadeTimeSec, 0.0, 1.0);
			const uint8 g = static_cast<uint8>(48 + 128 - static_cast<int32>(128 * t));
			const Color overlayColor{ 255, g, 0 };

			const ScopedRenderStates2D blend{ BlendState::Additive };
			GameRender::Rect().draw(overlayColor);
		}
	}

	bool GraphicsMain::hasMovie() const
	{
		return m_moviePanel.isEnabled();
	}

	void GraphicsMain::seekMoviePosSec(SecondsF posSec)
	{
		m_moviePanel.seekPosSec(posSec);
	}
}
