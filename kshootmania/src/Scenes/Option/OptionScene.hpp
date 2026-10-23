#pragma once
#include <CoTaskLib.hpp>
#include "OptionTopMenu.hpp"
#include "OptionMenu.hpp"
#include "OptionKeyConfigMenu.hpp"
#include "Scenes/Common/CommonOverlay.hpp"
#include "ksmaudio/ksmaudio.hpp"
#include "Vertical/VerticalModConfig.hpp"

class OptionScene : public Co::UpdaterSceneBase
{
public:
	// TODO: OptionTopMenu::Itemと統一
	enum OptionMenuType : int32
	{
		kDisplaySound = 0,
		kInputJudgment,
		kOther,
		kKeyConfig,

		kOptionMenuTypeEnumCount,
	};

private:
	const Vertical::ModConfig m_originalVerticalConfig = Vertical::ModConfig::Load();
	std::shared_ptr<noco::Canvas> m_canvas;

	OptionTopMenu m_topMenu;

	std::unique_ptr<std::array<OptionMenu, kOptionMenuTypeEnumCount>> m_optionMenus;

	OptionKeyConfigMenu m_keyConfigMenu;

	CommonOverlay m_commonOverlay;

	Optional<OptionMenuType> m_currentOptionMenuIdx = none;
    const bool m_returnToVertical;
    int m_switchSmokeStage = 0;
    Stopwatch m_switchSmokeClock{StartImmediately::Yes};
    String m_switchSmokeASIO;
    Array<String> m_switchSmokeTargets;
    String m_switchSmokeHistory;
    String m_outputError;
    String m_activeOutputSelection;
    String m_savedOutputSelection;
    String m_genericASIOSelection;
    struct AudioLogEntry { Array<String> lines; double createdAt; bool error; };
    Array<AudioLogEntry> m_audioLog;
    String m_loggedAudioSelection;
    bool m_audioRowFocused = false;
    void logAudioOutput(const String& title, bool error = false, bool includeMetrics = true);
    void updateAudioSelectionLog();
    void drawAudioLog() const;
    void saveOutputSelection();

	ksmaudio::Stream m_bgmStream{ "se/option_bgm.ogg", 1.0, false, false, true };

	std::unique_ptr<std::array<OptionMenu, kOptionMenuTypeEnumCount>> makeOptionMenus();

	OptionMenu::MouseInput settingMenuMouseInput();

	void refreshOptionMenusForLanguageChange();

	void createSettingItemNodes();

	void refreshSettingItemParams();

public:
	explicit OptionScene(bool displayShortcut=false);

	virtual void update() override;

	virtual void draw() const override;

	virtual Co::Task<void> fadeIn() override;

	virtual Co::Task<void> fadeOut() override;

	void exitScene();
};
