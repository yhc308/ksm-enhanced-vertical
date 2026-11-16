#include "Diagnostics/DiagnosticArguments.hpp"
#include "ksmaudio/Diagnostics.hpp"
#include "OptionScene.hpp"
#include "Common/FrameRateLimit.hpp"
#include "Common/IMEUtils.hpp"
#include "Addon/DisableIMEAddon.hpp"
#include "RuntimeConfig.hpp"
#include "Scenes/Title/TitleScene.hpp"
#include "Input/InputUtils.hpp"
#include "Vertical/VerticalDisplay.hpp"
#include "Vertical/VerticalResources.hpp"
#include "Vertical/ModOptionsScene.hpp"
#include "Ini/AudioCalibration.hpp"
#include "Ini/KSMIniData.hpp"
#include "Common/AudioOutputSettings.hpp"
#include <fstream>
#include <filesystem>

// TODO: TextureIdxまわりどうにかする

namespace
{
	constexpr Duration kFadeDuration = 0.6s;

	constexpr double kSettingMenuScrollSmoothTimeSec = 0.08;

	FilePath GetOptionSceneUIFilePath()
	{
		return Vertical::UIPath(U"ui/scene/option.noco");
	}

	std::shared_ptr<noco::Canvas> LoadOptionSceneCanvas()
	{
		const FilePath uiFilePath = GetOptionSceneUIFilePath();
		const auto canvas = noco::Canvas::LoadFromFile(uiFilePath);
		if (!canvas)
		{
			throw Error{ U"Failed to load '{}'"_fmt(uiFilePath) };
		}
		return canvas;
	}

	constexpr int32 kMasterVolumeMin = 0;
	constexpr int32 kMasterVolumeMax = 100;
	constexpr int32 kMasterVolumeDefault = 100;
	constexpr int32 kTimingAdjustMin = -10000;
	constexpr int32 kTimingAdjustMax = 10000;
	constexpr int32 kTimingAdjustDefault = 0;
	constexpr int32 kLaserInputSensitivityMin = -10000;
	constexpr int32 kLaserInputSensitivityMax = 10000;
	constexpr int32 kLaserInputSensitivityDefault = 50;
}

std::unique_ptr<std::array<OptionMenu, OptionScene::kOptionMenuTypeEnumCount>> OptionScene::makeOptionMenus()
{
	using StrPair = std::pair<String, String>;
	using IntStrPair = std::pair<int32, String>;
	using Category = OptionMenu::Category;
	using CreateInfo = OptionMenuField::CreateInfo;

	Array<StrPair> availableLanguageStrPairs;
	for (const auto& language : I18n::GetAvailableLanguageList())
	{
		availableLanguageStrPairs.emplace_back(language, language);
	}

	const bool verticalEnabled = Vertical::ModConfig::Load().enabled;
	Array<StrPair> verticalResolutions;
	for (const auto resolution : Vertical::kPresets)
		verticalResolutions.emplace_back(U"{},{}"_fmt(resolution.width, resolution.height), U"{}x{}"_fmt(resolution.width, resolution.height));
	Array<StrPair> audioOutputs{
		StrPair{ String{ ConfigIni::Value::AudioBackend::kDefault }, I18n::Get(I18n::Option::AudioBackendDefault) },
		StrPair{ String{ ConfigIni::Value::AudioBackend::kDirectSound }, I18n::Get(I18n::Option::AudioBackendDirectSound) },
		StrPair{ U"asio", U"ASIO" },
	};
#ifdef _WIN32
	for (const auto& driver : ksmaudio::GetASIODrivers())
	{
		const auto name = Unicode::FromUTF8(driver);
		audioOutputs.emplace_back(U"asio:" + name, U"ASIO: " + name);
	}
	const String savedOutput{ ConfigIni::GetString(ConfigIni::Key::kAudioBackend) };
	if (savedOutput.starts_with(U"asio:") && !audioOutputs.any([&](const StrPair& item)
															   {
																   return item.first == savedOutput;
															   }))
		audioOutputs.emplace_back(savedOutput, savedOutput + U" (unavailable)");
#endif

	return std::make_unique<std::array<OptionMenu, kOptionMenuTypeEnumCount>>(std::array<OptionMenu, kOptionMenuTypeEnumCount>{
		OptionMenu({
			Category{
				.heading = String{ I18n::Get(I18n::Option::HeadingScreen) },
				.fields = {
					verticalEnabled ? CreateInfo::Enum(U"VERTICAL DISPLAY", U"vertical_display_mode", Array<IntStrPair>{ { 0, U"WINDOWED" }, { 2, U"BORDERLESS FULLSCREEN" } }) : CreateInfo::Enum(I18n::Get(I18n::Option::ItemScreenMode), ConfigIni::Key::kFullScreen, Array<StringView>{
																																																																	 I18n::Get(I18n::Option::FullscreenOff),
																																																																	 I18n::Get(I18n::Option::FullscreenOn),
																																																																 }),
					verticalEnabled ? CreateInfo::Enum(U"VERTICAL RENDER PIXELS", U"vertical_resolution", verticalResolutions).setDefaultValue(1) : CreateInfo::Enum(I18n::Get(I18n::Option::ItemScreenResolutionWindow), ConfigIni::Key::kWindowResolution, Array<StrPair>{
																																																														 StrPair{ U"320,240", U"320x240" },
																																																														 StrPair{ U"480,320", U"480x320" },
																																																														 StrPair{ U"480,360", U"480x360" },
																																																														 StrPair{ U"512,384", U"512x384" },
																																																														 StrPair{ U"580,435", U"580x435" },
																																																														 StrPair{ U"640,480", U"640x480" },
																																																														 StrPair{ U"800,600", U"800x600" },
																																																														 StrPair{ U"1024,768", U"1024x768" },
																																																														 StrPair{ U"1280,720", U"1280x720" },
																																																														 StrPair{ U"1280,1024", U"1280x1024" },
																																																														 StrPair{ U"1366,768", U"1366x768" },
																																																														 StrPair{ U"1920,1080", U"1920x1080" },
																																																														 StrPair{ U"1920,1200", U"1920x1200" },
																																																													 }),
					verticalEnabled ? CreateInfo::Enum(U"BORDERLESS OUTPUT", U"vertical_borderless_info", Array<StringView>{ U"ASPECT FIT / BLACK LETTERBOX" }) : CreateInfo::Enum(I18n::Get(I18n::Option::ItemScreenResolutionFullscreen), ConfigIni::Key::kFullScreenResolution, Array<StrPair>{
																																																																			   StrPair{ U"640,480", U"640x480" },
																																																																			   StrPair{ U"800,600", U"800x600" },
																																																																			   StrPair{ U"1024,600", U"1024x600" },
																																																																			   StrPair{ U"1024,768", U"1024x768" },
																																																																			   StrPair{ U"1280,720", U"1280x720" },
																																																																			   StrPair{ U"1280,800", U"1280x800" },
																																																																			   StrPair{ U"1280,1024", U"1280x1024" },
																																																																			   StrPair{ U"1366,768", U"1366x768" },
																																																																			   StrPair{ U"1920,1080", U"1920x1080" },
																																																																			   StrPair{ U"1920,1200", U"1920x1200" },
																																																																		   }),
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemVsync), ConfigIni::Key::kVsync, Array<StrPair>{
						StrPair{ U"0;120", I18n::Get(I18n::Option::VsyncOffWithFps, 120) },
						StrPair{ U"0;144", I18n::Get(I18n::Option::VsyncOffWithFps, 144) },
						StrPair{ U"0;300", I18n::Get(I18n::Option::VsyncOffWithFps, 300) },
						StrPair{ U"1", I18n::Get(I18n::Option::VsyncOn) },
					}),
				},
			},
			Category{
				.heading = String{ I18n::Get(I18n::Option::HeadingDisplay) },
				.fields = {
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemDisplayLanguage), ConfigIni::Key::kLanguage, availableLanguageStrPairs)
						.setOnChangeCallback([this]
						{
							I18n::LoadLanguage(ConfigIni::GetString(ConfigIni::Key::kLanguage));
							m_languageChangeRequested = true;
						}),
					/*CreateInfo::Enum(ConfigIni::Key::kTextureSize, Array<IntStrPair>{
						IntStrPair{ ConfigIni::Value::TextureSize::kSmall, I18n::Get(I18n::Option::TextureSizeSmall) },
						IntStrPair{ ConfigIni::Value::TextureSize::kMedium, I18n::Get(I18n::Option::TextureSizeMedium) },
						IntStrPair{ ConfigIni::Value::TextureSize::kLarge, I18n::Get(I18n::Option::TextureSizeLarge) },
					}),*/
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemGameplayBG), ConfigIni::Key::kBGDisplayMode, Array<IntStrPair>{
						IntStrPair{ ConfigIni::Value::BGDisplayMode::kHide, I18n::Get(I18n::Option::BGDisplayModeHide) },
						IntStrPair{ ConfigIni::Value::BGDisplayMode::kShowNoLayer, I18n::Get(I18n::Option::BGDisplayModeShowNoLayer) },
						IntStrPair{ ConfigIni::Value::BGDisplayMode::kShowLayer, I18n::Get(I18n::Option::BGDisplayModeShowLayer) },
					}),
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemAlwaysShowOtherFolders), ConfigIni::Key::kAlwaysShowOtherFolders, Array<StringView>{
						I18n::Get(I18n::Option::AlwaysShowOtherFoldersOff),
						I18n::Get(I18n::Option::AlwaysShowOtherFoldersOn),
					}),
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemHideAllFolder), ConfigIni::Key::kHideAllFolder, Array<StringView>{
						I18n::Get(I18n::Option::HideAllFolderOff),
						I18n::Get(I18n::Option::HideAllFolderOn),
					}),
				},
			},
			Category{
				.heading = String{ I18n::Get(I18n::Option::HeadingSound) },
				.fields = {
					CreateInfo::Int(I18n::Get(I18n::Option::ItemMasterVolume), ConfigIni::Key::kMasterVolume, kMasterVolumeMin, kMasterVolumeMax, kMasterVolumeDefault, I18n::Get(I18n::Option::MasterVolumePercent), 5)
						.setOnChangeCallback([]
						{
							const int32 volume = ConfigIni::GetInt(ConfigIni::Key::kMasterVolume, kMasterVolumeDefault);
							ksmaudio::SetMasterVolume(volume / 100.0);
						}),
#ifdef _WIN32
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemAudioBackend), ConfigIni::Key::kAudioBackend, audioOutputs),
			[this] {
				auto field = CreateInfo::Int(I18n::Get(I18n::Mod::AsioJudgmentCorrection), ConfigIni::Key::kAsioJudgmentCorrection, -100, 100, 15, U"ms", 1);
				field.isEnabled = [] { return ksmaudio::IsASIOActive(); };
				field.onChangeCallback = [this] { if (!AudioCalibration::Store()) { m_outputError = I18n::Get(I18n::Mod::AudioSaveFailed); logAudioOutput(m_outputError, true, false); } };
				return field;
			}(),
#endif
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemAssistTick), ConfigIni::Key::kAssistTick, Array<StringView>{
						I18n::Get(I18n::Option::AssistTickOff),
						I18n::Get(I18n::Option::AssistTickAuto),
						I18n::Get(I18n::Option::AssistTickInput),
					}),
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemAutoPlaySE), ConfigIni::Key::kAutoPlaySE, Array<StringView>{
						I18n::Get(I18n::Option::Off),
						I18n::Get(I18n::Option::On),
					}),
				},
			},
		}),
		OptionMenu({
			Category{
				.heading = String{ I18n::Get(I18n::Option::HeadingJudgment) },
				.fields = {
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemDefaultBTJudgmentMode), ConfigIni::Key::kJudgmentModeBT, Array<StringView>{
						I18n::Get(I18n::Option::JudgmentOn),
						I18n::Get(I18n::Option::JudgmentOff),
						I18n::Get(I18n::Option::JudgmentAuto),
						I18n::Get(I18n::Option::JudgmentHide),
					}).setOnChangeCallback([]() {
						RuntimeConfig::SetJudgmentPlayModeBT(static_cast<JudgmentPlayMode>(ConfigIni::GetInt(ConfigIni::Key::kJudgmentModeBT)));
					}),
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemDefaultFXJudgmentMode), ConfigIni::Key::kJudgmentModeFX, Array<StringView>{
						I18n::Get(I18n::Option::JudgmentOn),
						I18n::Get(I18n::Option::JudgmentOff),
						I18n::Get(I18n::Option::JudgmentAuto),
						I18n::Get(I18n::Option::JudgmentHide),
					}).setOnChangeCallback([]() {
						RuntimeConfig::SetJudgmentPlayModeFX(static_cast<JudgmentPlayMode>(ConfigIni::GetInt(ConfigIni::Key::kJudgmentModeFX)));
					}),
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemDefaultLaserJudgmentMode), ConfigIni::Key::kJudgmentModeLaser, Array<StringView>{
						I18n::Get(I18n::Option::JudgmentOn),
						I18n::Get(I18n::Option::JudgmentOff),
						I18n::Get(I18n::Option::JudgmentAuto),
						I18n::Get(I18n::Option::JudgmentHide),
					}).setOnChangeCallback([]() {
						RuntimeConfig::SetJudgmentPlayModeLaser(static_cast<JudgmentPlayMode>(ConfigIni::GetInt(ConfigIni::Key::kJudgmentModeLaser)));
					}),
				},
			},
			Category{
				.heading = String{ I18n::Get(I18n::Option::HeadingTimingAdjustment) },
				.fields = {
					CreateInfo::Int(I18n::Get(I18n::Option::ItemTimingAdjustment), ConfigIni::Key::kInputDelay, kTimingAdjustMin, kTimingAdjustMax, kTimingAdjustDefault, I18n::Get(I18n::Option::TimingAdjustMs))
						.setAdditionalSuffixes(
							I18n::Get(I18n::Option::TimingAdjustSuffixNoAdjustment),
							I18n::Get(I18n::Option::TimingAdjustSuffixLater),
							I18n::Get(I18n::Option::TimingAdjustSuffixEarlier)),
					CreateInfo::Int(I18n::Get(I18n::Option::ItemLaserTimingAdjustment), ConfigIni::Key::kLaserInputDelay, kTimingAdjustMin, kTimingAdjustMax, kTimingAdjustDefault, I18n::Get(I18n::Option::TimingAdjustMs))
						.setAdditionalSuffixes(
							I18n::Get(I18n::Option::LaserTimingAdjustSuffixNoAdjustment),
							I18n::Get(I18n::Option::LaserTimingAdjustSuffixLater),
							I18n::Get(I18n::Option::LaserTimingAdjustSuffixEarlier)),
					CreateInfo::Int(I18n::Get(I18n::Option::ItemVisualOffset), ConfigIni::Key::kVisualOffset, kTimingAdjustMin, kTimingAdjustMax, kTimingAdjustDefault, I18n::Get(I18n::Option::TimingAdjustMs))
						.setAdditionalSuffixes(
							I18n::Get(I18n::Option::VisualOffsetSuffixNoAdjustment),
							I18n::Get(I18n::Option::VisualOffsetSuffixLater),
							I18n::Get(I18n::Option::VisualOffsetSuffixEarlier)),
				},
			},
			Category{
				.heading = String{ I18n::Get(I18n::Option::HeadingLaserInput) },
				.fields = {
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemLaserInputType), ConfigIni::Key::kLaserInputType, Array<IntStrPair>{
						IntStrPair{ ConfigIni::Value::LaserInputType::kKeyboard, I18n::Get(I18n::Option::LaserInputTypeKeyboard) },
						IntStrPair{ ConfigIni::Value::LaserInputType::kSlider, I18n::Get(I18n::Option::LaserInputTypeSlider) },
#if !defined(__APPLE__) // macOSではマウスデバイスの取得に入力監視の許可が必要で、配布アプリケーション用に正式なコード署名が必要になる(要Apple Developer Program加入)とみられるため実装済みだが一旦除外
						IntStrPair{ ConfigIni::Value::LaserInputType::kMouseXY, I18n::Get(I18n::Option::LaserInputTypeMouseXY) },
#endif
						IntStrPair{ ConfigIni::Value::LaserInputType::kAnalogStickXY, I18n::Get(I18n::Option::LaserInputTypeAnalogStickXY) },
					}).setOnChangeCallback([]() {
						InputUtils::InitKsmaxisForCurrentLaserInput();
					}),
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemMouseXInputDirection), ConfigIni::Key::kLaserMouseDirectionX, Array<StringView>{
						I18n::Get(I18n::Option::LaserMouseDirectionLeftThenRight),
						I18n::Get(I18n::Option::LaserMouseDirectionRightThenRight),
					}),
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemMouseYInputDirection), ConfigIni::Key::kLaserMouseDirectionY, Array<StringView>{
						I18n::Get(I18n::Option::LaserMouseDirectionUpThenRight),
						I18n::Get(I18n::Option::LaserMouseDirectionDownThenRight),
					}),
					CreateInfo::Int(I18n::Get(I18n::Option::ItemSliderMouseInputSensitivity), ConfigIni::Key::kLaserInputSensitivity, kLaserInputSensitivityMin, kLaserInputSensitivityMax, kLaserInputSensitivityDefault), // TODO: additional suffix for zero value
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemSwapLRLaser), ConfigIni::Key::kSwapLaserLR, Array<StringView>{
						I18n::Get(I18n::Option::Off),
						I18n::Get(I18n::Option::On),
					}),
				},
			},
			Category{
				.heading = String{ I18n::Get(I18n::Option::HeadingKeyboardButton) },
				.fields = {
#if defined(__linux__)
					// LinuxではIME無効化の実行間隔による違いがないためON/OFFのみ
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemDisableIME), ConfigIni::Key::kDisableIME, Array<IntStrPair>{
						IntStrPair{ static_cast<int32>(DisableIMEMode::kOff), I18n::Get(I18n::Option::Off) },
						IntStrPair{ static_cast<int32>(DisableIMEMode::kHigh), I18n::Get(I18n::Option::On) },
					}).setDefaultValue(1),
#else
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemDisableIME), ConfigIni::Key::kDisableIME, Array<StringView>{
						I18n::Get(I18n::Option::DisableIMEOff),
						I18n::Get(I18n::Option::DisableIMEOnLow),
						I18n::Get(I18n::Option::DisableIMEOnMid),
						I18n::Get(I18n::Option::DisableIMEOnHigh),
					}),
#endif
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemCloseDirectoryKey), ConfigIni::Key::kSelectCloseFolderKey, Array<StringView>{
						I18n::Get(I18n::Option::SelectCloseFolderKeyBackspace),
						I18n::Get(I18n::Option::SelectCloseFolderKeyEsc),
					}),
					CreateInfo::Enum(I18n::Get(I18n::Option::Item3BTKeysStartBack), ConfigIni::Key::kUse3BTsPlusStartAsBack, Array<StringView>{
						I18n::Get(I18n::Option::Off),
						I18n::Get(I18n::Option::On),
					}),
				},
			},
		}),
		OptionMenu({
			Category{
				.heading = String{ I18n::Get(I18n::Option::HeadingHispeedType) },
				.fields = {
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemHispeedTypeXMod), ConfigIni::Key::kHispeedShowXMod, Array<StringView>{
						I18n::Get(I18n::Option::HispeedTypeHide),
						I18n::Get(I18n::Option::HispeedTypeShow),
					}),
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemHispeedTypeOMod), ConfigIni::Key::kHispeedShowOMod, Array<StringView>{
						I18n::Get(I18n::Option::HispeedTypeHide),
						I18n::Get(I18n::Option::HispeedTypeShow),
					}),
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemHispeedTypeCMod), ConfigIni::Key::kHispeedShowCMod, Array<StringView>{
						I18n::Get(I18n::Option::HispeedTypeHide),
						I18n::Get(I18n::Option::HispeedTypeShow),
					}),
				},
			},
			Category{
				.heading = String{ I18n::Get(I18n::Option::HeadingMouseKeyboard) },
				.fields = {
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemHideMouseCursor), ConfigIni::Key::kHideMouseCursor, Array<StringView>{
						I18n::Get(I18n::Option::HideMouseCursorOff),
						I18n::Get(I18n::Option::HideMouseCursorOnPlayOnly),
						I18n::Get(I18n::Option::HideMouseCursorOnAll),
					}).setDefaultValue(ConfigIni::Value::HideMouseCursor::kPlayOnly),
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemEnableMouseUIInput), ConfigIni::Key::kEnableMouseUIInput, Array<StringView>{
						I18n::Get(I18n::Option::Off),
						I18n::Get(I18n::Option::On),
					}).setDefaultValue(1),
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemAssignRightClickToBack), ConfigIni::Key::kAssignRightClickToBack, Array<StringView>{
						I18n::Get(I18n::Option::RightClickToBackOff),
						I18n::Get(I18n::Option::RightClickToBackExceptPlay),
						I18n::Get(I18n::Option::RightClickToBackAll),
					}).setDefaultValue(ConfigIni::Value::RightClickToBack::kExceptPlay),
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemUseNumpadAsArrowKeys), ConfigIni::Key::kUseNumpadAsArrowKeys, Array<StringView>{
						I18n::Get(I18n::Option::UseNumpadAsArrowKeysOff),
						I18n::Get(I18n::Option::UseNumpadAsArrowKeysOnKeyboard),
						I18n::Get(I18n::Option::UseNumpadAsArrowKeysOnController),
					}),
				},
			},
			Category{
				.heading = String{ I18n::Get(I18n::Option::HeadingTranslit) },
				.fields = {
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemShowTranslitKana), ConfigIni::Key::kShowTranslitKana, Array<StringView>{
						I18n::Get(I18n::Option::Off),
						I18n::Get(I18n::Option::On),
					}),
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemShowTranslitHangul), ConfigIni::Key::kShowTranslitHangul, Array<StringView>{
						I18n::Get(I18n::Option::Off),
						I18n::Get(I18n::Option::On),
					}),
					CreateInfo::Enum(I18n::Get(I18n::Option::ItemShowTranslitKanji), ConfigIni::Key::kShowTranslitKanji, Array<StringView>{
						I18n::Get(I18n::Option::Off),
						I18n::Get(I18n::Option::On),
					}),
					
				},
			},
			Category{
				.heading = String{ I18n::Get(I18n::Mod::GameplaySettingsHeading) },
				.fields = {
					CreateInfo::Enum(I18n::Get(I18n::Mod::RememberEffRate), ConfigIni::Key::kRememberEffRate, Array<StringView>{ I18n::Get(I18n::Option::Off), I18n::Get(I18n::Option::On) }).setDefaultValue(0).setOnChangeCallback([]
																																																							 {
																																																								 RuntimeConfig::SetRememberEffRate(ConfigIni::GetBool(ConfigIni::Key::kRememberEffRate, false));
																																																							 }),
				},
			},
		}),
		OptionMenu({}),
	});
}

OptionScene::OptionScene(bool displayShortcut)
	: m_canvas(LoadOptionSceneCanvas()), m_optionMenus(makeOptionMenus()), m_returnToVertical(displayShortcut)
{
	ksmaudio::qa::Scope qaConstruct{ "OptionScene.construct", true };
	const auto& activeOutput = ksmaudio::GetAudioOutputStatus();
	m_activeOutputSelection = activeOutput.backend == ksmaudio::AudioBackend::ASIO ? U"asio:" + Unicode::FromUTF8(activeOutput.driver) : (activeOutput.backend == ksmaudio::AudioBackend::DirectSound ? U"direct_sound" : U"default");
	if (displayShortcut)
	{
		m_currentOptionMenuIdx = kDisplaySound;
		createSettingItemNodes();
	}
	m_bgmStream.play();
	AutoMuteAddon::SetEnabled(true);
	const auto installedDrivers = ksmaudio::GetASIODrivers();
	if (!installedDrivers.empty())
		m_genericASIOSelection = U"asio:" + Unicode::FromUTF8(installedDrivers.front());
	m_savedOutputSelection = String{ ConfigIni::GetString(ConfigIni::Key::kAudioBackend, U"default") };
	m_switchSmokeASIO = m_savedOutputSelection;
	m_switchSmokeTargets = { U"default", U"direct_sound", m_switchSmokeASIO };
	for (const auto& arg : ModDiagnostics::Arguments())
	{
		if (!arg.starts_with(U"--audio-switch-pair="))
			continue;
		if (!FileSystem::Exists(FsUtils::GetResourcePath(U"audio-test-fixture")))
			throw Error{ U"Pair test requires fixture directory" };
		const auto pair = arg.substr(20).split(U'|');
		if (pair.size() != 2)
			throw Error{ U"Pair test expects source|target" };
		m_switchSmokeTargets = pair;
	}
	if (ModDiagnostics::Arguments().contains(U"--audio-options-smoke-test"))
	{
		m_currentOptionMenuIdx = kDisplaySound;
		auto& menu = (*m_optionMenus)[kDisplaySound];
		const auto items = menu.getSettingItemParamsList();
		for (std::size_t i = 0; i < items.size(); ++i)
			if (items[i].name == I18n::Get(I18n::Option::ItemAudioBackend))
				menu.setCursor(static_cast<int32>(i));
		createSettingItemNodes();
	}
}

void OptionScene::update()
{
	if (m_returnToVertical && Vertical::OptionProbe::active() && Vertical::OptionProbe::stage == 3 && !isFadingIn())
	{
		Vertical::OptionProbe::record(U"display_shortcut_opens_existing_section", m_currentOptionMenuIdx == kDisplaySound);
		Vertical::OptionProbe::advance(4);
		exitScene();
		return;
	}
	ksmaudio::qa::Scope qaScope{ "OptionScene.update", false };
	ksmaudio::qa::Scene("option");
	if (ModDiagnostics::Arguments().contains(U"--audio-output-switch-smoke-test") && m_switchSmokeClock.sF() > 0.2 &&
		m_switchSmokeStage <= static_cast<int32>(m_switchSmokeTargets.size()))
	{
		if (!FileSystem::Exists(FileSystem::PathAppend(FsUtils::AppDataDirectoryPath(), U"audio-test-fixture")))
			throw Error{ U"Switch test requires fixture directory" };
		if (m_switchSmokeStage < static_cast<int32>(m_switchSmokeTargets.size()))
		{
			m_currentOptionMenuIdx = kDisplaySound;
			ConfigIni::SetString(ConfigIni::Key::kAudioBackend, m_switchSmokeTargets[m_switchSmokeStage]);
			++m_switchSmokeStage;
			saveOutputSelection();
			m_switchSmokeClock.restart();
			m_switchSmokeHistory += U"requested={} active={} saved={} bgm={} calibration={}\n"_fmt(
				ConfigIni::GetString(ConfigIni::Key::kAudioBackend), m_activeOutputSelection, m_savedOutputSelection,
				m_bgmStream.isPlaying(), AudioCalibration::active);
		}
		else
		{
			TextWriter report{ FileSystem::PathAppend(FsUtils::AppDataDirectoryPath(), U"audio-switch-smoke-status.txt") };
			report.writeln(U"stages={}, error={}, BGMPlaying={}, active={}"_fmt(m_switchSmokeStage, m_outputError, m_bgmStream.isPlaying(), m_activeOutputSelection));
			report.writeln(m_switchSmokeHistory);
			m_currentOptionMenuIdx = kDisplaySound;
			createSettingItemNodes();
			++m_switchSmokeStage;
		}
	}
	// フェード中はクリックやホバーが効かないようヒットテスト無効
	const noco::HitTestEnabledYN hitTestEnabled{ !isFadingIn() && !isFadingOut() && InputUtils::IsUIMouseInputEnabled() };

	// 共通オーバーレイを更新
	m_commonOverlay.update(hitTestEnabled);

	// メニュー更新
	if (m_currentOptionMenuIdx.has_value())
	{
		if (m_currentOptionMenuIdx == OptionMenuType::kKeyConfig)
		{
			m_keyConfigMenu.update(m_canvas.get());
		}
		else
		{
			const OptionMenu::MouseInput mouseInput = settingMenuMouseInput();
			const bool changed = (*m_optionMenus)[*m_currentOptionMenuIdx].update(mouseInput);
			if (changed)
			{
				if (m_languageChangeRequested)
				{
					m_languageChangeRequested = false;
					refreshOptionMenusForLanguageChange();
				}
				refreshSettingItemParams();
			}

			// クリックした項目がマウスカーソルの下から動かないよう、クリックによるカーソル移動ではスクロールしない
			if (mouseInput.clickedItemIdx.has_value())
			{
				m_isSettingMenuScrollFollowingCursor = false;
			}
			else if (changed)
			{
				m_isSettingMenuScrollFollowingCursor = true;
			}
			else if (Mouse::Wheel() != 0.0)
			{
				// マウスホイールによるスクロールを優先
				m_isSettingMenuScrollFollowingCursor = false;
			}
			updateSettingMenuScroll();
		}

		if (KeyConfig::Down(kButtonBack))
		{
			// ボタン編集中の場合は戻らない
			if (m_currentOptionMenuIdx == OptionMenuType::kKeyConfig && m_keyConfigMenu.isButtonEditingState())
			{
				// 何もしない
			}
			else
			{
				if (m_currentOptionMenuIdx == kDisplaySound)
					saveOutputSelection();
				if (m_returnToVertical)
				{
					exitScene();
					return;
				}
				m_currentOptionMenuIdx = none;
			}
		}
	}
	else
	{
		m_topMenu.update(m_canvas.get());

		if (KeyConfig::Down(kButtonStart) || m_topMenu.clickDecided())
		{
			m_currentOptionMenuIdx = m_topMenu.cursorAs<OptionMenuType>();

			// キーコンフィグ以外の設定メニューを開いた場合、設定項目ノードを作成
			if (m_currentOptionMenuIdx != OptionMenuType::kKeyConfig)
			{
				createSettingItemNodes();
				resetSettingMenuScroll();
			}
		}
		else if (KeyConfig::Down(kButtonBack))
		{
			exitScene();
		}
	}

	// 画面状態の判定
	const bool isTop = !m_currentOptionMenuIdx.has_value();
	const bool isKeyConfig = m_currentOptionMenuIdx == OptionMenuType::kKeyConfig;
	const bool isSetting = m_currentOptionMenuIdx.has_value() && !isKeyConfig;

	// ガイドテキストの決定
	String guideText;
	if (isTop)
	{
		guideText = I18n::Get(I18n::Option::GuideTop);
	}
	else if (isKeyConfig)
	{
		guideText = I18n::Get(I18n::Option::GuideKeyConfig);
	}
	else
	{
		guideText = I18n::Get(I18n::Option::GuideOption);
	}

	const int32 settingHeaderIndex = m_currentOptionMenuIdx.has_value() ? static_cast<int32>(*m_currentOptionMenuIdx) : -1;

	// 設定項目の上下端は、その方向にスクロールできる場合のみフェード表示
	bool isSettingMenuScrollableUp = false;
	bool isSettingMenuScrollableDown = false;
	if (isSetting)
	{
		if (const auto containerNode = m_canvas->findByName(U"SettingMenu"))
		{
			containerNode->refreshContainedCanvasLayoutImmediately();
			const double scrollY = containerNode->scrollOffset().y;
			const auto [minScroll, maxScroll] = containerNode->validScrollRange();
			isSettingMenuScrollableUp = scrollY > minScroll.y;
			isSettingMenuScrollableDown = scrollY < maxScroll.y;
		}
	}

	// Canvasパラメータの更新
	m_canvas->setParamValues({
		{ U"isTop", isTop },
		{ U"isSetting", isSetting },
		{ U"isKeyConfig", isKeyConfig },
		{ U"isSettingMenuScrollableUp", isSettingMenuScrollableUp },
		{ U"isSettingMenuScrollableDown", isSettingMenuScrollableDown },
		{ U"guideText", guideText },
		{ U"settingHeaderIndex", settingHeaderIndex },
		{ U"isJapanese", I18n::CurrentLanguage() == I18n::StandardLanguage::Japanese },
	});

	// Backボタンはキーコンフィグのボタン編集中は非表示
	m_commonOverlay.setBackButtonVisible(!(isKeyConfig && m_keyConfigMenu.isButtonEditingState()));

	// UI更新
	m_topMenu.updateUI(m_canvas.get());
	m_keyConfigMenu.updateUI(m_canvas.get());
	m_canvas->update(hitTestEnabled);
	updateAudioSelectionLog();
}

void OptionScene::saveOutputSelection()
{
	if (!Vertical::SettingsPersistence())
		return;
#ifdef _WIN32
	const String selected{ ConfigIni::GetString(ConfigIni::Key::kAudioBackend, U"default") };
	if (selected == m_savedOutputSelection)
		return;
	// Only persist the requested output. Never touch streams, output gain, or the
	// active calibration profile while this process continues to play.
	AudioCalibration::Store();
	ConfigIni::Save();
	const KSMIniData saved{ FsUtils::GetResourcePath(U"config.ini") };
	const bool success = saved.getString(ConfigIni::Key::kAudioBackend) == selected;
	if (!success)
	{
		ConfigIni::SetString(ConfigIni::Key::kAudioBackend, m_savedOutputSelection);
		m_outputError = I18n::Get(I18n::Mod::AudioSaveFailed);
		AudioOutputSettings::Log(U"output selection save failed: {}"_fmt(selected));
		logAudioOutput(m_outputError, true, false);
		refreshOptionMenusForLanguageChange();
		return;
	}
	m_savedOutputSelection = selected;
	m_outputError.clear();
	m_audioLog.clear();
	const bool pending = AudioOutputSettings::NeedsRestart(selected, m_activeOutputSelection, m_genericASIOSelection);
	logAudioOutput(String{ pending ? I18n::Get(I18n::Mod::AudioSavedNext) : I18n::Get(I18n::Mod::AudioCurrentAgain) }, false, !pending);
	AudioOutputSettings::Log(U"output saved for next launch: requested={}, active={} (no live switch)"_fmt(
		selected, m_activeOutputSelection));
#endif
}

void OptionScene::logAudioOutput(const String& title, bool error, bool includeMetrics)
{
	AudioLogEntry entry{ { title }, Scene::Time(), error };
	if (includeMetrics)
	{
		const auto metrics = ksmaudio::GetAudioLatencySnapshot();
		const auto& output = metrics.output;
		const String backend = output.backend == ksmaudio::AudioBackend::ASIO ? U"ASIO" : output.backend == ksmaudio::AudioBackend::DirectSound ? U"DirectSound"
																																				: U"Default / WASAPI";
		entry.lines.push_back(U"{} | {} | {} Hz"_fmt(backend, Unicode::FromUTF8(output.driver), output.sampleRate));
		if (metrics.reportedValid)
		{
			String latency = (output.backend == ksmaudio::AudioBackend::ASIO ? U"{} frames | "_fmt(output.bufferSamples) : U"") + String{ I18n::Get(I18n::Mod::ReportedLatency) } + U"{:.2f}ms"_fmt(output.latencySec * 1000);
			if (metrics.predictionValid)
				latency += U" | {:.2f}~{:.2f}ms (avg {:.2f})"_fmt(
					metrics.predictedMinSec * 1000, metrics.predictedMaxSec * 1000, metrics.predictedMeanSec * 1000);
			entry.lines.push_back(latency);
			if (output.backend == ksmaudio::AudioBackend::ASIO) entry.lines.push_back(String{ I18n::Get(I18n::Mod::AsioJudgmentCorrectionHelp) });
			entry.lines.push_back(String{ I18n::Get(I18n::Mod::LatencyEstimate) });
		}
	}
	m_audioLog.push_back(std::move(entry));
	while (m_audioLog.size() > 2)
		m_audioLog.erase(m_audioLog.begin());
}

void OptionScene::updateAudioSelectionLog()
{
	bool focused = false;
	String selectionLabel;
	if (m_currentOptionMenuIdx == kDisplaySound)
	{
		const auto& menu = (*m_optionMenus)[kDisplaySound];
		const auto items = menu.getSettingItemParamsList();
		const auto cursor = menu.cursor();
		focused = 0 <= cursor && static_cast<std::size_t>(cursor) < items.size() &&
				  items[cursor].name == I18n::Get(I18n::Option::ItemAudioBackend);
		if (focused)
			selectionLabel = items[cursor].value;
	}
	saveOutputSelection();
	const String selected{ ConfigIni::GetString(ConfigIni::Key::kAudioBackend, U"default") };
	if (focused && (!m_audioRowFocused || selected != m_loggedAudioSelection))
	{
		const bool pending = AudioOutputSettings::NeedsRestart(selected, m_activeOutputSelection, m_genericASIOSelection);
		logAudioOutput(pending ? String{ I18n::Get(I18n::Mod::NextLaunchOutput) } + selectionLabel : String{ I18n::Get(I18n::Mod::CurrentAudioOutput) }, false, !pending);
	}
	m_audioRowFocused = focused;
	m_loggedAudioSelection = selected;
}

void OptionScene::drawAudioLog() const
{
	const auto font = AssetManagement::SystemFont();
	const double scale = Clamp(Scene::Height() / 720.0, 0.7, 1.5);
	const double size = 14 * scale, margin = 16 * scale;
	const double width = Min(620 * scale, Scene::Width() - 2 * margin);
	const double lineHeight = 19 * scale;
	double bottom = Scene::Height() - 48 * scale;
	for (auto it = m_audioLog.rbegin(); it != m_audioLog.rend(); ++it)
	{
		const double age = Scene::Time() - it->createdAt;
		const double duration = it->error ? 8.0 : 5.0;
		if (age >= duration)
			continue;
		const double alpha = Min(1.0, duration - age);
		const double height = it->lines.size() * lineHeight + 16 * scale;
		const double left = Scene::Width() - width - margin, top = bottom - height;
		RectF{ left, top, width, height }.draw(ColorF{ 0.015, 0.025, 0.03, 0.88 * alpha });
		RectF{ left, top, 3 * scale, height }.draw(it->error ? ColorF{ 1.0, 0.45, 0.4, alpha } : ColorF{ 0.35, 0.85, 0.65, alpha });
		for (std::size_t i = 0; i < it->lines.size(); ++i)
		{
			String text = it->lines[i];
			const double maxWidth = width - 24 * scale;
			if (font(text).region(size).w > maxWidth)
			{
				while (!text.isEmpty() && font(text + U"…").region(size).w > maxWidth)
					text.pop_back();
				text += U"…";
			}
			font(text).draw(size, left + 12 * scale, top + 8 * scale + i * lineHeight, ColorF{ 0.93, alpha });
		}
		bottom = top - 6 * scale;
	}
}

void OptionScene::draw() const
{
	ksmaudio::qa::Scope qaScope{ "OptionScene.draw", false };
	m_canvas->draw();
	m_commonOverlay.draw();
	drawAudioLog();
}

Co::Task<void> OptionScene::fadeIn()
{
	co_await Co::ScreenFadeIn(kFadeDuration);
}

Co::Task<void> OptionScene::fadeOut()
{
	const auto canvasUpdateRunner = Co::UpdaterTask([this]
													{
														m_canvas->update(noco::HitTestEnabledYN::No);
														m_commonOverlay.update(noco::HitTestEnabledYN::No);
													})
										.runScoped();

	m_bgmStream.setFadeOut(kFadeDuration);
	co_await Co::ScreenFadeOut(kFadeDuration);
}

void OptionScene::exitScene()
{
	const auto candidateVerticalConfig = Vertical::ModConfig::Load();
	if (m_originalVerticalConfig.enabled)
		m_originalVerticalConfig.Save();
	if (Vertical::SettingsPersistence())
		AudioCalibration::Store();
	Vertical::SaveOptionsConfiguration();

	// Vsync設定を反映("0;120"または"1"の形式)
	const Array<String> vsyncParts = String{ ConfigIni::GetString(ConfigIni::Key::kVsync, U"0;300") }.split(U';');
	const bool vsyncEnabled = !vsyncParts.isEmpty() && vsyncParts[0] == U"1";
	Graphics::SetVSyncEnabled(vsyncEnabled);

	// フレームレート制限(Vsync有効時は無効化)
	const int32 fpsLimitValue = vsyncParts.size() >= 2 ? ParseOr<int32>(vsyncParts[1], 300) : 300;
	const Optional<int32> frameRateLimit = vsyncEnabled ? none : Optional<int32>(fpsLimitValue);
	Addon::GetAddon<FrameRateLimit>(FrameRateLimit::kAddonName)->setTargetFPS(frameRateLimit);

	// 画面サイズ反映
	if (m_originalVerticalConfig.enabled)
	{
		const bool changed = candidateVerticalConfig.displayMode != m_originalVerticalConfig.displayMode ||
							 candidateVerticalConfig.monitorId != m_originalVerticalConfig.monitorId ||
							 candidateVerticalConfig.windowSize.width != m_originalVerticalConfig.windowSize.width ||
							 candidateVerticalConfig.windowSize.height != m_originalVerticalConfig.windowSize.height;
		if (changed)
			Vertical::BeginDisplayChange(candidateVerticalConfig);
	}
	else
		ApplyScreenSizeConfig();

	// シーン遷移
	if (m_returnToVertical)
		requestNextScene<ModOptionsScene>(0, 1);
	else
		requestNextScene<TitleScene>(TitleMenuItem::kOption);
}

void OptionScene::refreshOptionMenusForLanguageChange()
{
	// 各メニューのカーソル位置を保存
	std::array<int32, kOptionMenuTypeEnumCount> cursorPositions;
	for (size_t i = 0; i < kOptionMenuTypeEnumCount; ++i)
	{
		cursorPositions[i] = (*m_optionMenus)[i].cursor();
	}

	// メニューを再生成
	m_optionMenus = makeOptionMenus();

	// カーソル位置を復元
	for (size_t i = 0; i < kOptionMenuTypeEnumCount; ++i)
	{
		(*m_optionMenus)[i].setCursor(cursorPositions[i]);
	}

	// UIを再作成
	createSettingItemNodes();
}

void OptionScene::createSettingItemNodes()
{
	const auto containerNode = m_canvas->findByName(U"SettingMenu");
	if (!containerNode)
	{
		Logger << U"[ksm error] OptionScene::createSettingItemNodes(): Container node 'SettingMenu' not found!";
		return;
	}

	// 既存の子を削除
	containerNode->removeChildrenAll();
	m_settingItemNodes.clear();

	// 各設定項目をSubCanvasとして追加
	const OptionMenu& optionMenu = (*m_optionMenus)[*m_currentOptionMenuIdx];
	const auto& headings = optionMenu.headings();
	auto headingItr = headings.begin();
	for (const auto& params : optionMenu.getSettingItemParamsList())
	{
		// カテゴリの先頭では見出しを追加
		for (; headingItr != headings.end() && headingItr->itemIdx <= static_cast<int32>(m_settingItemNodes.size()); ++headingItr)
		{
			containerNode->addSubCanvasNodeAsChild(
				U"ui/parts/option_setting_heading.noco",
				{
					{ U"name", headingItr->name },
				});
		}

		m_settingItemNodes.push_back(containerNode->addSubCanvasNodeAsChild(
			Vertical::UIPath(U"ui/parts/option_setting_item.noco"),
			{
				{ U"name", params.name },
				{ U"value", params.value },
				{ U"selected", params.selected },
				{ U"itemTint", params.enabled ? Color{ 255 } : Color{ 128 } },
				{ U"valueArrowIndex", params.valueArrowIndex },
				{ U"valueLeftArrowVisible", params.valueLeftArrowVisible },
				{ U"valueRightArrowVisible", params.valueRightArrowVisible },
			}));
	}
}

OptionMenu::MouseInput OptionScene::settingMenuMouseInput()
{
	OptionMenu::MouseInput mouseInput;

	// イベント発火元のSubCanvasを持つノードから設定項目のインデックスを取得
	const auto itemIdxOfEvent = [this](const noco::Event& event) -> Optional<int32>
	{
		const auto ownerNode = event.containedSubCanvasOwner.lock();
		if (!ownerNode)
		{
			return none;
		}
		for (int32 i = 0; i < static_cast<int32>(m_settingItemNodes.size()); ++i)
		{
			if (m_settingItemNodes[i] == ownerNode)
			{
				return i;
			}
		}
		return none;
	};

	for (const auto& event : m_canvas->getFiredEventsWithTag(U"onClickSettingItem"))
	{
		if (const auto itemIdx = itemIdxOfEvent(event))
		{
			mouseInput.clickedItemIdx = *itemIdx;
		}
	}

	// 左右端が押下された行へカーソルを移動した上で値を変更
	for (const auto& event : m_canvas->getFiredEventsWithTag(U"onPressRepeatValueLeft"))
	{
		if (const auto itemIdx = itemIdxOfEvent(event))
		{
			mouseInput.clickedItemIdx = *itemIdx;
			--mouseInput.valueDelta;
		}
	}
	for (const auto& event : m_canvas->getFiredEventsWithTag(U"onPressRepeatValueRight"))
	{
		if (const auto itemIdx = itemIdxOfEvent(event))
		{
			mouseInput.clickedItemIdx = *itemIdx;
			++mouseInput.valueDelta;
		}
	}

	return mouseInput;
}

void OptionScene::refreshSettingItemParams()
{
	const auto paramsList = (*m_optionMenus)[*m_currentOptionMenuIdx].getSettingItemParamsList();

	if (paramsList.size() != m_settingItemNodes.size())
	{
		Logger << U"[ksm error] OptionScene::refreshSettingItemParams(): Setting item node count mismatch! (params={}, nodes={})"_fmt(paramsList.size(), m_settingItemNodes.size());
		return;
	}

	for (size_t i = 0; i < paramsList.size(); ++i)
	{
		const auto& params = paramsList[i];
		const auto subCanvas = m_settingItemNodes[i]->getComponent<noco::SubCanvas>();
		if (!subCanvas)
		{
			Logger << U"[ksm error] OptionScene::refreshSettingItemParams(): SubCanvas component not found at index {}!"_fmt(i);
			continue;
		}

		const auto canvas = subCanvas->canvas();
		if (!canvas)
		{
			Logger << U"[ksm error] OptionScene::refreshSettingItemParams(): Canvas not loaded at index {}!"_fmt(i);
			continue;
		}

		canvas->setParamValues({
			{ U"name", params.name },
			{ U"value", params.value },
			{ U"selected", params.selected },
				{ U"itemTint", params.enabled ? Color{ 255 } : Color{ 128 } },
			{ U"valueArrowIndex", params.valueArrowIndex },
			{ U"valueLeftArrowVisible", params.valueLeftArrowVisible },
			{ U"valueRightArrowVisible", params.valueRightArrowVisible },
		});
	}
}

Optional<double> OptionScene::settingMenuScrollTargetY() const
{
	const auto containerNode = m_canvas->findByName(U"SettingMenu");
	if (!containerNode)
	{
		return none;
	}

	const int32 cursorIdx = (*m_optionMenus)[*m_currentOptionMenuIdx].cursor();
	if (cursorIdx < 0 || static_cast<int32>(m_settingItemNodes.size()) <= cursorIdx)
	{
		return none;
	}

	containerNode->refreshContainedCanvasLayoutImmediately();

	// カーソルが中央より下にある場合はカーソル位置の項目が中央に来るようにする
	const double itemCenterY = m_settingItemNodes[cursorIdx]->regionRect().centerY() + containerNode->scrollOffset().y;
	const auto [minScroll, maxScroll] = containerNode->validScrollRange();
	return Clamp(itemCenterY - containerNode->childrenRect().centerY(), minScroll.y, maxScroll.y);
}

void OptionScene::resetSettingMenuScroll()
{
	m_isSettingMenuScrollFollowingCursor = false;
	m_settingMenuScrollVelocity = 0.0;

	const auto containerNode = m_canvas->findByName(U"SettingMenu");
	const Optional<double> targetY = settingMenuScrollTargetY();
	if (!containerNode || !targetY.has_value())
	{
		return;
	}

	containerNode->setScrollOffset(Vec2{ 0.0, *targetY });
}

void OptionScene::updateSettingMenuScroll()
{
	if (!m_isSettingMenuScrollFollowingCursor)
	{
		m_settingMenuScrollVelocity = 0.0;
		return;
	}

	const auto containerNode = m_canvas->findByName(U"SettingMenu");
	const Optional<double> targetY = settingMenuScrollTargetY();
	if (!containerNode || !targetY.has_value())
	{
		m_isSettingMenuScrollFollowingCursor = false;
		return;
	}

	double scrollY = Math::SmoothDamp(containerNode->scrollOffset().y, *targetY, m_settingMenuScrollVelocity, kSettingMenuScrollSmoothTimeSec);
	if (Abs(*targetY - scrollY) < 0.01)
	{
		scrollY = *targetY;
		m_isSettingMenuScrollFollowingCursor = false;
	}
	containerNode->setScrollOffset(Vec2{ 0.0, scrollY });
}

#if KSM_ENABLE_DIAGNOSTICS
void OptionScene::runMigration220Checks()
{
	if (!ModDiagnostics::Arguments().contains(U"--silent-qa") ||
		!FileSystem::IsFile(FsUtils::GetResourcePath(U"audio-test-fixture")))
		throw Error{ U"Option migration checks require the isolated silent host" };
	TextWriter report{ FsUtils::GetResourcePath(U"options-220-test.txt") };
	int checks = 0;
	const auto check = [&](bool value, StringView name)
	{
		if (!value) throw Error{ U"2.2 option integration: {}"_fmt(name) };
		report.writeln(U"PASS {}"_fmt(name));
		++checks;
	};
	for (const String language : { U"English", U"Japanese", U"Korean", U"Simplified Chinese", U"Traditional Chinese" })
	{
		ConfigIni::SetString(ConfigIni::Key::kLanguage, language);
		I18n::LoadLanguage(language);
		// Recreate outside the field callback, using the same deferred refresh path.
		refreshOptionMenusForLanguageChange();
		for (int page = 0; page < 3; ++page)
		{
			m_currentOptionMenuIdx = static_cast<OptionMenuType>(page);
			createSettingItemNodes();
			auto& menu = (*m_optionMenus)[page];
			const auto fields = menu.getSettingItemParamsList();
			check(!fields.empty() && m_settingItemNodes.size() == fields.size(), U"heading nodes do not shift field indices");
			check(!menu.headings().empty(), U"upstream category headings are present");
			if (page == kDisplaySound)
				check(fields.any([](const auto& field) { return field.name == I18n::Get(I18n::Mod::AsioJudgmentCorrection); }), U"ASIO correction remains in sound settings");
			if (page == kOther)
				check(fields.any([](const auto& field) { return field.name == I18n::Get(I18n::Mod::RememberEffRate); }), U"EFF.RATE persistence remains in other settings");
			for (const bool last : { false, true })
			{
				menu.setCursor(last ? static_cast<int32>(fields.size()) - 1 : 0);
				refreshSettingItemParams();
				m_canvas->setParamValues({
					{ U"isTop", false }, { U"isSetting", true }, { U"isKeyConfig", false },
					{ U"settingHeaderIndex", page }, { U"isJapanese", language == U"Japanese" },
					{ U"guideText", String{ I18n::Get(I18n::Option::GuideOption) } },
				});
				m_canvas->update(Scene::Size(), noco::HitTestEnabledYN::No);
				resetSettingMenuScroll();
				m_canvas->update(Scene::Size(), noco::HitTestEnabledYN::No);
				const auto container = m_canvas->findByName(U"SettingMenu");
				check(container != nullptr, U"scroll container exists");
				const auto selected = m_settingItemNodes[menu.cursor()]->regionRect();
				const auto visible = container->childrenRect();
				check(selected.centerY() >= visible.y - 1 && selected.centerY() <= visible.bottomY() + 1,
					U"first and last selected rows remain inside the scroll viewport");
				RenderTexture output{ Scene::Size() };
				{
					const ScopedRenderTarget2D target{ output.clear(Palette::Black) };
					m_canvas->draw();
					Graphics2D::Flush();
				}
				Image image;
				output.readAsImage(image);
				check(!image.isEmpty(), U"localized option page renders");
				image.save(FsUtils::GetResourcePath(U"options-220-{}-{}-{}.png"_fmt(language, page, last ? U"last" : U"first")));
			}
		}
	}
	report.writeln(U"TOTAL {} option integration checks passed"_fmt(checks));
}
#endif
