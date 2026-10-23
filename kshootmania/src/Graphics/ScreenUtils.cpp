#include "Graphics/GameRenderContext.hpp"
#include "ScreenUtils.hpp"
#include "Vertical/VerticalDisplay.hpp"
#include "Input/KeyConfig.hpp"
#include "Input/InputUtils.hpp"
#include "UI/ModMenuStyle.hpp"
#include "Input/MenuInputProfiles.hpp"
#ifdef _WIN32
#include <Windows.h>
#endif

TextureRegion ScreenUtils::FitToHeight(const Texture& texture)
{
	const double height = GameRender::Height();
	const double width = texture.width() * height / texture.height();
	return texture.resized(width, height);
}

void ScreenUtils::ApplyScreenSizeConfig()
{
	Scene::SetLetterbox(Palette::Black);
	if (GameRender::VerticalEnabled())
	{
		Vertical::ApplyConfiguredDisplay();
		return;
	}
	Window::SetFullscreen(false);
	Window::SetStyle(WindowStyle::Fixed);
	const bool isFullScreen = ConfigIni::GetBool(ConfigIni::Key::kFullScreen, false);
	const String resolutionStr{ ConfigIni::GetString(isFullScreen ? ConfigIni::Key::kFullScreenResolution : ConfigIni::Key::kWindowResolution, U"800,600") };
	const Array<int32> resolution = resolutionStr.split(U',').map([](const String& str) { return ParseOr<int32>(str, 0); });
	Scene::SetResizeMode(ResizeMode::Keep);
	
	if (resolution.size() == 2U && resolution[0] > 0 && resolution[1] > 0)
	{
		Scene::Resize(resolution[0], resolution[1]);
		Window::Resize(resolution[0], resolution[1]);
	}
	else
	{
		Scene::Resize(800, 600);
		Window::Resize(800, 600);
	}
#ifdef __EMSCRIPTEN__
	// Web版ではフルスクリーン切り替えにユーザー操作が必要なため起動時には行わない
#else
	Window::SetFullscreen(isFullScreen);
#endif

#ifdef _WIN32
	// 他のウィンドウに切り替えられるようTOPMOSTを解除
	if (isFullScreen)
	{
		const auto hWnd = static_cast<HWND>(Platform::Windows::Window::GetHWND());
		if (hWnd != NULL)
		{
			SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
	}
#endif
}

namespace Vertical
{
    namespace
    {
        Optional<ModConfig> previousConfig;
        bool applyOnNextUpdate = false;
        Stopwatch confirmationTimer;
        Size previousSceneSize;
        Size previousFrameBufferSize;
        Point previousWindowPosition;
        bool confirmationArmed = false;
        int32 confirmationSelection = 0;
        bool persistConfig = true;

        size_t MonitorIndex(const ModConfig& config, const Array<MonitorInfo>& monitors)
        {
            for (size_t i = 0; i < monitors.size(); ++i)
                if (monitors[i].id == config.monitorId) return i;
            return Min(System::GetCurrentMonitorIndex(), monitors.size() - 1);
        }
    }

    void ApplyConfiguredDisplay()
    {
        const auto config = ModConfig::Load();
        const auto monitors = System::EnumerateMonitors();
        if (monitors.isEmpty()) throw Error{U"No display available"};
        const auto& monitor = monitors[MonitorIndex(config, monitors)];
        Window::SetFullscreen(false);
        Window::Restore();
        Scene::SetResizeMode(ResizeMode::Keep);
        if (config.displayMode == DisplayMode::Fullscreen)
            throw Error{U"Exclusive fullscreen requires a renderer extension; use windowed or borderless for this prototype."};

        if (config.displayMode == DisplayMode::BorderlessFullscreen)
        {
            Window::SetStyle(WindowStyle::Frameless);
            Window::SetPos(monitor.displayRect.pos);
            if (!Window::ResizeActual(monitor.displayRect.size, Centering::No))
                throw Error{U"Could not resize borderless window"};
            Window::SetPos(monitor.displayRect.pos);
            // Keep the logical render resolution independent of the desktop.
            // Siv3D fits this scene uniformly and letterboxes the unused area.
            Scene::Resize(config.windowSize.width, config.windowSize.height);
        }
        else
        {
            Window::SetStyle(WindowStyle::Fixed);
            Window::SetPos(monitor.workArea.pos);
            const double dpiScale = monitor.scaling.value_or(1.0);
            const double fit = Min(1.0, Min(
                Max(1, monitor.workArea.w - static_cast<int>(32 * dpiScale)) / static_cast<double>(config.windowSize.width),
                Max(1, monitor.workArea.h - static_cast<int>(80 * dpiScale)) / static_cast<double>(config.windowSize.height)));
            const Size size{Max(1, static_cast<int>(config.windowSize.width * fit)),
                Max(1, static_cast<int>(config.windowSize.height * fit))};
            if (!Window::ResizeActual(size, Centering::No))
                throw Error{U"Could not resize vertical window"};
            Scene::Resize(config.windowSize.width, config.windowSize.height);
            Window::SetPos(monitor.workArea.pos + Point{
                Max(0, (monitor.workArea.w - Window::GetState().bounds.w) / 2),
                Max(0, (monitor.workArea.h - Window::GetState().bounds.h) / 2)});
        }
#ifdef _WIN32
        const auto hwnd = static_cast<HWND>(Platform::Windows::Window::GetHWND());
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
    }

    bool IsDisplayChangePending() { return previousConfig.has_value(); }
    void SetDisplayConfigPersistence(bool enabled) { persistConfig = enabled; }

    bool BeginDisplayChange(const ModConfig& next)
    {
        if (previousConfig || (next.enabled && next.displayMode == DisplayMode::Fullscreen)) return false;
        previousConfig = ModConfig::Load();
        previousSceneSize = GameRender::Size();
        previousFrameBufferSize = Window::GetState().frameBufferSize;
        previousWindowPosition = Window::GetPos();
        next.Save();
        // Window operations must run after System::Update(), not inside the
        // scene addon's update/fade coroutine and its input/layout pass.
        applyOnNextUpdate = true;
        confirmationArmed = false;
        confirmationSelection = 0;
        return true;
    }

    void RestorePendingDisplayChange()
    {
        if (!previousConfig) return;
        const auto restore = *previousConfig;
        previousConfig.reset();
        applyOnNextUpdate = false;
        restore.Save();
        ApplyScreenSizeConfig();
        if (!restore.enabled || restore.displayMode == DisplayMode::Windowed)
        {
            Window::ResizeActual(previousFrameBufferSize, Centering::No);
            Scene::Resize(previousSceneSize);
            Window::SetPos(previousWindowPosition);
        }
        if (persistConfig) ConfigIni::Save();
    }

    void UpdateDisplayConfirmation()
    {
        if (!previousConfig) return;
        if (applyOnNextUpdate)
        {
            applyOnNextUpdate = false;
            try { ApplyScreenSizeConfig(); }
            catch (const Error& error)
            {
                Logger << error.what();
                RestorePendingDisplayChange();
                return;
            }
#ifdef _WIN32
            const auto hwnd = static_cast<HWND>(Platform::Windows::Window::GetHWND());
            // One activation for this explicit user display change. Do not
            // repeatedly steal focus when the user switches to another app.
            ::SetForegroundWindow(hwnd);
            ::SetActiveWindow(hwnd);
            ::SetFocus(hwnd);
#endif
            KeyConfig::ClearInput(kButtonStart);
            KeyConfig::ClearInput(kButtonBack);
            MouseL.clearInput();
            confirmationTimer.restart();
            return;
        }
        static CursorInput navigation{MenuInputProfiles::OptionValues()};navigation.update();
        if (!KeyConfig::Pressed(kButtonStart)) confirmationArmed = true;
        if(confirmationArmed){
            confirmationSelection=Clamp(confirmationSelection+navigation.deltaCursor(),0,1);
            if(InputUtils::IsUIMouseInputEnabled()&&(Cursor::DeltaF().lengthSq()>0||MouseL.down()))
                for(int32 i=0;i<2;++i)if(ModMenuStyle::DialogButton(2,i).mouseOver())confirmationSelection=i;
        }
        if (confirmationTimer.sF() >= 15.0 || KeyConfig::Down(kButtonBack) ||
            (confirmationArmed && confirmationSelection==1 && KeyConfig::Down(kButtonStart)) ||
            (confirmationArmed && InputUtils::IsUIMouseInputEnabled() && ModMenuStyle::DialogButton(2,1).leftClicked()))
        {
            RestorePendingDisplayChange();
            KeyConfig::ClearInput(kButtonBack);
        }
        else if (confirmationArmed && ((confirmationSelection==0 && KeyConfig::Down(kButtonStart)) ||
            (InputUtils::IsUIMouseInputEnabled() && ModMenuStyle::DialogButton(2,0).leftClicked())))
        {
            previousConfig.reset();
            if (persistConfig) ConfigIni::Save();
            KeyConfig::ClearInput(kButtonStart);
        }
    }

    int32 DisplayConfirmationSelection(){return confirmationSelection;}

    void DrawDisplayConfirmation()
    {
        if (!previousConfig) return;
        GameRender::Rect().draw(ColorF{0,.6});
        const double u=ModMenuStyle::Unit();const auto p=ModMenuStyle::DialogPanel(2);ModMenuStyle::Panel(p);
        ModMenuStyle::Text(U"KEEP DISPLAY?  {} s"_fmt(Max(0,15-static_cast<int>(confirmationTimer.sF()))),{p.x+16*u,p.y+20*u,p.w-32*u,40*u},26*u);
        for(int32 i=0;i<2;++i){const auto button=ModMenuStyle::DialogButton(2,i);button.draw(ColorF{1,i==confirmationSelection?.18:.1});if(i==confirmationSelection)button.drawFrame(u,Palette::White);ModMenuStyle::Hover(button,confirmationArmed);ModMenuStyle::Text(i==0?U"KEEP":U"RESTORE",button,23*u);}
        ModMenuStyle::Text(U"START: {}   BACK: {}"_fmt(I18n::Get(I18n::Mod::Keep),I18n::Get(I18n::Mod::Restore)),{p.x+12*u,p.y+142*u,p.w-24*u,22*u},14*u);
    }
}
