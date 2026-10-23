#include "ksmaudio/Diagnostics.hpp"
#include "Common/AudioOutputSettings.hpp"
#include <Siv3D.hpp>
#ifdef _WIN32
#include <bass.h>
#endif
#include "Diagnostics/SkinPresetFixture.hpp"
#include "Diagnostics/RenderGeometryFixture.hpp"
#include <CoTaskLib.hpp>
#include <NocoUI.hpp>
#include "Common/FrameRateLimit.hpp"
#include "Common/IMEUtils.hpp"
#include "Common/AssetManagement.hpp"
#include "Addon/AutoMuteAddon.hpp"
#include "Addon/CommonSEAddon.hpp"
#include "Addon/DisableIMEAddon.hpp"
#include "ksmaudio/ksmaudio.hpp"
#include <ksmaxis/ksmaxis.hpp>
#include "RuntimeConfig.hpp"
#include "Scenes/Title/TitleScene.hpp"
#include "Scenes/Option/OptionScene.hpp"
#include "Ini/AudioCalibration.hpp"
#include "Diagnostics/AudioTimingFixture.hpp"
#include "Diagnostics/AudioEndGameFixture.hpp"
#include "Diagnostics/ApsVideoFixture.hpp"
#include "Diagnostics/TimingHudFixture.hpp"
#include "Diagnostics/ExScoreFixture.hpp"
#include "Diagnostics/ApsFixture.hpp"
#include "Diagnostics/JudgeSettingFixture.hpp"
#include "Diagnostics/TimingHudCapture.hpp"
#include "Scenes/Select/SelectScene.hpp"
#include "Scenes/Result/ResultScene.hpp"
#include "Scenes/Play/PlayScene.hpp"
#include "Scenes/PlayPrepare/PlayPrepareScene.hpp"
#include "TestPlayArgs.hpp"
#include "Input/KeyConfig.hpp"
#include "Input/InputUtils.hpp"
#include "Vertical/VerticalDisplay.hpp"
#include "Vertical/VerticalSelfTest.hpp"
#include "Vertical/SpinRegression.hpp"
#include "Vertical/ModOptionsScene.hpp"
#include "Vertical/VerticalRenderPreview.hpp"
#include "Vertical/JudgmentReplayTest.hpp"
#include "Vertical/MenuInputTest.hpp"
#include "Vertical/DisplayTransitionTest.hpp"
#include "Scenes/Option/OptionScene.hpp"
#include "Scenes/Select/SelectScene.hpp"
#include "Scenes/Result/ResultScene.hpp"

#ifdef __APPLE__
#include <ksmplatform_macos/input_method.h>
#endif

void CreateHighScoreBackup()
{
#ifndef __APPLE__
	const FilePath scoreBackupDir = FileSystem::PathAppend(FsUtils::AppDataDirectoryPath(), U"score_backup");

	// 既にバックアップフォルダが存在する場合は何もしない
	if (FileSystem::Exists(scoreBackupDir))
	{
		return;
	}

	// scoreフォルダを再帰的にコピー
	const FilePath scoreDir = FsUtils::ScoreDirectoryPath();
	FileSystem::Copy(scoreDir, scoreBackupDir, CopyOption::UpdateExisting);
#endif
}

void CreateAppDataDirectory()
{
#ifdef __APPLE__
	const FilePath appDataDir = FsUtils::AppDataDirectoryPath();
	Logger << U"[ksm info] AppDataDirectory path: " << appDataDir;

	// ディレクトリが存在しない場合は作成
	if (!FileSystem::Exists(appDataDir))
	{
		Logger << U"[ksm info] Creating AppDataDirectory...";
		FileSystem::CreateDirectories(appDataDir);
		const bool created = FileSystem::Exists(appDataDir);
		Logger << U"[ksm info] Created: " << created;
	}
	else
	{
		Logger << U"[ksm info] AppDataDirectory already exists";
	}
#endif
}

void CopyResourcesIfNeeded()
{
#ifdef __APPLE__
	const FilePath songsDestPath = FsUtils::SongsDirectoryPath();
	const FilePath songsDefaultSrcPath = FsUtils::SongsDefaultDirectoryPath();

	Logger << U"[ksm info] Songs dest path: " << songsDestPath;
	Logger << U"[ksm info] Songs default src path: " << songsDefaultSrcPath;
	Logger << U"[ksm info] Songs dest exists: " << FileSystem::Exists(songsDestPath);
	Logger << U"[ksm info] Songs default src exists: " << FileSystem::Exists(songsDefaultSrcPath);

	// songsフォルダが存在しない場合、songs_defaultからコピー
	if (!FileSystem::Exists(songsDestPath))
	{
		if (FileSystem::Exists(songsDefaultSrcPath))
		{
			Logger << U"[ksm info] Copying songs_default to songs...";

			// Siv3DのFileSystem::CopyだとApplication Support内へのファイルコピーに失敗するためネイティブライブラリを使用
			const std::string srcPathUtf8 = songsDefaultSrcPath.toUTF8();
			const std::string dstPathUtf8 = songsDestPath.toUTF8();

			const bool copyResult = KSMPlatformMacOS_CopyDirectory(srcPathUtf8.c_str(), dstPathUtf8.c_str());
			Logger << U"[ksm info] Copy result: " << copyResult;
			Logger << U"[ksm info] Songs dest exists after copy: " << FileSystem::Exists(songsDestPath);

			// コピー後のディレクトリ内容を確認
			if (FileSystem::Exists(songsDestPath))
			{
				const auto copiedFiles = FileSystem::DirectoryContents(songsDestPath);
				Logger << U"[ksm info] Files in songs directory: " << copiedFiles.size();
			}
		}
		else
		{
			Logger << U"[ksm info] songs_default does not exist, skipping copy";
		}
	}
	else
	{
		Logger << U"[ksm info] songs directory already exists, skipping copy";
	}
#endif
}

void OutputLicenseTxt()
{
	// ライセンス情報を取得
	Array<LicenseInfo> licenses = LicenseManager::EnumLicenses();

	// BASS Audio LibraryとBASS_FXのライセンス情報を追加(Web版はBASS未使用のため除外)
#ifndef __EMSCRIPTEN__
	LicenseInfo bassLicense;
	bassLicense.title = U"BASS Audio Library";
	bassLicense.copyright = U"Copyright (c) 1999-2024 Un4seen Developments Ltd.\nhttps://www.un4seen.com/";
	bassLicense.text = U"BASS is free for non-commercial use.\nIf you are using BASS in a commercial product, you must obtain a license.";
	licenses.push_back(bassLicense);

	// BASS_FXのライセンス情報を追加
	LicenseInfo bassFxLicense;
	bassFxLicense.title = U"BASS_FX";
	bassFxLicense.copyright = U"Copyright (c) 2002-2018 Arthur Aminov (JOBnik!)\nhttp://www.jobnik.org";
	bassFxLicense.text = U"BASS_FX is fully useable in commercial software, as long as credit is given.\n"
		U"\n"
		U"Credits:\n"
		U"* BiQuad filters - Robert Bristow-Johnson\n"
		U"* Peaking Equalizer (BiQuad filter) - Manu Webber\n"
		U"* Tempo/Pitch/Rate/BPM [SoundTouch v2.0.0] - Copyright (c) 2002-2017 Olli Parviainen (LGPL license)\n"
		U"* Auto Wah, Chorus, Distortion, Echo and Phaser - Copyright (c) 2000 Aleksey Smoli\n"
		U"* Freeverb - Copyright (c) 2000 Jezar at Dreampoint (Public domain)\n"
		U"* Pitch shifting using FFT [smbPitchShift v1.2] - Copyright (c) 1999-2009 Stephan M. Bernsee";
	licenses.push_back(bassFxLicense);
#else
	// Web版のオーディオバックエンドで使用しているライブラリのライセンス情報を追加
	LicenseInfo miniaudioLicense;
	miniaudioLicense.title = U"miniaudio";
	miniaudioLicense.copyright = U"Copyright (c) 2025 David Reid\nhttps://miniaud.io/";
	miniaudioLicense.text = U"This software is available as a choice of the following licenses (MIT No Attribution / Public Domain).\n"
		U"\n"
		U"Permission is hereby granted, free of charge, to any person obtaining a copy of\n"
		U"this software and associated documentation files (the \"Software\"), to deal in\n"
		U"the Software without restriction, including without limitation the rights to\n"
		U"use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies\n"
		U"of the Software, and to permit persons to whom the Software is furnished to do so.\n"
		U"\n"
		U"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
		U"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
		U"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
		U"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
		U"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
		U"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.";
	licenses.push_back(miniaudioLicense);

	LicenseInfo stbVorbisLicense;
	stbVorbisLicense.title = U"stb_vorbis";
	stbVorbisLicense.copyright = U"Copyright (c) 2017 Sean Barrett\nhttps://github.com/nothings/stb";
	stbVorbisLicense.text = U"This software is available as a choice of the following licenses (MIT License / Public Domain).\n"
		U"\n"
		U"Permission is hereby granted, free of charge, to any person obtaining a copy of\n"
		U"this software and associated documentation files (the \"Software\"), to deal in\n"
		U"the Software without restriction, including without limitation the rights to\n"
		U"use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies\n"
		U"of the Software, and to permit persons to whom the Software is furnished to do\n"
		U"so, subject to the following conditions:\n"
		U"The above copyright notice and this permission notice shall be included in all\n"
		U"copies or substantial portions of the Software.\n"
		U"\n"
		U"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
		U"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
		U"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
		U"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
		U"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
		U"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.";
	licenses.push_back(stbVorbisLicense);
#endif

	// libksonのライセンス情報を追加
	LicenseInfo ksonLicense;
	ksonLicense.title = U"libkson";
	ksonLicense.copyright = U"Copyright (c) 2019-2022 masaka";
	ksonLicense.text = U"Permission is hereby granted, free of charge, to any person obtaining a copy\n"
		U"of this software and associated documentation files (the \"Software\"), to deal\n"
		U"in the Software without restriction, including without limitation the rights\n"
		U"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
		U"copies of the Software, and to permit persons to whom the Software is\n"
		U"furnished to do so, subject to the following conditions:\n"
		U"\n"
		U"The above copyright notice and this permission notice shall be included in all\n"
		U"copies or substantial portions of the Software.\n"
		U"\n"
		U"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
		U"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
		U"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
		U"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
		U"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
		U"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
		U"SOFTWARE.";
	licenses.push_back(ksonLicense);

	// ksmaxisのライセンス情報を追加
	LicenseInfo ksmaxisLicense;
	ksmaxisLicense.title = U"ksmaxis";
	ksmaxisLicense.copyright = U"Copyright (c) 2025 masaka";
	ksmaxisLicense.text = U"Permission is hereby granted, free of charge, to any person obtaining a copy\n"
		U"of this software and associated documentation files (the \"Software\"), to deal\n"
		U"in the Software without restriction, including without limitation the rights\n"
		U"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
		U"copies of the Software, and to permit persons to whom the Software is\n"
		U"furnished to do so, subject to the following conditions:\n"
		U"\n"
		U"The above copyright notice and this permission notice shall be included in all\n"
		U"copies or substantial portions of the Software.\n"
		U"\n"
		U"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
		U"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
		U"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
		U"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
		U"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
		U"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
		U"SOFTWARE.";
	licenses.push_back(ksmaxisLicense);

	// KSMフォントのライセンス情報を追加
	LicenseInfo ksmFontLicense;
	ksmFontLicense.title = U"KSM Fonts (KSM-JA/SC/TC-Medium)";
	ksmFontLicense.copyright = U"Component fonts:\n"
		U"1. Tektur (Modified as Tektur-KSM) - Copyright 2023 The Tektur Project Authors, Modified by K-Shoot MANIA Project\n"
		U"2. Corporate Logo ver3 - Copyright LOGOTYPE.JP, Based on Source Han Sans (Copyright 2014-2020 Adobe)\n"
		U"3. Noto Sans, Noto Sans JP/KR/SC/TC, Noto Sans Math, Noto Sans Symbols, Noto Music, Arabic, Thai, Hebrew, Cherokee - Copyright Google Inc. and Adobe Inc.";
	ksmFontLicense.text = U"All component fonts are licensed under the SIL Open Font License, Version 1.1.\n"
		U"\n"
		U"Permission is hereby granted, free of charge, to any person obtaining a copy of the Font Software,\n"
		U"to use, study, copy, merge, embed, modify, redistribute, and sell modified and unmodified copies\n"
		U"of the Font Software, subject to the following conditions:\n"
		U"\n"
		U"1) Neither the Font Software nor any of its individual components, in Original or Modified Versions,\n"
		U"   may be sold by itself.\n"
		U"2) Original or Modified Versions of the Font Software may be bundled, redistributed and/or sold with\n"
		U"   any software, provided that each copy contains the above copyright notice and this license.\n"
		U"3) The Font Software, modified or unmodified, in part or in whole, must be distributed entirely under\n"
		U"   this license, and must not be distributed under any other license.";
	licenses.push_back(ksmFontLicense);

	// SQLiteのライセンス情報を追加
	LicenseInfo sqliteLicense;
	sqliteLicense.title = U"SQLite";
	sqliteLicense.copyright = U"https://www.sqlite.org/copyright.html";
	sqliteLicense.text = U"SQLite Is Public Domain\n"
		U"\n"
		U"All of the code and documentation in SQLite has been dedicated to the public\n"
		U"domain by the authors. All code authors, and representatives of the companies\n"
		U"they work for, have signed affidavits dedicating their contributions to the\n"
		U"public domain and originals of those signed affidavits are stored in a firesafe\n"
		U"at the main offices of Hwaci. All contributors are citizens of countries that\n"
		U"allow creative works to be dedicated into the public domain. Anyone is free to\n"
		U"copy, modify, publish, use, compile, sell, or distribute the original SQLite\n"
		U"code, either in source code form or as a compiled binary, for any purpose,\n"
		U"commercial or non-commercial, and by any means.\n"
		U"\n"
		U"The previous paragraph applies to the deliverable code and documentation in\n"
		U"SQLite - those parts of the SQLite library that you actually bundle and ship\n"
		U"with a larger application. Some scripts used as part of the build process (for\n"
		U"example the \"configure\" scripts generated by autoconf) might fall under other\n"
		U"open-source licenses. Nothing from these build scripts ever reaches the final\n"
		U"deliverable SQLite library, however, and so the licenses associated with those\n"
		U"scripts should not be a factor in assessing your rights to copy and use the\n"
		U"SQLite library.\n"
		U"\n"
		U"All of the deliverable code in SQLite has been written from scratch. No code\n"
		U"has been taken from other projects or from the open internet. Every line of\n"
		U"code can be traced back to its original author, and all of those authors have\n"
		U"public domain dedications on file. So the SQLite code base is clean and is\n"
		U"uncontaminated with licensed code from other projects.";
	licenses.push_back(sqliteLicense);

	// SQLiteCppのライセンス情報を追加
	LicenseInfo sqliteCppLicense;
	sqliteCppLicense.title = U"SQLiteCpp";
	sqliteCppLicense.copyright = U"Copyright (c) 2012-2025 Sebastien Rombauts (sebastien.rombauts@gmail.com)";
	sqliteCppLicense.text = U"Permission is hereby granted, free of charge, to any person obtaining a copy\n"
		U"of this software and associated documentation files (the \"Software\"), to deal\n"
		U"in the Software without restriction, including without limitation the rights\n"
		U"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
		U"copies of the Software, and to permit persons to whom the Software is furnished\n"
		U"to do so, subject to the following conditions:\n"
		U"\n"
		U"The above copyright notice and this permission notice shall be included in all\n"
		U"copies or substantial portions of the Software.\n"
		U"\n"
		U"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
		U"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
		U"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
		U"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\n"
		U"WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR\n"
		U"IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.";
	licenses.push_back(sqliteCppLicense);

	// stb_imageのライセンス情報を追加
	LicenseInfo stbImageLicense;
	stbImageLicense.title = U"stb_image";
	stbImageLicense.copyright = U"Copyright (c) 2017 Sean Barrett\nhttps://github.com/nothings/stb\nLicensed under the MIT License (ALTERNATIVE A of stb's dual license).";
	stbImageLicense.text = U"Permission is hereby granted, free of charge, to any person obtaining a copy of\n"
		U"this software and associated documentation files (the \"Software\"), to deal in\n"
		U"the Software without restriction, including without limitation the rights to\n"
		U"use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies\n"
		U"of the Software, and to permit persons to whom the Software is furnished to do\n"
		U"so, subject to the following conditions:\n"
		U"\n"
		U"The above copyright notice and this permission notice shall be included in all\n"
		U"copies or substantial portions of the Software.\n"
		U"\n"
		U"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
		U"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
		U"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
		U"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
		U"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
		U"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
		U"SOFTWARE.";
	licenses.push_back(stbImageLicense);

	// アルファベット順にソート
	licenses.sort_by([](const LicenseInfo& a, const LicenseInfo& b)
	{
		return a.title < b.title;
	});

	// ファイルに保存
	TextWriter writer(U"license.txt");
	if (!writer)
	{
		Logger << U"[ksm error] Failed to output license.txt";
		return;
	}

	writer.writeln(U"●使用ライブラリのライセンス表記●");

	for (const auto& license : licenses)
	{
		writer.writeln(U"====================");
		writer.writeln(license.title);
		writer.writeln(U"====================");
		if (license.copyright)
		{
			writer.writeln(license.copyright);
			writer.writeln();
		}
		writer.writeln(license.text);
		writer.writeln();
	}
}

#ifdef _WIN32
ksmaudio::AudioBackend GetAudioBackendFromConfig()
{
	const StringView value = ConfigIni::GetString(ConfigIni::Key::kAudioBackend, ConfigIni::Value::AudioBackend::kDefault);
    if (value == U"asio" || value.starts_with(U"asio:")) return ksmaudio::AudioBackend::ASIO;
	return value == ConfigIni::Value::AudioBackend::kDirectSound ? ksmaudio::AudioBackend::DirectSound : ksmaudio::AudioBackend::Default;
}
#endif

void KSMMain()
{
#if defined(__linux__)
	FsUtils::InitModulePathForLinux();
#endif

	// Escキーによるプログラム終了を無効化
	System::SetTerminationTriggers(UserAction::CloseButtonClicked);

	// F1キーによるライセンス表示を無効化
	LicenseManager::DisableDefaultTrigger();

	// ウィンドウタイトル
	Window::SetTitle(U"K-Shoot Mania V2.1.0 - Enhanced Vertical Mod (Unofficial Build)");

	// カレントディレクトリを設定
	// (ChangeCurrentDirectoryはここ以外は基本的に使用禁止。どうしても使う必要がある場合は必ずResourceDirectoryPathに戻すこと)
	FileSystem::ChangeCurrentDirectory(FsUtils::ResourceDirectoryPath());
    // Always record normal play as well as QA launches, in a session per process.
    {
        const auto root=FsUtils::GetResourcePath(U"log").toWstr();
        if(!ksmaudio::qa::Start(std::filesystem::path{root})) throw Error{U"Cannot create log folder next to the executable"};
        ksmaudio::qa::Event("BUILD",0,0,"ASIO+Sync automatic logging 2026-10-18");
    }
    ksmaudio::qa::Phase("startup.config");


	// デフォルト色を指定
	Scene::SetBackground(Palette::Black);
	Graphics3D::SetGlobalAmbientColor(Palette::White);
	Graphics3D::SetSunColor(Palette::Black);

	// アプリケーションデータディレクトリを作成(macOSのみ)
	CreateAppDataDirectory();

	// リソースファイルをコピー(macOSのみ)
	CopyResourcesIfNeeded();

	// config.iniを読み込み
	ConfigIni::Load();
	const auto originalVerticalConfig = Vertical::ModConfig::Load();
	const bool previewTitle = System::GetCommandLineArgs().contains(U"--vertical-preview-title");
	const bool previewOptions = System::GetCommandLineArgs().contains(U"--vertical-preview-options") || System::GetCommandLineArgs().contains(U"--vertical-options-test");
	const bool previewHUD = System::GetCommandLineArgs().contains(U"--vertical-preview-hud");
	const bool previewLegacyOptions = System::GetCommandLineArgs().contains(U"--vertical-preview-option-scene");
	const bool previewPrepare = System::GetCommandLineArgs().contains(U"--vertical-preview-prepare");
	const bool previewResult = System::GetCommandLineArgs().contains(U"--vertical-preview-result");
	const bool previewSelect = System::GetCommandLineArgs().contains(U"--vertical-preview-select");
	const bool previewSettings = System::GetCommandLineArgs().contains(U"--vertical-preview-settings");
	const bool displayTest = System::GetCommandLineArgs().contains(U"--vertical-display-test");
	const bool previewMode = previewResult || previewTitle || previewOptions || previewHUD || previewLegacyOptions || previewSelect || previewPrepare || previewSettings || displayTest;
	if (displayTest) Vertical::SetDisplayConfigPersistence(false);
	if (!previewMode && Vertical::ModConfig::MigrateLayout()) ConfigIni::Save();
	if (previewMode)
	{
        Vertical::SettingsPersistence()=false;
        Vertical::SetDisplayConfigPersistence(false);
		auto previewConfig = originalVerticalConfig;
		previewConfig.ResetLayout();
		previewConfig.enabled = true;
        if(System::GetCommandLineArgs().contains(U"--vertical-options-test")) {previewConfig.backgroundDepthEnabled=true;previewConfig.ResetDepthImages();}
        if(System::GetCommandLineArgs().contains(U"--vertical-preview-depth-off"))previewConfig.backgroundDepthEnabled=false;
		previewConfig.displayMode = System::GetCommandLineArgs().contains(U"--vertical-preview-borderless")
			? Vertical::DisplayMode::BorderlessFullscreen : Vertical::DisplayMode::Windowed;
		previewConfig.windowSize = {540, 960};
        if(System::GetCommandLineArgs().contains(U"--vertical-preview-1080")) previewConfig.windowSize={1080,1920};
        if(System::GetCommandLineArgs().contains(U"--vertical-preview-720")) previewConfig.windowSize={720,1280};
        if(System::GetCommandLineArgs().contains(U"--vertical-preview-landscape"))
        {
            previewConfig.enabled = false;
            previewConfig.windowSize = {1280,720};
        }
		previewConfig.Save();
	}
    ksmaudio::qa::Event("CONFIG_BACKEND",0,0,String{ConfigIni::GetString(ConfigIni::Key::kAudioBackend)}.toUTF8());
    ksmaudio::qa::Event("CONFIG_ASSIST_TIMING",ConfigIni::GetInt(ConfigIni::Key::kAssistTick),ConfigIni::GetInt(ConfigIni::Key::kShowFastSlow));

	// ランタイム設定を初期化
	RuntimeConfig::RestoreJudgmentModesFromConfigIni();

	// 言語ファイルを読み込み
	// (BASS初期化失敗時ダイアログの文言に必要なのでBASS初期化前に行う)
	I18n::LoadLanguage(ConfigIni::GetString(ConfigIni::Key::kLanguage));

	// 音声処理を初期化
#ifdef _WIN32
    const String configuredOutput{ConfigIni::GetString(ConfigIni::Key::kAudioBackend)};
    AudioOutputSettings::Log(U"BEGIN startup output initialization: requested={}"_fmt(configuredOutput));
    const std::string asioDriver = configuredOutput.starts_with(U"asio:") ? configuredOutput.substr(5).toUTF8() : "";
	const bool audioInitialized = ksmaudio::InitWithTrace(s3d::Platform::Windows::Window::GetHWND(), GetAudioBackendFromConfig(), asioDriver,
        [](const char* phase) { AudioOutputSettings::Log(Unicode::FromUTF8(phase)); ksmaudio::qa::Event("STARTUP_AUDIO",0,0,phase); ksmaudio::qa::Phase("startup.audio"); });
#else
	const bool audioInitialized = ksmaudio::Init(nullptr);
#endif
#ifdef _WIN32
    AudioOutputSettings::Log(U"END startup output initialization: success={}, backend={}, driver={}, rate={}, frames={}, warning={}"_fmt(
        audioInitialized, static_cast<int>(ksmaudio::GetAudioOutputStatus().backend),
        Unicode::FromUTF8(ksmaudio::GetAudioOutputStatus().driver), ksmaudio::GetAudioOutputStatus().sampleRate,
        ksmaudio::GetAudioOutputStatus().bufferSamples, Unicode::FromUTF8(ksmaudio::GetAudioOutputStatus().warning)));
#endif
	if (!audioInitialized)
	{
		throw Error{ I18n::Get(I18n::General::ErrorBassInitFailed, ksmaudio::GetLastErrorCode()) };
	}

    ksmaudio::qa::Event("OUTPUT", static_cast<std::uint64_t>(ksmaudio::GetAudioOutputStatus().sampleRate),
        ksmaudio::GetAudioOutputStatus().bufferSamples, ksmaudio::GetAudioOutputStatus().driver);
    ksmaudio::qa::Event("OUTPUT_WARNING",0,0,ksmaudio::GetAudioOutputStatus().warning);
    AudioCalibration::Initialize();
    if(System::GetCommandLineArgs().contains(U"--ex-score-test")){
        try{ExScoreFixture::Run();}catch(const Error& error){TextWriter{FsUtils::GetResourcePath(U"ex-score-failure.txt")}.writeln(error);}
        Vertical::OptionsPresentation::Reset();ksmaudio::Terminate();return;
    }
    if (System::GetCommandLineArgs().contains(U"--timing-hud-smoke-test")) {
        try { TimingHudFixture::Run(); }
        catch (const Error& error) { TextWriter{FsUtils::GetResourcePath(U"timing-hud-failure.txt")}.writeln(error); }
        Vertical::OptionsPresentation::Reset();ksmaudio::Terminate(); return;
    }
    if (System::GetCommandLineArgs().contains(U"--audio-timing-smoke-test")) {
        try { AudioTimingFixture::Run(); }
        catch (const Error& error) { TextWriter{FileSystem::PathAppend(FsUtils::AppDataDirectoryPath(),U"audio-timing-failure.txt")}.writeln(error); }
        Vertical::OptionsPresentation::Reset();ksmaudio::Terminate(); return;
    }
    if (System::GetCommandLineArgs().contains(U"--audio-calibration-smoke-test")) {
        AudioCalibration::RunFixtureTests(); Vertical::OptionsPresentation::Reset();ksmaudio::Terminate(); return;
    }
    const auto& audioStatus = ksmaudio::GetAudioOutputStatus();
    Logger << U"Audio output: " << (audioStatus.backend == ksmaudio::AudioBackend::ASIO ? U"ASIO" : U"BASS default/DirectSound")
        << U", driver=" << Unicode::FromUTF8(audioStatus.driver)
        << U", sampleRate=" << audioStatus.sampleRate << U", latencyMs=" << audioStatus.latencySec * 1000.0;
    if (!audioStatus.warning.empty())
        MessageBoxUtils::ShowOK(U"ASIO: " + Unicode::FromUTF8(audioStatus.warning) + U"\nUsing the default audio output. Check Sound settings to select another output.", MessageBoxStyle::Warning);

	// マスターボリュームを適用
	{
		constexpr int32 kMasterVolumeDefault = 100;
		const int32 masterVolume = ConfigIni::GetInt(ConfigIni::Key::kMasterVolume, kMasterVolumeDefault);
		ksmaudio::SetMasterVolume(previewMode ? 0.0 : masterVolume / 100.0);
	}

	// ハイスコアのバックアップを作成
	CreateHighScoreBackup();

	// 画面サイズ反映
	Window::SetToggleFullscreenEnabled(false); // Alt+Enter無効化
	ApplyScreenSizeConfig();

	// Validate the ordinary Play startup path, without preview-mode overrides.
	if (System::GetCommandLineArgs().contains(U"--play-launch-config-check"))
	{
		TextWriter report{U"play-launch-config-check.csv"};
		report.writeln(U"check,actual,expected,status");
		bool passed = true;
		const auto record = [&](StringView name, const String& actual, const String& expected, bool ok) {
			passed = passed && ok;
			report.writeln(U"{},{},{},{}"_fmt(name, actual, expected, ok ? U"PASS" : U"FAIL"));
		};
		record(U"landscape_restored", U"{}x{}"_fmt(Scene::Width(), Scene::Height()), U"800x600",
			!Vertical::ModConfig::Load().enabled && Scene::Size() == Size{800,600});
		const auto keys = String{ConfigIni::GetString(U"key")}.split(U',');
		record(U"default_key_mapping", Format(keys.size()), U"14", keys.size() == 14);
#ifdef _WIN32
		const double volume = BASS_GetVolume();
		const double expected = ConfigIni::GetInt(ConfigIni::Key::kMasterVolume, 100) / 100.0;
		record(U"nonzero_master_volume", Format(volume), Format(expected), expected > 0 && Abs(volume - expected) < .001);
#endif
		report.writeln(U"overall,,,{}"_fmt(passed ? U"PASS" : U"FAIL"));
		Vertical::OptionsPresentation::Reset();ksmaudio::Terminate();
		return;
	}
	// アセット一覧を登録
	SkinPreset::Initialize();
	AssetManagement::RegisterAssets();
    if (RenderGeometryFixture::Requested()) {RenderGeometryFixture::Run();Vertical::OptionsPresentation::Reset();ksmaudio::Terminate();return;}
    if (SkinFixture::Enabled() && System::GetCommandLineArgs().contains(U"--skin-core-test")) { SkinFixture::Core(); Vertical::OptionsPresentation::Reset();ksmaudio::Terminate(); return; }

	// Vsync設定を反映("0;120"または"1"の形式)
	const Array<String> vsyncParts = String{ ConfigIni::GetString(ConfigIni::Key::kVsync, U"0;300") }.split(U';');
	const bool vsyncEnabled = vsyncParts[0] == U"1";
	Graphics::SetVSyncEnabled(vsyncEnabled);

	// フレームレート制限(Vsync有効時は無効化)
	const int32 fpsLimitValue = vsyncParts.size() >= 2 ? ParseOr<int32>(vsyncParts[1], 300) : 300;
#ifdef __EMSCRIPTEN__
	// Web版ではブラウザ側で描画タイミングが制御されるためフレームレート制限は使用しない
	(void)fpsLimitValue;
	const Optional<int32> frameRateLimit = none;
#else
	const Optional<int32> frameRateLimit = vsyncEnabled ? none : Optional<int32>(fpsLimitValue);
#endif
	Addon::Register(FrameRateLimit::kAddonName, std::make_unique<FrameRateLimit>(frameRateLimit), -100);

	Addon::Register(AutoMuteAddon::kAddonName, std::make_unique<AutoMuteAddon>(), 1);

	Addon::Register(CommonSEAddon::kAddonName, std::make_unique<CommonSEAddon>(), 2);

#if defined(_WIN32) || defined(__APPLE__)
	Addon::Register(DisableIMEAddon::kAddonName, std::make_unique<DisableIMEAddon>(), 3);
#endif

	// 毎フレーム連続してアセット生成した時の警告を無効化
	// (楽曲選択でのスクロールにおいては、正常系でもテクスチャ読み込みが毎フレーム発生するため)
	Profiler::EnableAssetCreationWarning(false);

#if defined(_WIN32) || defined(__APPLE__)
	// 起動時はIME無効化を一度実行
	// (これは起動時のものなので、IME無効化設定とは関係なく必ず実行)
	IMEUtils::DetachIMEContext();
#endif

#ifdef __APPLE__
	// macOS: 英数・かなキーのイベントタップを開始してIME切り替えを防ぐ
	KSMPlatformMacOS_StartBlockingIMEKeys();
#endif

#ifdef _DEBUG
	// ライブラリ側のデバッグ用にコンソール表示(Debugビルドの場合のみ)
	Console.open();
#endif

	// ライブラリ初期化
	Co::Init();
	noco::Init();

	// レーザー入力方式がキーボード以外なら、ksmaxisを初期化
	InputUtils::InitKsmaxisForCurrentLaserInput();

	// NocoUIのグローバルデフォルトフォントを設定
	noco::SetGlobalDefaultFont(AssetManagement::SystemFont());
    if(System::GetCommandLineArgs().contains(U"--judge-setting-test")){
        try{JudgeSettingFixture::Run();}catch(const Error& error){TextWriter{FsUtils::GetResourcePath(U"judge-setting-failure.txt")}.writeln(error);}
        Vertical::OptionsPresentation::Reset();ksmaudio::Terminate();return;
    }
    if(System::GetCommandLineArgs().contains(U"--aps-video-fixture")){
        try{ApsVideoFixture::Run();}catch(const Error& error){TextWriter{FsUtils::GetResourcePath(U"aps-video-failure.txt")}.writeln(error);}
        Vertical::OptionsPresentation::Reset();ksmaudio::Terminate();return;
    }
    if(System::GetCommandLineArgs().contains(U"--aps-test")){
        try{ApsFixture::Run();}catch(const Error& error){TextWriter{FsUtils::GetResourcePath(U"aps-failure.txt")}.writeln(error);}
        Vertical::OptionsPresentation::Reset();ksmaudio::Terminate();return;
    }
    if(System::GetCommandLineArgs().contains(U"--audio-end-game-test")) {
        try { AudioEndGameFixture::Run(); }
        catch(const Error& error){TextWriter{FsUtils::GetResourcePath(U"audio-end-game-failure.txt")}.writeln(error);}
        Vertical::OptionsPresentation::Reset();ksmaudio::Terminate();return;
    }


	// NocoUIの音声再生をゲーム本体と同じ音声バックエンド(BASS)で行う
	noco::UISound::SetPlayAudioCallback(
		[](const String& audioFilePath, [[maybe_unused]] const String& audioAssetName, double volume)
		{
			if (audioFilePath.isEmpty())
			{
				return;
			}
			CommonSEAddon::PlayFilePath(audioFilePath, volume);
		});

#ifdef OUTPUT_LICENSE_TXT
	OutputLicenseTxt();
#endif

	// コマンドライン引数をパース
	bool shouldExit = false;
	const auto testPlayArgs = ParseTestPlayArgs(&shouldExit);
	if (shouldExit)
	{
		return;
	}

	// テストプレイの場合、譜面ファイルの読み込みを事前検証
	if (testPlayArgs.has_value())
	{
		const auto chartData = FsUtils::HasKsonExtension(testPlayArgs->chartFilePath)
			? kson::LoadKsonChartData(testPlayArgs->chartFilePath.toUTF8())
			: kson::LoadKshChartData(testPlayArgs->chartFilePath.toUTF8());
		if (chartData.error != kson::ErrorType::None)
		{
			MessageBoxUtils::ShowOK(I18n::Get(I18n::Play::ErrorChartLoadFailed), MessageBoxStyle::Error);
			return;
		}
	}

    const bool audioOptionsSmokeTest = System::GetCommandLineArgs().contains(U"--audio-options-smoke-test");
    if (audioOptionsSmokeTest)
    {
        ksmaudio::SetMasterVolume(0.0);
        ScreenCapture::SetScreenshotDirectory(FsUtils::AppDataDirectoryPath());
    }

    const bool resultDetailCapture=System::GetCommandLineArgs().contains(U"--result-detail-capture");
    if(resultDetailCapture) {
        if(!FileSystem::Exists(FsUtils::GetResourcePath(U"audio-test-fixture"))) throw Error{U"Isolated result fixture required"};
        ksmaudio::SetMasterVolume(0);
        ScreenCapture::SetScreenshotDirectory(FsUtils::GetResourcePath(U"./"));
    }
    const bool timingVisualCapture=TimingHudCapture::Enabled();
    if(timingVisualCapture) {
        ksmaudio::SetMasterVolume(0);
        FileSystem::CreateDirectories(FsUtils::GetResourcePath(U"timing-captures"));
        ScreenCapture::SetScreenshotDirectory(FsUtils::GetResourcePath(U"timing-captures/"));
    }
    int32 timingCapturedStage=-1;
	// 開始シーンを決定
	Co::SceneFactory initialSceneFactory;
    if(resultDetailCapture) {
        ResultSceneArgs args;
        args.chartFilePath=FsUtils::GetResourcePath(U"songs/K-Shoot MANIA/flavorofyouth/flavorofyouth_ch.ksh");
        args.chartData=kson::LoadKshChartData(args.chartFilePath.toUTF8());
        args.playResult.playOption.testPlayOption.emplace(); // Never write a high score.
        args.playResult.playOption.timingHud=RuntimeConfig::LoadTimingHudSettingsFromConfigIni();
        args.playResult.score=9876543;args.playResult.maxCombo=1234;
        args.playResult.detailedStats={12,56,90,1234,67,78,34};
        args.playResult.comboStats={1391,56,78,46};
        args.playResult.totalCombo=1581; // Includes ten unjudged notes: SLOW ERROR gets 44.
        initialSceneFactory=Co::MakeSceneFactory<ResultScene>(args);
    }
    else if (timingVisualCapture && TimingHudCapture::IsMenu()) {
        initialSceneFactory=Co::MakeSceneFactory<SelectScene>(Optional<SelectSceneSearchParams>{none});
    }
    else if (audioOptionsSmokeTest)
    {
        initialSceneFactory = Co::MakeSceneFactory<OptionScene>();
    }
	else if (testPlayArgs.has_value() && testPlayArgs->testPlayOption.hasStartMeasure())
	{
		// テストプレイ(-from指定あり)の場合
		initialSceneFactory = Co::MakeSceneFactory<PlayScene>(
			testPlayArgs->chartFilePath,
			testPlayArgs->isAutoPlay,
			Optional<CoursePlayState>{ none },
			MakeOptional(testPlayArgs->testPlayOption));
	}
	else if (testPlayArgs.has_value())
	{
		// テストプレイ(-fromなし)の場合
		initialSceneFactory = Co::MakeSceneFactory<PlayPrepareScene>(
			testPlayArgs->chartFilePath,
			testPlayArgs->isAutoPlay,
			Optional<CoursePlayState>{ none },
			MakeOptional(testPlayArgs->testPlayOption));
	}
	else
	{
		// 通常起動
		initialSceneFactory = Co::MakeSceneFactory<TitleScene>(TitleMenuItem::kStart);
	}

	// メインループ
	if (previewOptions) initialSceneFactory = Co::MakeSceneFactory<ModOptionsScene>(
        System::GetCommandLineArgs().contains(U"--vertical-preview-depth") ? 3 :
        System::GetCommandLineArgs().contains(U"--vertical-preview-graphics") ? 1 :
        System::GetCommandLineArgs().contains(U"--vertical-preview-developer") ? 2 : 0,0);
	if (previewHUD) initialSceneFactory = Co::MakeSceneFactory<VerticalRenderPreview>();
	if (previewLegacyOptions || previewSettings) initialSceneFactory = Co::MakeSceneFactory<OptionScene>();
    if(previewPrepare){
        FilePath chart=U"../../test-output/ui-v12/fixture/test.ksh";
        const auto args=System::GetCommandLineArgs();
        for(size_t i=0;i+1<args.size();++i)if(args[i]==U"--vertical-preview-chart")chart=args[i+1];
        initialSceneFactory=Co::MakeSceneFactory<PlayPrepareScene>(chart,MusicGame::IsAutoPlayYN::No);
    }
	if (previewSelect) initialSceneFactory = Co::MakeSceneFactory<SelectScene>();
    if (previewResult)
    {
        ResultSceneArgs fixture;
        fixture.chartData = kson::CreateEditorDefaultChartData();
        fixture.chartData.meta.title = "Vertical result preview";
        fixture.chartData.meta.artist = "Render-only fixture";
        const auto args = System::GetCommandLineArgs();
        for (size_t i = 0; i + 1 < args.size(); ++i)
            if (args[i] == U"--vertical-preview-chart")
            {
                fixture.chartFilePath = args[i + 1];
                fixture.chartData = FsUtils::HasKsonExtension(fixture.chartFilePath)
                    ? kson::LoadKsonChartData(fixture.chartFilePath.toUTF8())
                    : kson::LoadKshChartData(fixture.chartFilePath.toUTF8());
            }
        const bool cleared = args.contains(U"--vertical-preview-result-clear");
        fixture.playResult.playOption.diagnosticSession = true;
        fixture.playResult.playOption.timingHud=RuntimeConfig::LoadTimingHudSettingsFromConfigIni();
        fixture.playResult.score = cleared ? 9850000 : 60150;
        fixture.playResult.totalCombo = 1330;
        fixture.playResult.comboStats.critical = cleared ? 1300 : 8;
        fixture.playResult.maxCombo = cleared ? 1000 : 2;
        fixture.playResult.gaugePercentage = cleared ? 100.0 : 0.0;
        fixture.playResult.gaugePercentageForGrade = fixture.playResult.gaugePercentage;
        initialSceneFactory = Co::MakeSceneFactory<ResultScene>(fixture);
    }
	const auto sceneRunner = Co::PlaySceneFrom(std::move(initialSceneFactory)).runScoped();
	// Scene rendering runs in the CoTaskLib addon, after main-loop drawing.
	// Keep the display confirmation above scenes and transition fades.
	const Co::ScopedDrawer displayConfirmationDrawer{
		[] { Vertical::DrawDisplayConfirmation(); }, Co::Layer::User_PostTransition_10};
	Stopwatch previewTimer{StartImmediately::Yes};
	bool previewCaptured = false;
	bool previewMenuOpened = false;
    bool previewSelectMoved = false;
    int previewPrepareFrame=0;
    const bool fastPreview=System::GetCommandLineArgs().contains(U"--vertical-preview-fast");
    const bool warpBench=previewHUD && System::GetCommandLineArgs().contains(U"--vertical-preview-benchmark");
    const bool warpSequence=previewHUD && System::GetCommandLineArgs().contains(U"--vertical-preview-sequence");
    std::array<double,960> warpFrameTimes{};
    int warpFrame=0,warpSequenceFrame=0;
    double warpPreviousTime=previewTimer.sF();
	std::unique_ptr<Vertical::DisplayTransitionTest> displayTestRunner;
	if (displayTest) displayTestRunner = std::make_unique<Vertical::DisplayTransitionTest>();
	
    Stopwatch audioSmokeClock{StartImmediately::Yes};
    bool audioSmokeCaptured = false;
    int32 audioSmokeFrames = 0;
    Stopwatch audioDiagnosticClock{StartImmediately::Yes};
    std::uint64_t lastLateAudioEvents=0, lastRejectedAudioEvents=0;
	while (true)
	{
        ksmaudio::qa::FrameBegin();
        if(!System::Update()) break;
        ksmaudio::qa::Phase("main.update");
        if(ksmaudio::qa::writerFailed) throw Error{U"QA logging failed. Check free disk space and folder permissions."};
        if(resultDetailCapture) {
            if(!audioSmokeCaptured && audioSmokeClock.sF()>2.0) {
                ResultBestScorePanel::CaptureLayout();ExScorePanel::CaptureLayout(U"result");ScreenCapture::SaveCurrentFrame(U"result-detail.png");audioSmokeCaptured=true;
            }
            if(audioSmokeClock.sF()>2.5) break;
        }
        if (timingVisualCapture && TimingHudCapture::Ready()) {
            const auto stage=TimingHudCapture::Stage();
            if (stage!=timingCapturedStage && TimingHudCapture::Fraction()>0.9) {
                ExScorePanel::CaptureLayout(U"menu");ScreenCapture::SaveCurrentFrame(U"{}-{}.png"_fmt(TimingHudCapture::IsMenu()?U"menu":U"hud",stage));
                timingCapturedStage=stage;
            }
            if (TimingHudCapture::Done()) break;
        }
        if (audioOptionsSmokeTest)
        {
            ++audioSmokeFrames;
            if (!audioSmokeCaptured && audioSmokeClock.sF() >= 2.0)
            {
                ScreenCapture::SaveCurrentFrame(U"audio-options-smoke.png");
                audioSmokeCaptured = true;
            }
            const bool pairTest = System::GetCommandLineArgs().any([](const String& arg){ return arg.starts_with(U"--audio-switch-pair="); });
            if (audioSmokeClock.sF() >= 3.0 && (!pairTest || FileSystem::Exists(FsUtils::GetResourcePath(U"audio-switch-smoke-status.txt")))) break;
        }
        if (audioDiagnosticClock.sF() >= 5.0) {
            const auto audioMetrics = ksmaudio::GetAudioLatencySnapshot();
            if (audioMetrics.lateEvents != lastLateAudioEvents || audioMetrics.rejectedEvents != lastRejectedAudioEvents) {
                Logger << U"[audio] scheduled events: late={}, rejected={}, sampleRate={}, frames={}"_fmt(
                    audioMetrics.lateEvents, audioMetrics.rejectedEvents, audioMetrics.output.sampleRate, audioMetrics.output.bufferSamples);
                lastLateAudioEvents=audioMetrics.lateEvents; lastRejectedAudioEvents=audioMetrics.rejectedEvents;
            }
            audioDiagnosticClock.restart();
        }
        const auto audioFailure = ksmaudio::GetAudioOutputFailure();
        if (!audioFailure.empty())
        {
            AudioCalibration::Store();
        ConfigIni::Save();
            ksmaudio::SetMute(true);
            // Abort before another judgment update. Do not save a disconnected-device score.
            throw Error{Unicode::FromUTF8(audioFailure)};
        }
		// マウスカーソル非表示設定(全画面での非表示)
		// プレイ中のみの非表示はPlaySceneで処理する
		if (ConfigIni::GetInt(ConfigIni::Key::kHideMouseCursor, ConfigIni::Value::HideMouseCursor::kPlayOnly) == ConfigIni::Value::HideMouseCursor::kAll)
		{
			Cursor::RequestStyle(CursorStyle::Hidden);
		}

		if (ksmaxis::IsInitialized())
		{
            ksmaudio::qa::Scope qaPhase{"input.ksmaxis"};
			ksmaxis::Update();
		}

		// KeyConfigの入力状態を更新
        ksmaudio::qa::Phase("input.KeyConfig");
		KeyConfig::Update();
        if (previewSelect && !previewSelectMoved && previewTimer.sF() >= 1.95 &&
            (System::GetCommandLineArgs().contains(U"--vertical-preview-select-down") ||
             System::GetCommandLineArgs().contains(U"--vertical-preview-select-up")))
        {
            KeyConfig::RequestVirtualDown(System::GetCommandLineArgs().contains(U"--vertical-preview-select-up")
                ? kButtonUp : kButtonDown);
            previewSelectMoved = true;
        }
		if (previewSettings && !previewMenuOpened && previewTimer.sF() >= 0.8)
		{
			KeyConfig::RequestVirtualDown(kButtonStart);
			previewMenuOpened = true;
		}

		// ウィンドウのフォーカス状態を更新
        ksmaudio::qa::Phase("input.focus");
		InputUtils::UpdateWindowFocusState();
        Vertical::OptionProbe::update();
		Vertical::UpdateDisplayConfirmation();
        if(Vertical::OptionProbe::active() && Vertical::OptionProbe::done()) break;
		if (displayTestRunner && displayTestRunner->update()) break;
		if(warpBench) {
            const double now=previewTimer.sF();
            if(warpFrame>=240 && warpFrame<1200) warpFrameTimes[warpFrame-240]=(now-warpPreviousTime)*1000;
            warpPreviousTime=now;
            if(++warpFrame>=1200) break;
        }
        if(warpSequence && previewTimer.sF() >= .1*warpSequenceFrame && warpSequenceFrame<30) {
            ScreenCapture::SaveCurrentFrame(U"warp-sequence-{:02}.png"_fmt(warpSequenceFrame++));
        }
        if(previewPrepare && previewPrepareFrame<3 && previewTimer.sF()>=std::array<double,3>{.25,1.25,2.5}[previewPrepareFrame])
            ScreenCapture::SaveCurrentFrame(U"vertical-prepare-stage-{}.png"_fmt(previewPrepareFrame++));
        if (previewMode && !warpBench && !displayTest && !previewCaptured && previewTimer.sF() >= (fastPreview ? .8 : 2.0))
		{
			ScreenCapture::SaveCurrentFrame(previewResult ? U"vertical-result-preview.png" : previewPrepare ? U"vertical-prepare-preview.png" : previewTitle ? U"vertical-title-preview.png" :
				(previewOptions ? U"vertical-options-preview.png" :
				(previewLegacyOptions ? U"vertical-option-scene-preview.png" :
				(previewSettings ? U"vertical-settings-preview.png" :
				(previewSelect ? U"vertical-select-preview.png" : U"vertical-hud-preview.png")))));
			previewCaptured = true;
		}
		if (previewMode && !warpBench && !displayTest && !Vertical::OptionProbe::active() && previewTimer.sF() >= (fastPreview ? 1.1 : 3.2)) break;

#ifdef __APPLE__
		// テキスト編集中かどうかを設定(テキスト編集中はIMEキーをブロックしない)
		KSMPlatformMacOS_SetIsEditingText(noco::IsEditingTextBox());
#endif

        ksmaudio::qa::Phase("sceneRunner.done");
		if (sceneRunner.done())
		{
			break;
		}
	}

    if(warpBench) {
        double sum=0; int over=0;
        for(const double ms:warpFrameTimes) {sum+=ms; if(ms>1000.0/120) ++over;}
        std::sort(warpFrameTimes.begin(),warpFrameTimes.end());
        TextWriter writer{U"vertical-warp-benchmark.csv"};
        writer.writeln(U"samples,mean_ms,p99_ms,over_8_333ms,width,height");
        writer.writeln(U"960,{},{},{},{},{}"_fmt(sum/960,warpFrameTimes[950],over,Scene::Width(),Scene::Height()));
    }
	Vertical::RestorePendingDisplayChange();
	if (previewMode) originalVerticalConfig.Save();
    if (audioOptionsSmokeTest)
    {
        TextWriter smokeReport{FsUtils::GetResourcePath(U"audio-smoke-status.txt")};
        smokeReport.writeln(U"frames={}, captureRequested={}, backend={}, driver={}"_fmt(audioSmokeFrames, audioSmokeCaptured,
            ksmaudio::IsASIOActive() ? U"ASIO" : U"BASS", Unicode::FromUTF8(ksmaudio::GetAudioOutputStatus().driver)));
    }
	// config.iniを保存
    ksmaudio::qa::Scene("shutdown");
    ksmaudio::qa::Event("MAIN_LOOP_EXIT");
    ksmaudio::qa::Scope qaShutdown{"main.shutdown",true};
    // audio_backend is the NEXT launch selection, not necessarily this process's
    // active output. Never overwrite it with the still-running output at shutdown.
	if (!previewMode) { AudioCalibration::Store(); ConfigIni::Save(); }

#ifdef __APPLE__
	// macOS: 英数・かなキーのイベントタップを停止
	KSMPlatformMacOS_StopBlockingIMEKeys();
#endif

	// ライブラリ終了
	if (ksmaxis::IsInitialized())
	{
		ksmaxis::Terminate();
	}
	AudioOutputSettings::Log(U"BEGIN normal audio shutdown");
	Vertical::OptionsPresentation::Reset();ksmaudio::Terminate();
	AudioOutputSettings::Log(U"END normal audio shutdown");
}

void Main()
{
    ksmaudio::qa::SessionGuard qaSession;
    if(System::GetCommandLineArgs().contains(U"--vertical-spin-test")){Vertical::RunSpinRegression();return;}
	if (System::GetCommandLineArgs().contains(U"--vertical-menu-input-test"))
	{
		Vertical::RunMenuInputTest();
		return;
	}
	if (System::GetCommandLineArgs().contains(U"--vertical-replay-test"))
	{
		Vertical::RunJudgmentReplayTest();
		return;
	}
	if (System::GetCommandLineArgs().contains(U"--vertical-self-test"))
	{
		Vertical::RunSelfTest();
		return;
	}
	try
	{
		KSMMain();
	}
	catch (const Error& e)
	{
        ksmaudio::qa::Event("EXCEPTION",0,0,e.what().toUTF8());
        ksmaudio::qa::Phase("exception.audio_cleanup");
        Vertical::OptionsPresentation::Reset();ksmaudio::Terminate();
		if(RenderGeometryFixture::Requested()){TextWriter{FsUtils::GetResourcePath(U"render-geometry-failure.txt")}.writeln(e.what());return;}
		if (SkinFixture::Requested()) { TextWriter{FsUtils::GetResourcePath(U"skin-test-failure.txt")}.writeln(e.what()); return; }
		MessageBoxUtils::ShowOK(e.what(), MessageBoxStyle::Error);
		throw;
	}
	catch (const std::exception& e)
	{
        ksmaudio::qa::Event("EXCEPTION",0,0,e.what());
        ksmaudio::qa::Phase("exception.audio_cleanup");
        Vertical::OptionsPresentation::Reset();ksmaudio::Terminate();
		MessageBoxUtils::ShowOK(Unicode::FromUTF8(e.what()), MessageBoxStyle::Error);
		throw;
	}
	catch (...)
	{
        ksmaudio::qa::Event("EXCEPTION_UNKNOWN");
        ksmaudio::qa::Phase("exception.audio_cleanup");
        Vertical::OptionsPresentation::Reset();ksmaudio::Terminate();
		MessageBoxUtils::ShowOK(U"Unknown exception", MessageBoxStyle::Error);
		throw;
	}
}
