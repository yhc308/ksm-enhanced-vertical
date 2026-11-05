#include "AssetManagement.hpp"

namespace AssetManagement
{
	namespace
	{
		constexpr StringView kFontAssetSystem = U"System";
		constexpr StringView kFontAssetSystemBold = U"SystemBold";
		constexpr StringView kFontResourcePathJa = U"assets/font/KSM-JA-Medium.ttf";
		constexpr StringView kFontResourcePathSc = U"assets/font/KSM-SC-Medium.ttf";
		constexpr StringView kFontResourcePathTc = U"assets/font/KSM-TC-Medium.ttf";

		FilePath GetFontPathForCurrentLanguage()
		{
			const I18n::StandardLanguage language = I18n::CurrentLanguage();
			switch (language)
			{
			case I18n::StandardLanguage::SimplifiedChinese:
				return FilePath{ kFontResourcePathSc };
			case I18n::StandardLanguage::TraditionalChinese:
				return FilePath{ kFontResourcePathTc };
			default:
				return FilePath{ kFontResourcePathJa };
			}
		}

		void RegisterTextureAssets()
		{
			SkinPreset::RegisterTextures();
		}

		void RegisterAudioAssets()
		{
			// "se"ディレクトリ以下の全ファイルを登録
			const FilePath seFullPath = FsUtils::GetResourcePath(U"se");
			for (const FilePath& path : FileSystem::DirectoryContents(seFullPath, Recursive::Yes))
			{
				AudioAsset::Register(FileSystem::RelativePath(path, seFullPath), path);
			}
		}

		void RegisterPixelShaderAssets()
		{
			const FilePath shaderDir = FsUtils::GetResourcePath(U"shaders");
#ifdef __EMSCRIPTEN__
			PixelShaderAsset::Register(U"ApsColorDodge", ESSL{ FilePath{ FileSystem::PathAppend(shaderDir, U"aps_color_dodge.essl.frag") }, { { U"PSConstants2D", 0 } } });
#else
			PixelShaderAsset::Register(U"ApsColorDodge", HLSL{ FilePath{ FileSystem::PathAppend(shaderDir, U"aps_color_dodge.hlsl") }, U"PS" } | GLSL{ FilePath{ FileSystem::PathAppend(shaderDir, U"aps_color_dodge.frag") }, { { U"PSConstants2D", 0 } } });
#endif
#ifdef __EMSCRIPTEN__
			PixelShaderAsset::Register(U"MonochromeAlpha",
									   ESSL{ FilePath{ FileSystem::PathAppend(shaderDir, U"monochrome_alpha.essl.frag") }, { { U"PSConstants2D", 0 } } });
#else
			PixelShaderAsset::Register(U"MonochromeAlpha",
									   HLSL{ FilePath{ FileSystem::PathAppend(shaderDir, U"monochrome_alpha.hlsl") }, U"PS" } | GLSL{ FilePath{ FileSystem::PathAppend(shaderDir, U"monochrome_alpha.frag") }, { { U"PSConstants2D", 0 } } });
#endif
		}

		void RegisterFontAssets()
		{
			const FilePath fontPath = GetFontPathForCurrentLanguage();
			if (FontAsset::IsRegistered(kFontAssetSystem))
			{
				FontAsset::Unregister(kFontAssetSystem);
			}
			if (FontAsset::IsRegistered(kFontAssetSystemBold))
			{
				FontAsset::Unregister(kFontAssetSystemBold);
			}
#ifdef __linux
			constexpr StringView kResourceDir = U"resources/";
			FontAsset::Register(kFontAssetSystem, FontMethod::MSDF, 44, kResourceDir + fontPath, FontStyle::Default);
			FontAsset::Register(kFontAssetSystemBold, FontMethod::MSDF, 44, kResourceDir + fontPath, FontStyle::Bold);
#else
			FontAsset::Register(kFontAssetSystem, FontMethod::MSDF, 44, Resource(fontPath), FontStyle::Default);
			FontAsset::Register(kFontAssetSystemBold, FontMethod::MSDF, 44, Resource(fontPath), FontStyle::Bold);
#endif
			// Keep the selected language's glyph shapes first. Fallback fonts only
			// supply missing characters in song names; never rewrite their paths.
			static const Array<Font> fallbackFonts = []
			{
				Array<Font> fonts;
				for (const auto path : { kFontResourcePathJa, kFontResourcePathSc, kFontResourcePathTc })
				{
#ifdef __linux
					fonts.emplace_back(FontMethod::MSDF, 44, Resource(U"resources/" + String{ path }));
#else
					fonts.emplace_back(FontMethod::MSDF, 44, Resource(path));
#endif
				}
				if (Font::IsAvailable(Typeface::CJK_Regular_KR))
					fonts.emplace_back(FontMethod::MSDF, 44, Typeface::CJK_Regular_KR);
#ifdef _WIN32
				const FilePath emojiPath = FileSystem::PathAppend(FileSystem::GetFolderPath(SpecialFolder::SystemFonts), U"seguiemj.ttf");
				if (FileSystem::IsFile(emojiPath))
					fonts.emplace_back(44, emojiPath);
#endif
				if (Font::IsAvailable(Typeface::ColorEmoji))
					fonts.emplace_back(44, Typeface::ColorEmoji);
				else if (Font::IsAvailable(Typeface::MonochromeEmoji))
					fonts.emplace_back(44, Typeface::MonochromeEmoji);
				return fonts;
			}();
			for (const auto& font : fallbackFonts)
				if (font)
				{
					FontAsset(kFontAssetSystem).addFallback(font);
					FontAsset(kFontAssetSystemBold).addFallback(font);
				}
		}
	}

	void RegisterAssets()
	{
		// 注意: アセットの実際のロードはここではなく、各シーンでの使用時に実施される
		RegisterTextureAssets();
		RegisterAudioAssets();
		RegisterPixelShaderAssets();
		// フォントは言語読み込み後にRefreshSystemFontForCurrentLanguage()で登録される
	}

	void RefreshSystemFontForCurrentLanguage()
	{
		if (FontAsset::IsRegistered(kFontAssetSystem))
		{
			FontAsset::Unregister(kFontAssetSystem);
			FontAsset::Unregister(kFontAssetSystemBold);
		}
		RegisterFontAssets();
	}

	Font SystemFont()
	{
		return FontAsset(kFontAssetSystem);
	}

	Font SystemFontBold()
	{
		return FontAsset(kFontAssetSystemBold);
	}
}
