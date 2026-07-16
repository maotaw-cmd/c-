// SPECTRA UI REVISION 20 - WINDOWS TYPES / SDK COMPILE FIX
// SOURCE MARKER: REV25_PREVIEW_REMOVED
#pragma message("REV35_SLIDERS_CLEAN_CONFIG_ACTIVE - draggable desktop keybind and spectator windows")

#pragma message("REV35_SLIDERS_CLEAN_CONFIG_ACTIVE")

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

// Standard C/C++ types first. This prevents uintptr_t, va_list and tm errors.
#include <cstdint>
#include <cstdarg>
#include <cstddef>
#include <ctime>
#include <cwchar>
#include <cwctype>

// Windows SDK must be included at global scope, never inside extern "C".
#include <Windows.h>
#include <Windowsx.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dwmapi.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <type_traits>

static_assert(sizeof(DWORD) == 4, "Windows SDK DWORD type is unavailable or incorrect.");
static_assert(sizeof(ULONG) == 4, "Windows SDK ULONG type is unavailable or incorrect.");
static_assert(sizeof(std::uintptr_t) == sizeof(void*), "uintptr_t is unavailable or incorrect.");

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

template <typename T>
void SafeRelease(T*& p)
{
    if (p)
    {
        p->Release();
        p = nullptr;
    }
}

static D2D1_COLOR_F Col(UINT32 rgb, float a = 1.0f)
{
    return D2D1::ColorF(
        ((rgb >> 16) & 0xFF) / 255.0f,
        ((rgb >> 8) & 0xFF) / 255.0f,
        (rgb & 0xFF) / 255.0f,
        a);
}

static bool Hit(float x, float y, float l, float t, float r, float b)
{
    return x >= l && x <= r && y >= t && y <= b;
}

namespace Ui
{
    constexpr int Width = 900;
    constexpr int Height = 700;
        constexpr float Sidebar = 215.0f;
    constexpr float Topbar = 66.0f;
    constexpr float Radius = 8.0f;
}

struct Theme
{
    D2D1_COLOR_F bg = Col(0x090B0D, 0.93f);
    D2D1_COLOR_F sidebar = Col(0x0B0E11, 0.96f);
    D2D1_COLOR_F top = Col(0x0B0E11, 0.94f);
    D2D1_COLOR_F row = Col(0x101417, 0.86f);
    D2D1_COLOR_F rowHover = Col(0x14191D, 0.96f);
    D2D1_COLOR_F rowDark = Col(0x0D1013, 0.84f);
    D2D1_COLOR_F border = Col(0x1A2025, 0.88f);
    D2D1_COLOR_F text = Col(0xD5D8DB);
    D2D1_COLOR_F sub = Col(0xA5ABB1);
    D2D1_COLOR_F faint = Col(0x747B82);
    D2D1_COLOR_F red = Col(0xFF174C);
    D2D1_COLOR_F redSoft = Col(0x7F1733, 0.55f);
    D2D1_COLOR_F cyan = Col(0x00D9FF);
    D2D1_COLOR_F blue = Col(0x2B8CFF);
    D2D1_COLOR_F white = Col(0xFFFFFF);
};

Theme g_theme;

enum class Page
{
    Visuals,
    Players,
    World,
    View,
    Configs
};

struct State
{
    Page page = Page::Visuals;
    bool enemy = true;
    bool local = false;
    bool ally = false;

    bool esp = true;
    bool throughWalls = true;
    int yieldingFrames = 1;
    bool limitDistance = false;
    bool lastVisiblePosition = false;
    bool box = true;
    bool health = true;
    bool weapon = true;
    bool name = true;
    bool skeleton = true;
    bool sound = false;
    bool offscreen = true;

    bool chams = true;
    bool chamsThroughWalls = true;
    bool chamsDistance = false;
    int visibleMaterial = 0;
    int occludedMaterial = 1;
    int colourMode = 0;
    bool overlayVisible = true;

    // Editable colors used by the custom RGB picker.
    D2D1_COLOR_F espThroughWallsColor = Col(0xFF174C);
    D2D1_COLOR_F chamsEnabledColor = Col(0x0085FF);
    D2D1_COLOR_F chamsThroughWallsColor = Col(0xBD4464);
    D2D1_COLOR_F overlayVisibleColor = Col(0x00F9FF);

    // Per-element preview colors, editable from each setting gear popup.
    D2D1_COLOR_F boxColor = Col(0xF1F3F5);
    D2D1_COLOR_F healthHighColor = Col(0x43D98C);
    D2D1_COLOR_F healthMidColor = Col(0xFF9F43);
    D2D1_COLOR_F healthLowColor = Col(0xFF3B5C);
    D2D1_COLOR_F weaponColor = Col(0xB7C0C8);
    D2D1_COLOR_F nameColor = Col(0xFFFFFF);
    D2D1_COLOR_F skeletonColor = Col(0xDDE3E8);

    // Aimbot page (UI demonstration variables).
    bool aimbotEnabled = true;
    bool silentAim = false;
    bool autoFire = false;
    bool autoScope = false;
    bool recoilControl = true;
    bool targetLock = false;
    bool visibleOnly = true;
    bool ignoreSmoke = false;
    bool minDamageOverride = true;
    bool forceBodyAim = false;
    bool forceSafePoint = false;
    int targetSelection = 0;
    int hitbox = 0;
    int hitChance = 65;
    int minimumDamage = 50;
    int overrideDamage = 70;
    int aimFov = 8;
    int smoothing = 4;
    int overrideKey = 0;

    // World page
    bool worldItems = true;
    bool worldObjectives = true;
    bool worldDistance = false;
    int worldQuality = 1;

    // View page - these options immediately update the live preview.
    bool crosshair = true;
    bool watermark = true;
    bool showName = true;
    bool showFps = true;
    bool showClock = false;
    bool statusIndicator = true;
    bool compactMode = false;
    int viewScale = 100;

    // Additional interface/menu variables (UI demonstration only).
    bool notifications = true;
    bool menuParticles = true;
    bool menuBlur = true;
    bool rememberPosition = true;
    bool streamProof = false;
    bool keybindList = true;
    bool spectatorList = true;
    bool animationEffects = true;
    int menuTheme = 0;
    int watermarkPosition = 0;
    int animationSpeed = 1;
};

class App
{
public:
    bool Init(HINSTANCE instance);
    int Run();
    void Shutdown();

private:
    HWND hwnd_ = nullptr;
    HWND previewHwnd_ = nullptr;
    HWND watermarkHwnd_ = nullptr;
    HWND keybindHwnd_ = nullptr;
    HWND spectatorHwnd_ = nullptr;
    HINSTANCE instance_ = nullptr;
    ID2D1Factory* factory_ = nullptr;
    ID2D1HwndRenderTarget* target_ = nullptr;
    ID2D1SolidColorBrush* brush_ = nullptr;
    IDWriteFactory* writeFactory_ = nullptr;
    IDWriteTextFormat* text_ = nullptr;
    IDWriteTextFormat* small_ = nullptr;
    IDWriteTextFormat* tiny_ = nullptr;
    IDWriteTextFormat* medium_ = nullptr;
    IDWriteTextFormat* bold_ = nullptr;
    IDWriteTextFormat* logo_ = nullptr;
    IDWriteTextFormat* icons_ = nullptr;

    POINT mouse_{ -1000, -1000 };
    bool pressed_ = false;
    bool down_ = false;
    std::wstring openCombo_;

    // A dropdown is drawn in two passes: its row during normal layout, then its
    // popup after every card. This guarantees the popup is always top-most.
    bool dropdownOverlayReady_ = false;
    std::wstring dropdownOverlayId_;
    std::vector<std::wstring> dropdownOverlayItems_;
    int* dropdownOverlaySelection_ = nullptr;
    float dropdownOverlayX_ = 0.0f;
    float dropdownOverlayY_ = 0.0f;
    float dropdownOverlayW_ = 0.0f;

    State state_;

    // Modal RGB color picker state.
    bool colorPickerOpen_ = false;
    D2D1_COLOR_F* pickerColor_ = nullptr;
    D2D1_COLOR_F pickerOriginal_{};
    int activeRgbSlider_ = -1;

    // Small advanced popup opened by the gear icon beside a toggle.
    bool gearPopupOpen_ = false;
    std::wstring gearPopupTitle_;
    bool* gearPopupValue_ = nullptr;
    D2D1_COLOR_F* gearPopupColor_ = nullptr;
    float gearStrength_ = 65.0f;

    // Smooth animated preview damage state.
    float previewDisplayedHp_ = 100.0f;
    float previewTargetHp_ = 100.0f;
    float previewPreviousHp_ = 100.0f;
    float previewDamageTextTimer_ = 0.0f;
    ULONGLONG previewLastDamageTick_ = 0;
    int gearMode_ = 0;

    // Named configuration manager. Users can create any number of configs.
    std::wstring configName_ = L"My Config";
    bool configNameFocused_ = false;
    int selectedConfig_ = 0;
    std::vector<std::wstring> configNames_;
    std::wstring configStatus_;
    float configStatusTime_ = 0.0f;

    // Runtime overlay counters used by the real watermark/FPS display.
    ULONGLONG fpsWindowStart_ = GetTickCount64();
    unsigned int fpsFrameCount_ = 0;
    int currentFps_ = 0;

    // Live menu-interface state.
    bool interfaceInitialized_ = false;
    bool lastBlur_ = true;
    bool lastStreamProof_ = false;
    int lastTheme_ = -1;
    int lastViewScale_ = -1;
    POINT savedWindowPos_{ -1, -1 };
    POINT savedKeybindPos_{ -1, -1 };
    POINT savedSpectatorPos_{ -1, -1 };

    static LRESULT CALLBACK StaticProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK PreviewProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK WatermarkProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK OverlayProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT Proc(UINT, WPARAM, LPARAM);
    LRESULT PreviewWindowProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT WatermarkWindowProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT OverlayWindowProc(HWND, UINT, WPARAM, LPARAM);

    HRESULT CreateDevice();
    void DiscardDevice();
    void Render();
    void EnableBlur();

    void Set(D2D1_COLOR_F c);
    void Fill(float l, float t, float r, float b, D2D1_COLOR_F c);
    void Rect(float l, float t, float r, float b, D2D1_COLOR_F c, float w = 1.0f);
    void Round(float l, float t, float r, float b, float radius, D2D1_COLOR_F c);
    void RoundRect(float l, float t, float r, float b, float radius, D2D1_COLOR_F c, float w = 1.0f);
    void Line(float x1, float y1, float x2, float y2, D2D1_COLOR_F c, float w = 1.0f);
    void Circle(float cx, float cy, float radius, D2D1_COLOR_F c, bool fill = false, float stroke = 1.5f);
    void DrawSwitch(bool value, float x, float y);
    void DrawNavIcon(Page p, float x, float y, D2D1_COLOR_F c);
    void DrawGear(float cx, float cy, D2D1_COLOR_F c);
    void Text(const std::wstring& value, float x, float y, float w, float h,
              D2D1_COLOR_F c, IDWriteTextFormat* format = nullptr,
              DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING);
    bool Hover(float l, float t, float r, float b) const;

    void DrawTopbar(float width);
    void DrawSidebar(float height);
    void DrawContent(float width, float height);
    void DrawVisualsPage(float x, float y, float width, float height);
    void DrawEmbeddedPreview(float x, float y, float w, float h);
    void DrawPlayers(float x, float y, float width);
    void DrawWorld(float x, float y, float width);
    void DrawView(float x, float y, float width);
    void DrawConfigs(float x, float y, float width);
    void DrawPlaceholder(float x, float y, float width, const std::wstring& title);

    void NavItem(Page p, const std::wstring& glyph, const std::wstring& label,
                 float x, float y, bool child = false);
    void Tab(const std::wstring& label, bool& value, float x, float y, float w);
    void SectionLabel(const std::wstring& label, float x, float y, float w);
    void RowBase(float x, float y, float w, float h, bool hover = false);
    void Toggle(const std::wstring& label, bool& value, float x, float y, float w, float h = 42.0f);
    void ColorToggle(const std::wstring& label, D2D1_COLOR_F& swatch,
                     bool& value, float x, float y, float w);
    std::wstring HexColor(const D2D1_COLOR_F& color) const;
    void OpenColorPicker(D2D1_COLOR_F& color);
    void DrawColorPicker(float width, float height);
    void RgbSlider(const std::wstring& label, float& channel, int sliderId,
                   float x, float y, float w);
    void GearToggle(const std::wstring& label, bool& value, float x, float y, float w,
                    D2D1_COLOR_F* color = nullptr);
    void AimbotGearToggle(const std::wstring& label, bool& value, int mode,
                          float x, float y, float w);
    void Combo(const std::wstring& id, const std::wstring& label,
               const std::vector<std::wstring>& items,
               int& selected, float x, float y, float w);
    void ValueSlider(const std::wstring& label, int& value, int minimum, int maximum,
                     int step, const std::wstring& suffix, float x, float y, float w);
    void DrawDropdownOverlay();
    void DrawGearPopup(float width, float height);
    void DrawRuntimeOverlay(float width);
    void ApplyInterfaceSettings();
    void ApplyTheme();
    void ApplyBlurSetting();
    void ApplyStreamProofSetting();
    void ApplyScaleSetting();
    void LoadWindowPosition();
    void SaveWindowPosition();
    std::filesystem::path InterfacePath() const;
    void UpdateCompanionWindows();
    void PaintPreviewWindow(HWND hwnd);
    void PaintWatermarkWindow(HWND hwnd);
    void PaintOverlayWindow(HWND hwnd, bool keybind);
    void SaveOverlayPositions();
    std::wstring BuildWatermarkText() const;
    bool ActionButton(const std::wstring& label, float x, float y, float w, bool accent = false, bool danger = false);
    std::filesystem::path ConfigDirectory() const;
    std::filesystem::path ConfigPath(const std::wstring& name) const;
    std::wstring SanitizeConfigName(const std::wstring& name) const;
    void RefreshConfigs();
    bool SaveConfig(const std::wstring& name);
    bool LoadConfig(const std::wstring& name);
    bool DeleteConfig(const std::wstring& name);
    void Footer(float height);
};

HRESULT App::CreateDevice()
{
    if (target_)
        return S_OK;

    RECT rc{};
    GetClientRect(hwnd_, &rc);

    const auto props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_HARDWARE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    HRESULT hr = factory_->CreateHwndRenderTarget(
        props,
        D2D1::HwndRenderTargetProperties(
            hwnd_,
            D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top),
            D2D1_PRESENT_OPTIONS_IMMEDIATELY),
        &target_);

    if (FAILED(hr))
        return hr;

    return target_->CreateSolidColorBrush(g_theme.white, &brush_);
}

void App::DiscardDevice()
{
    SafeRelease(brush_);
    SafeRelease(target_);
}

void App::EnableBlur()
{
    MARGINS margins{ -1 };
    DwmExtendFrameIntoClientArea(hwnd_, &margins);

    DWM_BLURBEHIND blur{};
    blur.dwFlags = DWM_BB_ENABLE;
    blur.fEnable = TRUE;
    blur.hRgnBlur = nullptr;
    DwmEnableBlurBehindWindow(hwnd_, &blur);
}

bool App::Init(HINSTANCE instance)
{
    instance_ = instance;
    SetProcessDPIAware();

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory_)))
        return false;

    if (FAILED(DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&writeFactory_))))
        return false;

    auto makeFont = [&](const wchar_t* family, float size, DWRITE_FONT_WEIGHT weight,
                        IDWriteTextFormat** out)
    {
        HRESULT hr = writeFactory_->CreateTextFormat(
            family, nullptr, weight,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            size, L"en-US", out);
        if (SUCCEEDED(hr))
        {
            (*out)->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            (*out)->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
        return hr;
    };

    if (FAILED(makeFont(L"Segoe UI", 13.0f, DWRITE_FONT_WEIGHT_NORMAL, &text_))) return false;
    if (FAILED(makeFont(L"Segoe UI", 11.5f, DWRITE_FONT_WEIGHT_NORMAL, &small_))) return false;
    if (FAILED(makeFont(L"Segoe UI", 9.5f, DWRITE_FONT_WEIGHT_NORMAL, &tiny_))) return false;
    if (FAILED(makeFont(L"Segoe UI", 14.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, &medium_))) return false;
    if (FAILED(makeFont(L"Segoe UI", 16.0f, DWRITE_FONT_WEIGHT_BOLD, &bold_))) return false;
    if (FAILED(makeFont(L"Segoe UI", 14.0f, DWRITE_FONT_WEIGHT_BOLD, &logo_))) return false;
    if (FAILED(makeFont(L"Segoe MDL2 Assets", 16.0f, DWRITE_FONT_WEIGHT_NORMAL, &icons_))) return false;

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = StaticProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"SpectraUiRev17Main";
    RegisterClassExW(&wc);

    WNDCLASSEXW previewClass{ sizeof(previewClass) };
    previewClass.style = CS_HREDRAW | CS_VREDRAW;
    previewClass.lpfnWndProc = PreviewProc;
    previewClass.hInstance = instance;
    previewClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    previewClass.lpszClassName = L"SpectraUiRev17Preview";
    RegisterClassExW(&previewClass);

    WNDCLASSEXW watermarkClass{ sizeof(watermarkClass) };
    watermarkClass.style = CS_HREDRAW | CS_VREDRAW;
    watermarkClass.lpfnWndProc = WatermarkProc;
    watermarkClass.hInstance = instance;
    watermarkClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    watermarkClass.lpszClassName = L"SpectraUiRev17Watermark";
    RegisterClassExW(&watermarkClass);

    WNDCLASSEXW overlayClass{ sizeof(overlayClass) };
    overlayClass.style = CS_HREDRAW | CS_VREDRAW;
    overlayClass.lpfnWndProc = OverlayProc;
    overlayClass.hInstance = instance;
    overlayClass.hCursor = LoadCursor(nullptr, IDC_SIZEALL);
    overlayClass.lpszClassName = L"SpectraUiRev34DesktopOverlay";
    RegisterClassExW(&overlayClass);

    LoadWindowPosition();
    int x = savedWindowPos_.x >= 0 ? savedWindowPos_.x : (GetSystemMetrics(SM_CXSCREEN) - Ui::Width) / 2;
    int y = savedWindowPos_.y >= 0 ? savedWindowPos_.y : (GetSystemMetrics(SM_CYSCREEN) - Ui::Height) / 2;

    hwnd_ = CreateWindowExW(
        WS_EX_LAYERED,
        wc.lpszClassName,
        L"Spectra UI Revision 35 - Real External Overlays",
        WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU,
        x, y, Ui::Width, Ui::Height,
        nullptr, nullptr, instance, this);

    if (!hwnd_)
        return false;

    watermarkHwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        watermarkClass.lpszClassName,
        L"Spectra Watermark",
        WS_POPUP,
        0, 0, 260, 34,
        nullptr, nullptr, instance, this);

    if (!watermarkHwnd_)
        return false;

    const int keyX = savedKeybindPos_.x >= 0 ? savedKeybindPos_.x : x + Ui::Width + 18;
    const int keyY = savedKeybindPos_.y >= 0 ? savedKeybindPos_.y : y + 80;
    const int specX = savedSpectatorPos_.x >= 0 ? savedSpectatorPos_.x : x + Ui::Width + 18;
    const int specY = savedSpectatorPos_.y >= 0 ? savedSpectatorPos_.y : y + 196;

    keybindHwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        overlayClass.lpszClassName, L"Spectra Keybinds", WS_POPUP,
        keyX, keyY, 184, 104, nullptr, nullptr, instance, this);
    spectatorHwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        overlayClass.lpszClassName, L"Spectra Spectators", WS_POPUP,
        specX, specY, 184, 82, nullptr, nullptr, instance, this);
    if (!keybindHwnd_ || !spectatorHwnd_)
        return false;

    SetLayeredWindowAttributes(hwnd_, 0, 255, LWA_ALPHA);
    EnableBlur();
    ApplyInterfaceSettings();

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    UpdateCompanionWindows();
    SetTimer(hwnd_, 1, 33, nullptr);
    return true;
}

int App::Run()
{
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

void App::Shutdown()
{
    if (hwnd_)
        KillTimer(hwnd_, 1);
    SaveOverlayPositions();
    if (keybindHwnd_) DestroyWindow(keybindHwnd_);
    if (spectatorHwnd_) DestroyWindow(spectatorHwnd_);
    if (watermarkHwnd_) DestroyWindow(watermarkHwnd_);
    keybindHwnd_ = nullptr;
    spectatorHwnd_ = nullptr;
    watermarkHwnd_ = nullptr;

    DiscardDevice();
    SafeRelease(icons_);
    SafeRelease(logo_);
    SafeRelease(bold_);
    SafeRelease(medium_);
    SafeRelease(tiny_);
    SafeRelease(small_);
    SafeRelease(text_);
    SafeRelease(writeFactory_);
    SafeRelease(factory_);
}

void App::Set(D2D1_COLOR_F c)
{
    brush_->SetColor(c);
}

void App::Fill(float l, float t, float r, float b, D2D1_COLOR_F c)
{
    Set(c);
    target_->FillRectangle(D2D1::RectF(l, t, r, b), brush_);
}

void App::Rect(float l, float t, float r, float b, D2D1_COLOR_F c, float w)
{
    Set(c);
    target_->DrawRectangle(D2D1::RectF(l, t, r, b), brush_, w);
}

void App::Round(float l, float t, float r, float b, float radius, D2D1_COLOR_F c)
{
    Set(c);
    target_->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(l, t, r, b), radius, radius), brush_);
}

void App::RoundRect(float l, float t, float r, float b, float radius, D2D1_COLOR_F c, float w)
{
    Set(c);
    target_->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(l, t, r, b), radius, radius), brush_, w);
}

void App::Line(float x1, float y1, float x2, float y2, D2D1_COLOR_F c, float w)
{
    Set(c);
    target_->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush_, w);
}

void App::Circle(float cx, float cy, float radius, D2D1_COLOR_F c, bool fill, float stroke)
{
    Set(c);
    const auto e = D2D1::Ellipse(D2D1::Point2F(cx, cy), radius, radius);
    if (fill) target_->FillEllipse(e, brush_);
    else target_->DrawEllipse(e, brush_, stroke);
}

void App::DrawSwitch(bool value, float x, float y)
{
    // Compact 40x20 switch with a fully visible pill-shaped track.
    const float width = 40.0f;
    const float height = 20.0f;
    const float radius = 10.0f;

    const D2D1_COLOR_F track = value ? Col(0xF0194E) : Col(0x282E33);
    const D2D1_COLOR_F outline = value ? Col(0xFF5578) : Col(0x687078);

    Round(x, y, x + width, y + height, radius, track);
    RoundRect(x, y, x + width, y + height, radius, outline, 1.0f);

    const float knobX = value ? x + width - 10.0f : x + 10.0f;
    Circle(knobX, y + height * 0.5f, 7.2f, Col(0xF4F6F7), true);
    Circle(knobX, y + height * 0.5f, 7.2f,
           value ? Col(0xFFFFFF) : Col(0xC8CDD1), false, 0.9f);
}

void App::DrawGear(float cx, float cy, D2D1_COLOR_F c)
{
    Circle(cx, cy, 6.0f, c, false, 1.5f);
    Circle(cx, cy, 2.0f, c, false, 1.4f);
    for (int i = 0; i < 8; ++i)
    {
        const float a = 3.14159265f * 2.0f * i / 8.0f;
        const float x1 = cx + std::cos(a) * 7.0f;
        const float y1 = cy + std::sin(a) * 7.0f;
        const float x2 = cx + std::cos(a) * 9.0f;
        const float y2 = cy + std::sin(a) * 9.0f;
        Line(x1, y1, x2, y2, c, 1.5f);
    }
}

void App::DrawNavIcon(Page p, float x, float y, D2D1_COLOR_F c)
{
    // Clean monochrome vector icons. They stay white/bright and never depend
    // on icon fonts, so they look consistent on every Windows installation.
    const float cx = x + 14.0f;
    const float cy = y + 21.0f;
    const float stroke = 1.9f;

    Set(c);

    switch (p)
    {
    case Page::Visuals:
        // Modern eye: two smooth arcs with a small centered pupil.
        {
            ID2D1PathGeometry* geometry = nullptr;
            ID2D1GeometrySink* sink = nullptr;
            if (SUCCEEDED(factory_->CreatePathGeometry(&geometry)) &&
                SUCCEEDED(geometry->Open(&sink)))
            {
                sink->BeginFigure(D2D1::Point2F(cx - 10.5f, cy), D2D1_FIGURE_BEGIN_HOLLOW);
                sink->AddBezier(D2D1::BezierSegment(
                    D2D1::Point2F(cx - 5.5f, cy - 7.0f),
                    D2D1::Point2F(cx + 5.5f, cy - 7.0f),
                    D2D1::Point2F(cx + 10.5f, cy)));
                sink->AddBezier(D2D1::BezierSegment(
                    D2D1::Point2F(cx + 5.5f, cy + 7.0f),
                    D2D1::Point2F(cx - 5.5f, cy + 7.0f),
                    D2D1::Point2F(cx - 10.5f, cy)));
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                sink->Close();
                target_->DrawGeometry(geometry, brush_, stroke);
            }
            SafeRelease(sink);
            SafeRelease(geometry);
            Circle(cx, cy, 3.0f, c, true);
        }
        break;

    case Page::Players:
        // Rounded single-user silhouette.
        Circle(cx, cy - 5.5f, 4.0f, c, false, stroke);
        {
            ID2D1PathGeometry* geometry = nullptr;
            ID2D1GeometrySink* sink = nullptr;
            if (SUCCEEDED(factory_->CreatePathGeometry(&geometry)) &&
                SUCCEEDED(geometry->Open(&sink)))
            {
                sink->BeginFigure(D2D1::Point2F(cx - 8.5f, cy + 9.0f), D2D1_FIGURE_BEGIN_HOLLOW);
                sink->AddBezier(D2D1::BezierSegment(
                    D2D1::Point2F(cx - 8.0f, cy + 2.0f),
                    D2D1::Point2F(cx - 4.2f, cy + 0.5f),
                    D2D1::Point2F(cx, cy + 0.5f)));
                sink->AddBezier(D2D1::BezierSegment(
                    D2D1::Point2F(cx + 4.2f, cy + 0.5f),
                    D2D1::Point2F(cx + 8.0f, cy + 2.0f),
                    D2D1::Point2F(cx + 8.5f, cy + 9.0f)));
                sink->EndFigure(D2D1_FIGURE_END_OPEN);
                sink->Close();
                target_->DrawGeometry(geometry, brush_, stroke);
            }
            SafeRelease(sink);
            SafeRelease(geometry);
        }
        break;

    case Page::World:
        // Minimal globe with curved longitude and latitude strokes.
        Circle(cx, cy, 9.5f, c, false, stroke);
        Line(cx - 9.0f, cy, cx + 9.0f, cy, c, 1.45f);
        target_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), 4.6f, 9.5f), brush_, 1.45f);
        break;

    case Page::View:
        // Compact focus/preview icon using four corner brackets.
        Line(cx - 9, cy - 8, cx - 3, cy - 8, c, stroke);
        Line(cx - 9, cy - 8, cx - 9, cy - 2, c, stroke);
        Line(cx + 9, cy - 8, cx + 3, cy - 8, c, stroke);
        Line(cx + 9, cy - 8, cx + 9, cy - 2, c, stroke);
        Line(cx - 9, cy + 8, cx - 3, cy + 8, c, stroke);
        Line(cx - 9, cy + 8, cx - 9, cy + 2, c, stroke);
        Line(cx + 9, cy + 8, cx + 3, cy + 8, c, stroke);
        Line(cx + 9, cy + 8, cx + 9, cy + 2, c, stroke);
        Circle(cx, cy, 2.6f, c, true);
        break;

    case Page::Configs:
        // Sliders/tuning icon with rounded control points.
        Line(cx - 9, cy - 7, cx + 9, cy - 7, c, 1.6f);
        Line(cx - 9, cy,     cx + 9, cy,     c, 1.6f);
        Line(cx - 9, cy + 7, cx + 9, cy + 7, c, 1.6f);
        Circle(cx - 3.5f, cy - 7, 2.2f, c, true);
        Circle(cx + 4.0f, cy, 2.2f, c, true);
        Circle(cx - 1.0f, cy + 7, 2.2f, c, true);
        break;
    }
}

void App::Text(const std::wstring& value, float x, float y, float w, float h,
               D2D1_COLOR_F c, IDWriteTextFormat* format, DWRITE_TEXT_ALIGNMENT align)
{
    if (!format)
        format = text_;

    format->SetTextAlignment(align);
    Set(c);
    target_->DrawTextW(
        value.c_str(), static_cast<UINT32>(value.size()), format,
        D2D1::RectF(x, y, x + w, y + h), brush_, D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

bool App::Hover(float l, float t, float r, float b) const
{
    return Hit(static_cast<float>(mouse_.x), static_cast<float>(mouse_.y), l, t, r, b);
}

void App::DrawTopbar(float width)
{
    Fill(Ui::Sidebar, 0, width, Ui::Topbar, g_theme.top);
    Line(Ui::Sidebar, Ui::Topbar - 1, width, Ui::Topbar - 1, g_theme.border);

    // Clean top bar: breadcrumb labels were removed as requested.
    // The empty area can still be used to drag the borderless window.

    const float buttonTop = 18.0f;
    const float buttonSize = 34.0f;
    const float minLeft = width - 86.0f;
    const float closeLeft = width - 45.0f;

    const bool minHover = Hover(minLeft, buttonTop, minLeft + buttonSize, buttonTop + buttonSize);
    const bool closeHover = Hover(closeLeft, buttonTop, closeLeft + buttonSize, buttonTop + buttonSize);

    if (minHover)
        Round(minLeft, buttonTop, minLeft + buttonSize, buttonTop + buttonSize, 3.0f, g_theme.rowHover);
    if (closeHover)
        Round(closeLeft, buttonTop, closeLeft + buttonSize, buttonTop + buttonSize, 3.0f, Col(0x7A1730, 0.95f));

    // Vector minimize icon: never depends on an icon font.
    const D2D1_COLOR_F minColor = minHover ? g_theme.white : Col(0xD8DCE0);
    Line(minLeft + 10.0f, buttonTop + 18.0f,
         minLeft + 24.0f, buttonTop + 18.0f, minColor, 2.1f);

    // Vector close icon: two crisp diagonal lines instead of the text glyph ×.
    const D2D1_COLOR_F closeColor = closeHover ? g_theme.white : Col(0xD8DCE0);
    Line(closeLeft + 10.0f, buttonTop + 10.0f,
         closeLeft + 24.0f, buttonTop + 24.0f, closeColor, 2.1f);
    Line(closeLeft + 24.0f, buttonTop + 10.0f,
         closeLeft + 10.0f, buttonTop + 24.0f, closeColor, 2.1f);
}
void App::Tab(const std::wstring& label, bool& value, float x, float y, float w)
{
    bool hover = Hover(x, y, x + w, y + 35);
    if (!colorPickerOpen_ && hover && pressed_)
    {
        state_.enemy = false;
        state_.local = false;
        state_.ally = false;
        value = true;
    }

    if (value || hover)
        Round(x, y, x + w, y + 35, 2.0f, value ? Col(0x14181B, 0.96f) : g_theme.rowHover);

    Text(label, x, y, w, 35, value ? g_theme.text : g_theme.sub, tiny_, DWRITE_TEXT_ALIGNMENT_CENTER);
}

void App::NavItem(Page p, const std::wstring& glyph, const std::wstring& label,
                  float x, float y, bool child)
{
    bool selected = state_.page == p;
    bool hover = Hover(0, y, Ui::Sidebar, y + 44);

    if (!colorPickerOpen_ && hover && pressed_)
    {
        state_.page = p;
        openCombo_.clear();
    }

    if (selected)
        Fill(0, y, Ui::Sidebar, y + 44, Col(0x111518, 0.98f));
    else if (hover)
        Fill(0, y, Ui::Sidebar, y + 44, g_theme.rowHover);

    const D2D1_COLOR_F iconColor = selected ? g_theme.white : (hover ? g_theme.white : Col(0xE2E6EA));
    DrawNavIcon(p, x, y, iconColor);
    Text(label, x + 40, y, Ui::Sidebar - x - 58, 44,
         selected ? g_theme.white : (hover ? g_theme.white : Col(0xB8BDC2)), small_);
    if (selected)
        Round(Ui::Sidebar - 14, y + 11, Ui::Sidebar - 10, y + 33, 2.0f, g_theme.red);
}

void App::DrawSidebar(float height)
{
    Fill(0, 0, Ui::Sidebar, height, Col(0x090C0F, 0.995f));
    Line(Ui::Sidebar - 1, 0, Ui::Sidebar - 1, height, Col(0x2A3036));

    // High-contrast branding. Every part is drawn at full opacity so the logo
    // and title stay readable even over a bright blurred desktop background.
    const D2D1_COLOR_F logoWhite = Col(0xFFFFFF);
    const D2D1_COLOR_F logoGrey = Col(0xBFC5CA);
    const D2D1_COLOR_F logoRed = Col(0xFF1F4F);

    // Slight dark backing area behind branding for guaranteed contrast.
    Fill(0, 0, Ui::Sidebar, 76.0f, Col(0x080A0D, 1.0f));
    Line(0, 75.0f, Ui::Sidebar, 75.0f, Col(0x171C20));

    // Outer triangular mark.
    Line(28.0f, 54.0f, 49.0f, 13.0f, logoWhite, 3.4f);
    Line(49.0f, 13.0f, 71.0f, 54.0f, logoWhite, 3.4f);
    Line(71.0f, 54.0f, 28.0f, 54.0f, logoWhite, 3.4f);
    // Inner triangular mark.
    Line(39.0f, 45.0f, 49.0f, 26.0f, logoGrey, 2.3f);
    Line(49.0f, 26.0f, 59.0f, 45.0f, logoGrey, 2.3f);
    Line(59.0f, 45.0f, 39.0f, 45.0f, logoGrey, 2.3f);
    // Red underline and center accent.
    Line(30.0f, 59.0f, 69.0f, 59.0f, logoRed, 2.8f);
    Line(45.0f, 42.0f, 49.0f, 34.0f, logoRed, 2.0f);
    Line(49.0f, 34.0f, 53.0f, 42.0f, logoRed, 2.0f);

    // Renamed title. Change these three strings whenever you want a new brand.
    Text(L"SPECTRA", 82.0f, 10.0f, 91.0f, 29.0f, logoRed, logo_);
    Text(L"UI", 173.0f, 10.0f, 38.0f, 29.0f, logoWhite, logo_);
    Text(L"DIRECT2D INTERFACE", 82.0f, 39.0f, 158.0f, 20.0f, Col(0xDCE0E4), tiny_);
    Fill(82.0f, 61.0f, 205.0f, 78.0f, Col(0xFF1F4F));
    Text(L"REV 26 • COMBAT UI", 88.0f, 60.0f, 112.0f, 18.0f, Col(0xFFFFFF), tiny_);

    // Navigation is top-aligned directly below the branding instead of
    // vertically centered. Every item keeps the exact same x-position.
    constexpr float itemHeight = 44.0f;
    constexpr float itemGap = 4.0f;
    float navY = 104.0f;
    const float navX = 18.0f;

    NavItem(Page::Visuals, L"", L"Visuals", navX, navY, false); navY += itemHeight + itemGap;
    NavItem(Page::Players, L"", L"Aimbot", navX, navY, false); navY += itemHeight + itemGap;
    NavItem(Page::World, L"", L"World", navX, navY, false); navY += itemHeight + itemGap;
    NavItem(Page::View, L"", L"View", navX, navY, false); navY += itemHeight + itemGap;
    NavItem(Page::Configs, L"", L"Configs", navX, navY, false);

    Footer(height);
}
void App::Footer(float height)
{
    // Minimal version text only. The old EXE badge/card has been removed.
    Text(L"VERSION 2.6.0  •  COMBAT INTERFACE", 18.0f, height - 31.0f,
         Ui::Sidebar - 36.0f, 18.0f, Col(0x666E75), tiny_);
}

void App::SectionLabel(const std::wstring& label, float x, float y, float w)
{
    Text(label, x, y, w, 24, g_theme.sub, medium_);
}

void App::RowBase(float x, float y, float w, float h, bool hover)
{
    Fill(x, y, x + w, y + h, hover ? g_theme.rowHover : g_theme.row);
    Line(x, y + h - 1, x + w, y + h - 1, Col(0x14191D, 0.9f));
}

void App::Toggle(const std::wstring& label, bool& value, float x, float y, float w, float h)
{
    bool hover = Hover(x, y, x + w, y + h);
    if (!colorPickerOpen_ && openCombo_.empty() && hover && pressed_)
        value = !value;

    RowBase(x, y, w, h, hover);
    Text(label, x + 16, y, w - 75, h, g_theme.text, small_);

    DrawSwitch(value, x + w - 53.0f, y + h / 2.0f - 10.0f);
}

void App::ColorToggle(const std::wstring& label, D2D1_COLOR_F& swatch,
                      bool& value, float x, float y, float w)
{
    const float h = 42.0f;
    const float switchX = x + w - 53.0f;

    // Compact swatch sits immediately to the left of the toggle.
    const float swatchW = 22.0f;
    const float swatchH = 14.0f;
    const float swatchR = switchX - 10.0f;
    const float swatchL = swatchR - swatchW;
    const float swatchT = y + (h - swatchH) * 0.5f;
    const float swatchB = swatchT + swatchH;

    const bool rowHover = Hover(x, y, x + w, y + h);
    const bool swatchHover = Hover(swatchL - 3.0f, swatchT - 3.0f,
                                   swatchR + 3.0f, swatchB + 3.0f);

    if (!colorPickerOpen_ && openCombo_.empty() && pressed_)
    {
        if (swatchHover)
            OpenColorPicker(swatch);
        else if (rowHover)
            value = !value;
    }

    RowBase(x, y, w, h, rowHover && !colorPickerOpen_);

    // More room for long labels; no hex text inside the row, so nothing overlaps.
    Text(label, x + 12.0f, y, swatchL - x - 18.0f, h,
         g_theme.text, small_);

    Round(swatchL - 2.0f, swatchT - 2.0f, swatchR + 2.0f, swatchB + 2.0f,
          3.0f, swatchHover ? Col(0x394149) : Col(0x242A2F));
    Round(swatchL, swatchT, swatchR, swatchB, 2.0f, swatch);
    RoundRect(swatchL, swatchT, swatchR, swatchB, 2.0f,
              swatchHover ? g_theme.white : Col(0x596168), 1.0f);

    DrawSwitch(value, switchX, y + h * 0.5f - 10.0f);
}

std::wstring App::HexColor(const D2D1_COLOR_F& color) const
{
    const int r = std::clamp(static_cast<int>(std::round(color.r * 255.0f)), 0, 255);
    const int g = std::clamp(static_cast<int>(std::round(color.g * 255.0f)), 0, 255);
    const int b = std::clamp(static_cast<int>(std::round(color.b * 255.0f)), 0, 255);

    std::wstringstream stream;
    stream << L'#' << std::uppercase << std::hex << std::setfill(L'0')
           << std::setw(2) << r << std::setw(2) << g << std::setw(2) << b;
    return stream.str();
}

void App::OpenColorPicker(D2D1_COLOR_F& color)
{
    pickerColor_ = &color;
    pickerOriginal_ = color;
    colorPickerOpen_ = true;
    activeRgbSlider_ = -1;
    openCombo_.clear();
}

void App::RgbSlider(const std::wstring& label, float& channel, int sliderId,
                    float x, float y, float w)
{
    const float trackY = y + 24.0f;
    const bool hover = Hover(x, trackY - 9.0f, x + w, trackY + 9.0f);

    if (hover && pressed_)
        activeRgbSlider_ = sliderId;

    if (activeRgbSlider_ == sliderId && down_)
        channel = std::clamp((static_cast<float>(mouse_.x) - x) / w, 0.0f, 1.0f);

    const int value = std::clamp(static_cast<int>(std::round(channel * 255.0f)), 0, 255);
    Text(label, x, y, 50.0f, 18.0f, g_theme.text, small_);
    Text(std::to_wstring(value), x + w - 52.0f, y, 52.0f, 18.0f,
         g_theme.white, tiny_, DWRITE_TEXT_ALIGNMENT_TRAILING);

    Round(x, trackY - 3.0f, x + w, trackY + 3.0f, 3.0f, Col(0x272D32));
    Round(x, trackY - 3.0f, x + w * channel, trackY + 3.0f, 3.0f,
          sliderId == 0 ? Col(0xFF315F) : sliderId == 1 ? Col(0x36D47B) : Col(0x358BFF));
    const float knobX = x + w * channel;
    Circle(knobX, trackY, 6.0f, g_theme.white, true);
    Circle(knobX, trackY, 6.0f, Col(0x8E969D), false, 1.0f);
}

void App::DrawColorPicker(float width, float height)
{
    if (!colorPickerOpen_ || !pickerColor_)
        return;

    Fill(Ui::Sidebar, Ui::Topbar, width, height, Col(0x000000, 0.58f));

    // Smaller, more compact RGB picker.
    const float boxW = 304.0f;
    const float boxH = 250.0f;
    const float x = Ui::Sidebar + (width - Ui::Sidebar - boxW) * 0.5f;
    const float y = Ui::Topbar + (height - Ui::Topbar - boxH) * 0.5f;

    Round(x, y, x + boxW, y + boxH, 7.0f, Col(0x0D1114, 0.995f));
    RoundRect(x, y, x + boxW, y + boxH, 7.0f, Col(0x343B41), 1.0f);
    Fill(x, y, x + boxW, y + 46.0f, Col(0x11161A));
    Line(x, y + 46.0f, x + boxW, y + 46.0f, Col(0x252C31));

    Text(L"RGB Color", x + 18.0f, y, boxW - 36.0f, 46.0f,
         g_theme.white, medium_);

    Round(x + 20.0f, y + 62.0f, x + 76.0f, y + 98.0f, 4.0f, *pickerColor_);
    RoundRect(x + 20.0f, y + 62.0f, x + 76.0f, y + 98.0f, 4.0f,
              Col(0xFFFFFF, 0.42f), 1.0f);
    Text(HexColor(*pickerColor_), x + 92.0f, y + 57.0f, boxW - 112.0f, 42.0f,
         g_theme.white, bold_);

    const float sx = x + 20.0f;
    const float sw = boxW - 40.0f;
    RgbSlider(L"Red", pickerColor_->r, 0, sx, y + 108.0f, sw);
    RgbSlider(L"Green", pickerColor_->g, 1, sx, y + 142.0f, sw);
    RgbSlider(L"Blue", pickerColor_->b, 2, sx, y + 176.0f, sw);

    const float buttonY = y + boxH - 38.0f;
    const float cancelL = x + boxW - 152.0f;
    const float applyL = x + boxW - 78.0f;
    const bool cancelHover = Hover(cancelL, buttonY, cancelL + 64.0f, buttonY + 26.0f);
    const bool applyHover = Hover(applyL, buttonY, applyL + 58.0f, buttonY + 26.0f);

    Round(cancelL, buttonY, cancelL + 64.0f, buttonY + 26.0f, 3.0f,
          cancelHover ? g_theme.rowHover : Col(0x181D21));
    RoundRect(cancelL, buttonY, cancelL + 64.0f, buttonY + 26.0f, 3.0f, Col(0x343B41));
    Text(L"Cancel", cancelL, buttonY, 64.0f, 26.0f, g_theme.text, tiny_, DWRITE_TEXT_ALIGNMENT_CENTER);

    Round(applyL, buttonY, applyL + 58.0f, buttonY + 26.0f, 3.0f,
          applyHover ? Col(0xFF315F) : g_theme.red);
    Text(L"Apply", applyL, buttonY, 58.0f, 26.0f, g_theme.white, tiny_, DWRITE_TEXT_ALIGNMENT_CENTER);

    if (pressed_ && cancelHover)
    {
        *pickerColor_ = pickerOriginal_;
        colorPickerOpen_ = false;
        pickerColor_ = nullptr;
        activeRgbSlider_ = -1;
    }
    else if (pressed_ && applyHover)
    {
        colorPickerOpen_ = false;
        pickerColor_ = nullptr;
        activeRgbSlider_ = -1;
    }
}

void App::GearToggle(const std::wstring& label, bool& value, float x, float y, float w,
                     D2D1_COLOR_F* color)
{
    const bool hover = Hover(x, y, x + w, y + 42);
    const float gearL = x + w - 101.0f;
    const float gearR = x + w - 70.0f;
    const bool gearHover = Hover(gearL, y + 5.0f, gearR, y + 37.0f);

    if (!colorPickerOpen_ && !gearPopupOpen_ && openCombo_.empty() && pressed_)
    {
        if (gearHover)
        {
            gearPopupOpen_ = true;
            gearPopupTitle_ = label;
            gearPopupValue_ = &value;
            gearPopupColor_ = color;
        }
        else if (hover)
        {
            value = !value;
        }
    }

    RowBase(x, y, w, 42, hover);
    Text(label, x + 16, y, w - 118, 42, g_theme.text, small_);

    if (gearHover)
        Round(gearL, y + 6.0f, gearR, y + 36.0f, 4.0f, Col(0x252B30));
    DrawGear((gearL + gearR) * 0.5f, y + 21.0f,
             gearHover ? g_theme.white : g_theme.sub);
    DrawSwitch(value, x + w - 53.0f, y + 11.0f);
}

void App::AimbotGearToggle(const std::wstring& label, bool& value, int mode,
                              float x, float y, float w)
{
    const float h = 42.0f;
    const float switchX = x + w - 53.0f;
    const float gearL = switchX - 38.0f;
    const float gearR = switchX - 8.0f;
    const bool rowHover = Hover(x, y, x + w, y + h);
    const bool gearHover = Hover(gearL, y + 6.0f, gearR, y + 36.0f);
    const bool switchHover = Hover(switchX - 4.0f, y + 6.0f, x + w, y + 36.0f);

    if (!colorPickerOpen_ && !gearPopupOpen_ && openCombo_.empty() && pressed_)
    {
        if (gearHover)
        {
            gearPopupOpen_ = true;
            gearPopupTitle_ = label;
            gearPopupValue_ = &value;
            gearPopupColor_ = nullptr;
            gearMode_ = mode;

            // Give every variable a sensible default advanced value.
            switch (mode)
            {
            case 1: gearStrength_ = 100.0f; break; // master enable
            case 2: gearStrength_ = 80.0f; break;  // silent aim
            case 3: gearStrength_ = 65.0f; break;  // visibility
            case 4: gearStrength_ = 72.0f; break;  // recoil control
            case 5: gearStrength_ = 18.0f; break;  // fire delay
            case 6: gearStrength_ = 35.0f; break;  // scope delay
            case 7: gearStrength_ = 70.0f; break;  // target lock
            case 8: gearStrength_ = 55.0f; break;  // damage override
            case 9: gearStrength_ = 45.0f; break;  // body aim
            case 10: gearStrength_ = 60.0f; break; // safe point
            case 11: gearStrength_ = 50.0f; break; // smoke check
            default: gearStrength_ = 65.0f; break;
            }
        }
        else if (switchHover || rowHover)
        {
            value = !value;
        }
    }

    RowBase(x, y, w, h, rowHover);
    Text(label, x + 16.0f, y, gearL - x - 22.0f, h, g_theme.text, small_);

    if (gearHover)
        Round(gearL, y + 6.0f, gearR, y + 36.0f, 4.0f, Col(0x252B30));
    DrawGear((gearL + gearR) * 0.5f, y + 21.0f,
             gearHover ? g_theme.white : g_theme.sub);
    DrawSwitch(value, switchX, y + 11.0f);
}

void App::Combo(const std::wstring& id, const std::wstring& label,
                const std::vector<std::wstring>& items,
                int& selected, float x, float y, float w)
{
    selected = std::clamp(selected, 0, static_cast<int>(items.size()) - 1);
    const bool hover = Hover(x, y, x + w, y + 42.0f);
    const bool open = openCombo_ == id;

    // Every dropdown has a unique id, so two rows with the same visible label
    // can never open together.
    if (!colorPickerOpen_ && hover && pressed_ &&
        (openCombo_.empty() || open))
    {
        openCombo_ = open ? L"" : id;
    }

    RowBase(x, y, w, 42.0f, hover || open);
    Text(label, x + 16.0f, y, w * 0.48f, 42.0f, g_theme.text, small_);
    Text(items[selected], x + w * 0.51f, y, w * 0.40f, 42.0f,
         g_theme.sub, tiny_, DWRITE_TEXT_ALIGNMENT_TRAILING);

    const D2D1_COLOR_F arrow = open ? g_theme.white : g_theme.sub;
    Line(x + w - 25.0f, y + 18.0f, x + w - 20.0f, y + 23.0f, arrow, 1.6f);
    Line(x + w - 20.0f, y + 23.0f, x + w - 15.0f, y + 18.0f, arrow, 1.6f);

    if (open)
    {
        dropdownOverlayReady_ = true;
        dropdownOverlayId_ = id;
        dropdownOverlayItems_ = items;
        dropdownOverlaySelection_ = &selected;
        dropdownOverlayX_ = x + w * 0.48f;
        dropdownOverlayY_ = y + 42.0f;
        dropdownOverlayW_ = w * 0.52f;
    }
}

void App::ValueSlider(const std::wstring& label, int& value, int minimum, int maximum,
                      int step, const std::wstring& suffix, float x, float y, float w)
{
    const float h = 52.0f;
    const float trackL = x + 15.0f;
    const float trackR = x + w - 15.0f;
    const float trackY = y + 36.0f;
    const bool hover = Hover(x, y, x + w, y + h);
    const bool trackHover = Hover(trackL, trackY - 10.0f, trackR, trackY + 10.0f);

    value = std::clamp(value, minimum, maximum);
    RowBase(x, y, w, h, hover);
    Text(label, x + 15.0f, y + 2.0f, w - 100.0f, 25.0f, g_theme.text, small_);
    Text(std::to_wstring(value) + suffix, x + w - 86.0f, y + 2.0f, 71.0f, 25.0f,
         g_theme.white, tiny_, DWRITE_TEXT_ALIGNMENT_TRAILING);

    const float range = static_cast<float>(maximum - minimum);
    float t = range > 0.0f ? static_cast<float>(value - minimum) / range : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);

    Round(trackL, trackY - 2.5f, trackR, trackY + 2.5f, 1.0f, Col(0x282E33));
    Round(trackL, trackY - 2.5f, trackL + (trackR - trackL) * t,
          trackY + 2.5f, 1.0f, g_theme.red);

    const float knobX = trackL + (trackR - trackL) * t;
    Round(knobX - 3.0f, trackY - 7.0f, knobX + 3.0f, trackY + 7.0f,
          1.0f, trackHover || down_ ? g_theme.white : Col(0xE3E6E8));

    if (!colorPickerOpen_ && openCombo_.empty() && down_ && trackHover)
    {
        float raw = (static_cast<float>(mouse_.x) - trackL) / (trackR - trackL);
        raw = std::clamp(raw, 0.0f, 1.0f);
        int next = minimum + static_cast<int>(std::round(raw * range));
        if (step > 1)
            next = minimum + static_cast<int>(std::round(static_cast<float>(next - minimum) / step)) * step;
        value = std::clamp(next, minimum, maximum);
    }
}

void App::DrawDropdownOverlay()
{
    if (!dropdownOverlayReady_ || dropdownOverlaySelection_ == nullptr ||
        dropdownOverlayItems_.empty())
        return;

    const float itemH = 32.0f;
    const float x = dropdownOverlayX_;
    const float y = dropdownOverlayY_;
    const float w = dropdownOverlayW_;
    const float h = itemH * static_cast<float>(dropdownOverlayItems_.size());

    // Opaque backing plus a subtle shadow keeps the menu above every row/card.
    Round(x + 4.0f, y + 5.0f, x + w + 4.0f, y + h + 5.0f,
          3.0f, Col(0x000000, 0.42f));
    Fill(x, y, x + w, y + h, Col(0x0B0F12, 1.0f));
    Rect(x, y, x + w, y + h, Col(0x343B41), 1.0f);

    for (size_t i = 0; i < dropdownOverlayItems_.size(); ++i)
    {
        const float iy = y + itemH * static_cast<float>(i);
        const bool itemHover = Hover(x, iy, x + w, iy + itemH);

        if (itemHover)
            Fill(x + 1.0f, iy + 1.0f, x + w - 1.0f, iy + itemH - 1.0f,
                 g_theme.rowHover);

        if (static_cast<int>(i) == *dropdownOverlaySelection_)
            Fill(x + 1.0f, iy + 7.0f, x + 3.0f, iy + itemH - 7.0f,
                 g_theme.red);

        Text(dropdownOverlayItems_[i], x + 12.0f, iy, w - 24.0f, itemH,
             static_cast<int>(i) == *dropdownOverlaySelection_
                 ? g_theme.red
                 : (itemHover ? g_theme.white : g_theme.text),
             tiny_);

        if (!colorPickerOpen_ && itemHover && pressed_)
        {
            *dropdownOverlaySelection_ = static_cast<int>(i);
            openCombo_.clear();
        }
    }
}

bool App::ActionButton(const std::wstring& label, float x, float y, float w, bool accent, bool danger)
{
    const float h = 38.0f;
    const bool hover = Hover(x, y, x + w, y + h);
    D2D1_COLOR_F bg = Col(0x14191D);
    D2D1_COLOR_F border = Col(0x30373D);
    if (accent) { bg = hover ? Col(0xFF315F) : g_theme.red; border = Col(0xFF5578); }
    else if (danger) { bg = hover ? Col(0x6E1B2E) : Col(0x35151D); border = Col(0x8E2941); }
    else if (hover) bg = g_theme.rowHover;
    Round(x, y, x + w, y + h, 1.0f, bg);
    RoundRect(x, y, x + w, y + h, 1.0f, border);
    Text(label, x, y, w, h, accent ? g_theme.white : g_theme.text, small_, DWRITE_TEXT_ALIGNMENT_CENTER);
    return !colorPickerOpen_ && openCombo_.empty() && hover && pressed_;
}

std::filesystem::path App::ConfigDirectory() const
{
    wchar_t appData[MAX_PATH]{};
    const DWORD n = GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH);
    std::filesystem::path dir = n
        ? std::filesystem::path(appData) / L"SpectraUI" / L"Configs"
        : std::filesystem::current_path() / L"Configs";

    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

std::wstring App::SanitizeConfigName(const std::wstring& name) const
{
    std::wstring clean;
    clean.reserve(name.size());

    for (wchar_t ch : name)
    {
        const bool invalid = ch < 32 || ch == L'<' || ch == L'>' || ch == L':' ||
            ch == L'"' || ch == L'/' || ch == L'\\' || ch == L'|' ||
            ch == L'?' || ch == L'*';
        if (!invalid)
            clean.push_back(ch);
    }

    while (!clean.empty() && (clean.back() == L' ' || clean.back() == L'.'))
        clean.pop_back();
    while (!clean.empty() && clean.front() == L' ')
        clean.erase(clean.begin());

    if (clean.size() > 32)
        clean.resize(32);
    return clean;
}

std::filesystem::path App::ConfigPath(const std::wstring& name) const
{
    return ConfigDirectory() / (SanitizeConfigName(name) + L".bin");
}

void App::RefreshConfigs()
{
    configNames_.clear();
    std::error_code ec;
    const auto dir = ConfigDirectory();

    for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
    {
        if (ec)
            break;
        if (!entry.is_regular_file(ec))
            continue;
        if (entry.path().extension() == L".bin")
            configNames_.push_back(entry.path().stem().wstring());
    }

    std::sort(configNames_.begin(), configNames_.end(),
        [](const std::wstring& a, const std::wstring& b)
        {
            std::wstring al = a;
            std::wstring bl = b;
            std::transform(al.begin(), al.end(), al.begin(), ::towlower);
            std::transform(bl.begin(), bl.end(), bl.begin(), ::towlower);
            return al < bl;
        });

    if (configNames_.empty())
        selectedConfig_ = 0;
    else
        selectedConfig_ = std::clamp(selectedConfig_, 0,
            static_cast<int>(configNames_.size()) - 1);
}

bool App::SaveConfig(const std::wstring& name)
{
    const std::wstring clean = SanitizeConfigName(name);
    if (clean.empty())
        return false;

    std::ofstream file(ConfigPath(clean), std::ios::binary | std::ios::trunc);
    if (!file)
        return false;

    const uint32_t magic = 0x53504332;
    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char*>(&state_), sizeof(state_));
    return file.good();
}

bool App::LoadConfig(const std::wstring& name)
{
    const std::wstring clean = SanitizeConfigName(name);
    if (clean.empty())
        return false;

    std::ifstream file(ConfigPath(clean), std::ios::binary);
    if (!file)
        return false;

    uint32_t magic = 0;
    State loaded{};
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&loaded), sizeof(loaded));
    if (!file.good() || magic != 0x53504332)
        return false;

    state_ = loaded;
    return true;
}

bool App::DeleteConfig(const std::wstring& name)
{
    const std::wstring clean = SanitizeConfigName(name);
    if (clean.empty())
        return false;

    std::error_code ec;
    return std::filesystem::remove(ConfigPath(clean), ec);
}

void App::DrawGearPopup(float width, float height)
{
    if (!gearPopupOpen_ || gearPopupValue_ == nullptr)
        return;

    Fill(Ui::Sidebar, Ui::Topbar, width, height, Col(0x000000, 0.62f));

    const float boxW = 344.0f;
    const float boxH = gearPopupColor_ ? 282.0f : 230.0f;
    const float x = Ui::Sidebar + (width - Ui::Sidebar - boxW) * 0.5f;
    const float y = Ui::Topbar + (height - Ui::Topbar - boxH) * 0.5f;

    Round(x, y, x + boxW, y + boxH, 9.0f, Col(0x0C1013, 0.998f));
    RoundRect(x, y, x + boxW, y + boxH, 9.0f, Col(0x3A434A), 1.0f);
    Fill(x, y, x + boxW, y + 50.0f, Col(0x12171B));
    Fill(x, y, x + 3.0f, y + 50.0f, g_theme.red);
    Line(x, y + 50.0f, x + boxW, y + 50.0f, Col(0x252C31));

    Text(gearPopupTitle_ + L" settings", x + 18.0f, y,
         boxW - 36.0f, 50.0f, g_theme.white, medium_);

    const float rowX = x + 16.0f;
    const float rowW = boxW - 32.0f;

    RowBase(rowX, y + 62.0f, rowW, 42.0f,
            Hover(rowX, y + 62.0f, rowX + rowW, y + 104.0f));
    Text(L"Enabled", rowX + 14.0f, y + 62.0f, rowW - 82.0f, 42.0f,
         g_theme.text, small_);
    const bool enabledHover = Hover(rowX, y + 62.0f, rowX + rowW, y + 104.0f);
    if (!colorPickerOpen_ && pressed_ && enabledHover)
        *gearPopupValue_ = !*gearPopupValue_;
    DrawSwitch(*gearPopupValue_, rowX + rowW - 53.0f, y + 73.0f);

    float intensityY = y + 120.0f;

    if (gearPopupColor_)
    {
        const float colorY = y + 112.0f;
        const bool colorHover = Hover(rowX, colorY, rowX + rowW, colorY + 46.0f);
        RowBase(rowX, colorY, rowW, 46.0f, colorHover);
        Text(L"Element color", rowX + 14.0f, colorY, 150.0f, 46.0f,
             g_theme.text, small_);

        const float swW = 42.0f;
        const float swH = 18.0f;
        const float swR = rowX + rowW - 14.0f;
        const float swL = swR - swW;
        const float swT = colorY + (46.0f - swH) * 0.5f;
        const float swB = swT + swH;

        Round(swL - 3.0f, swT - 3.0f, swR + 3.0f, swB + 3.0f,
              4.0f, colorHover ? Col(0x46515A) : Col(0x252C32));
        Round(swL, swT, swR, swB, 3.0f, *gearPopupColor_);
        RoundRect(swL, swT, swR, swB, 3.0f,
                  colorHover ? g_theme.white : Col(0x66717A), 1.0f);

        Text(HexColor(*gearPopupColor_), swL - 82.0f, colorY,
             72.0f, 46.0f, g_theme.sub, tiny_,
             DWRITE_TEXT_ALIGNMENT_TRAILING);

        if (!colorPickerOpen_ && pressed_ && colorHover)
            OpenColorPicker(*gearPopupColor_);

        intensityY = y + 176.0f;
    }

    std::wstring advancedLabel = L"Intensity";
    std::wstring suffix = L"%";
    std::wstring hint = L"Controls the strength of this visual option.";

    switch (gearMode_)
    {
    case 1: advancedLabel = L"Activation strength"; hint = L"Master aimbot activation and response level."; break;
    case 2: advancedLabel = L"Silent correction"; hint = L"How strongly silent correction is applied."; break;
    case 3: advancedLabel = L"Visibility strictness"; hint = L"Higher values require a cleaner visibility check."; break;
    case 4: advancedLabel = L"Recoil compensation"; hint = L"Amount of recoil removed while aiming."; break;
    case 5: advancedLabel = L"Fire delay"; suffix = L" ms"; hint = L"Delay before automatic firing is triggered."; break;
    case 6: advancedLabel = L"Scope delay"; suffix = L" ms"; hint = L"Delay before automatic scope activation."; break;
    case 7: advancedLabel = L"Lock strength"; hint = L"How strongly the current target is retained."; break;
    case 8: advancedLabel = L"Override threshold"; suffix = L" dmg"; hint = L"Damage threshold used while the override key is held."; break;
    case 9: advancedLabel = L"Body preference"; hint = L"Higher values prefer body hitboxes more often."; break;
    case 10: advancedLabel = L"Safe-point preference"; hint = L"Higher values prefer safer hitbox intersections."; break;
    case 11: advancedLabel = L"Smoke tolerance"; hint = L"Controls how strict the smoke obstruction check is."; break;
    case 20: advancedLabel = L"Crosshair size"; hint = L"Changes the size of the live menu crosshair."; break;
    case 21: advancedLabel = L"Notification duration"; suffix = L"%"; hint = L"Controls how long interface notifications remain visible."; break;
    case 22: advancedLabel = L"Panel opacity"; hint = L"Controls the keybind-list panel opacity."; break;
    case 23: advancedLabel = L"Panel opacity"; hint = L"Controls the spectator-list panel opacity."; break;
    case 24: advancedLabel = L"Blur strength"; hint = L"Controls the visual strength of the background blur."; break;
    case 25: advancedLabel = L"Capture protection"; hint = L"Controls capture exclusion for supported methods."; break;
    case 26: advancedLabel = L"Particle amount"; hint = L"Controls the amount of animated background particles."; break;
    case 27: advancedLabel = L"Position persistence"; hint = L"Controls how strongly the menu position is persisted."; break;
    case 28: advancedLabel = L"Animation amount"; hint = L"Controls interface pulse and movement intensity."; break;
    default: break;
    }

    Text(advancedLabel, rowX, intensityY, rowW - 74.0f, 22.0f,
         g_theme.sub, tiny_);
    Text(std::to_wstring(static_cast<int>(std::round(gearStrength_))) + suffix,
         rowX + rowW - 74.0f, intensityY, 74.0f, 22.0f,
         g_theme.text, tiny_, DWRITE_TEXT_ALIGNMENT_TRAILING);
    Text(hint, rowX, intensityY + 53.0f, rowW, 20.0f,
         g_theme.faint, tiny_);

    const float sx = rowX;
    const float sy = intensityY + 32.0f;
    const float sw = rowW;
    Round(sx, sy - 3.0f, sx + sw, sy + 3.0f, 3.0f, Col(0x262D32));
    Round(sx, sy - 3.0f, sx + sw * gearStrength_ / 100.0f,
          sy + 3.0f, 3.0f, g_theme.red);
    Circle(sx + sw * gearStrength_ / 100.0f, sy, 6.0f,
           g_theme.white, true);
    Circle(sx + sw * gearStrength_ / 100.0f, sy, 6.0f,
           Col(0x77818A), false, 1.0f);

    if (!colorPickerOpen_ && down_ &&
        Hover(sx, sy - 11.0f, sx + sw, sy + 11.0f))
    {
        gearStrength_ = std::clamp(
            (static_cast<float>(mouse_.x) - sx) / sw * 100.0f,
            0.0f, 100.0f);
    }

    const float buttonY = y + boxH - 46.0f;
    if (!colorPickerOpen_ &&
        ActionButton(L"Done", x + boxW - 92.0f, buttonY, 72.0f, true))
    {
        gearPopupOpen_ = false;
        gearPopupValue_ = nullptr;
        gearPopupColor_ = nullptr;
        gearMode_ = 0;
    }
}

void App::DrawEmbeddedPreview(float x, float y, float w, float h)
{
    // Full compact target preview. The floating damage indicator itself remains
    // number-only: no chip, no border, no background and no extra words.
    const float panelH = std::max(178.0f, h);

    Round(x, y, x + w, y + panelH, 7.0f, Col(0x0A0E11));
    RoundRect(x, y, x + w, y + panelH, 7.0f, Col(0x2D353B), 1.0f);

    Fill(x, y, x + w, y + 32.0f, Col(0x101519));
    Line(x, y + 32.0f, x + w, y + 32.0f, Col(0x22292E));
    Text(L"TARGET PREVIEW", x + 12.0f, y, w - 24.0f, 32.0f,
         g_theme.sub, tiny_);

    const ULONGLONG now = GetTickCount64();
    if (previewLastDamageTick_ == 0)
        previewLastDamageTick_ = now;

    // Loop through damage states so the health bar and floating number animate.
    if (now - previewLastDamageTick_ >= 2200ULL)
    {
        previewPreviousHp_ = previewTargetHp_;

        if (previewTargetHp_ > 75.0f)
            previewTargetHp_ = 68.0f;
        else if (previewTargetHp_ > 45.0f)
            previewTargetHp_ = 42.0f;
        else if (previewTargetHp_ > 20.0f)
            previewTargetHp_ = 18.0f;
        else
            previewTargetHp_ = 100.0f;

        previewDamageTextTimer_ =
            previewTargetHp_ < previewPreviousHp_ ? 1.35f : 0.0f;
        previewLastDamageTick_ = now;
    }

    const float hpSpeed = previewTargetHp_ < previewDisplayedHp_ ? 0.085f : 0.045f;
    previewDisplayedHp_ += (previewTargetHp_ - previewDisplayedHp_) * hpSpeed;

    if (std::fabs(previewDisplayedHp_ - previewTargetHp_) < 0.08f)
        previewDisplayedHp_ = previewTargetHp_;

    if (previewDamageTextTimer_ > 0.0f)
        previewDamageTextTimer_ = std::max(0.0f, previewDamageTextTimer_ - 0.016f);

    const float hp = std::clamp(previewDisplayedHp_ / 100.0f, 0.0f, 1.0f);
    const D2D1_COLOR_F hpColor = hp > 0.60f ? state_.healthHighColor
                                 : hp > 0.30f ? state_.healthMidColor
                                              : state_.healthLowColor;

    const float stageL = x + 12.0f;
    const float stageR = x + w - 12.0f;
    const float stageT = y + 42.0f;
    const float stageB = y + panelH - 12.0f;

    Round(stageL, stageT, stageR, stageB, 5.0f, Col(0x0D1216));
    RoundRect(stageL, stageT, stageR, stageB, 5.0f, Col(0x232B31), 1.0f);

    const float targetH = std::max(92.0f, stageB - stageT - 36.0f);
    const float targetW = std::min(74.0f, (stageR - stageL) * 0.30f);
    const float bx = stageL + (stageR - stageL - targetW) * 0.5f;
    const float by = stageT + 22.0f;

    if (state_.name)
    {
        Text(L"TARGET_01", bx - 28.0f, by - 21.0f,
             targetW + 56.0f, 18.0f, state_.nameColor,
             tiny_, DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    if (state_.box)
    {
        const D2D1_COLOR_F boxCol = hp <= 0.30f ? hpColor : state_.boxColor;
        const float corner = 14.0f;
        const float thick = 1.7f;

        Line(bx, by, bx + corner, by, boxCol, thick);
        Line(bx, by, bx, by + corner, boxCol, thick);
        Line(bx + targetW, by, bx + targetW - corner, by, boxCol, thick);
        Line(bx + targetW, by, bx + targetW, by + corner, boxCol, thick);
        Line(bx, by + targetH, bx + corner, by + targetH, boxCol, thick);
        Line(bx, by + targetH, bx, by + targetH - corner, boxCol, thick);
        Line(bx + targetW, by + targetH,
             bx + targetW - corner, by + targetH, boxCol, thick);
        Line(bx + targetW, by + targetH,
             bx + targetW, by + targetH - corner, boxCol, thick);
    }

    if (state_.skeleton)
    {
        const float sway = std::sin(static_cast<float>(now) / 900.0f) * 1.6f;
        const float cx = bx + targetW * 0.5f;
        const D2D1_COLOR_F sk = state_.skeletonColor;

        Circle(cx, by + targetH * 0.16f, 6.4f, sk, false, 1.15f);
        Line(cx, by + targetH * 0.23f,
             cx + sway, by + targetH * 0.57f, sk, 1.15f);
        Line(cx, by + targetH * 0.34f,
             cx - targetW * 0.26f, by + targetH * 0.50f, sk, 1.15f);
        Line(cx, by + targetH * 0.34f,
             cx + targetW * 0.26f, by + targetH * 0.50f, sk, 1.15f);
        Line(cx + sway, by + targetH * 0.57f,
             cx - targetW * 0.22f, by + targetH * 0.91f, sk, 1.15f);
        Line(cx + sway, by + targetH * 0.57f,
             cx + targetW * 0.22f, by + targetH * 0.91f, sk, 1.15f);
    }

    if (state_.health)
    {
        const float barL = bx - 13.0f;
        const float barR = barL + 6.0f;
        const float barT = by;
        const float barB = by + targetH;

        Round(barL, barT, barR, barB, 3.0f, Col(0x242C31));

        const float fillT = barB - targetH * hp;
        if (fillT < barB - 1.0f)
            Round(barL, fillT, barR, barB, 3.0f, hpColor);

        Text(std::to_wstring(static_cast<int>(std::round(previewDisplayedHp_))),
             barL - 17.0f, barB - 16.0f, 40.0f, 16.0f,
             hpColor, tiny_, DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    if (state_.weapon)
    {
        Text(L"RIFLE  28", bx - 25.0f, by + targetH + 2.0f,
             targetW + 50.0f, 17.0f, state_.weaponColor,
             tiny_, DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    // Plain damage number: rises and fades with no surrounding UI.
    if (previewDamageTextTimer_ > 0.0f && previewTargetHp_ < previewPreviousHp_)
    {
        const int damage = static_cast<int>(
            std::round(previewPreviousHp_ - previewTargetHp_));
        constexpr float duration = 1.35f;
        const float progress = std::clamp(
            1.0f - previewDamageTextTimer_ / duration, 0.0f, 1.0f);
        const float eased = 1.0f - std::pow(1.0f - progress, 3.0f);
        const float rise = 38.0f * eased;
        const float fade = progress < 0.35f
            ? 1.0f
            : std::clamp(1.0f - (progress - 0.35f) / 0.65f, 0.0f, 1.0f);

        D2D1_COLOR_F damageColor = state_.healthLowColor;
        damageColor.a *= fade;

        Text(std::to_wstring(damage),
             bx + targetW + 8.0f,
             by + targetH * 0.52f - rise,
             std::max(46.0f, stageR - (bx + targetW + 8.0f)),
             32.0f,
             damageColor, bold_, DWRITE_TEXT_ALIGNMENT_CENTER);
    }
}

void App::DrawVisualsPage(float x, float y, float width, float height)
{
    (void)height;

    Text(L"Visuals", x, y, width, 32.0f, g_theme.text, bold_);

    const float gap = 26.0f;
    const float colW = (width - gap) / 2.0f;
    const float right = x + colW + gap;

    SectionLabel(L"Player ESP", x, y + 44.0f, colW);
    float ly = y + 78.0f;
    Toggle(L"Enabled", state_.esp, x, ly, colW); ly += 43.0f;
    ColorToggle(L"Through walls", state_.espThroughWallsColor,
                state_.throughWalls, x, ly, colW); ly += 43.0f;
    Combo(L"visuals.yielding_frames", L"Yielding frames",
          { L"Off", L"1 frame", L"2 frames", L"3 frames" },
          state_.yieldingFrames, x, ly, colW); ly += 43.0f;
    Toggle(L"Limit distance", state_.limitDistance, x, ly, colW); ly += 43.0f;
    Toggle(L"Last visible position", state_.lastVisiblePosition, x, ly, colW); ly += 43.0f;
    GearToggle(L"Box", state_.box, x, ly, colW, &state_.boxColor); ly += 43.0f;
    GearToggle(L"Health", state_.health, x, ly, colW, &state_.healthHighColor); ly += 43.0f;
    GearToggle(L"Weapon", state_.weapon, x, ly, colW, &state_.weaponColor); ly += 43.0f;
    GearToggle(L"Name", state_.name, x, ly, colW, &state_.nameColor); ly += 43.0f;
    GearToggle(L"Skeleton", state_.skeleton, x, ly, colW, &state_.skeletonColor); ly += 43.0f;
    Toggle(L"Sound ESP", state_.sound, x, ly, colW); ly += 43.0f;
    Toggle(L"Off-screen arrows", state_.offscreen, x, ly, colW);

    SectionLabel(L"Chams", right, y + 44.0f, colW);
    float ry = y + 78.0f;
    ColorToggle(L"Enabled", state_.chamsEnabledColor,
                state_.chams, right, ry, colW); ry += 43.0f;
    ColorToggle(L"Through walls", state_.chamsThroughWallsColor,
                state_.chamsThroughWalls, right, ry, colW); ry += 43.0f;
    Toggle(L"Distance fade", state_.chamsDistance, right, ry, colW); ry += 43.0f;
    Combo(L"visuals.visible_material", L"Visible material",
          { L"Flat", L"Glow", L"Metallic", L"Glass" },
          state_.visibleMaterial, right, ry, colW); ry += 43.0f;
    Combo(L"visuals.occluded_material", L"Occluded material",
          { L"Flat", L"Glow", L"Metallic", L"Glass" },
          state_.occludedMaterial, right, ry, colW); ry += 43.0f;
    Combo(L"visuals.colour_mode", L"Colour mode",
          { L"Static", L"Health", L"Visibility", L"Team" },
          state_.colourMode, right, ry, colW); ry += 43.0f;
    ColorToggle(L"Visible overlay", state_.overlayVisibleColor,
                state_.overlayVisible, right, ry, colW); ry += 58.0f;

    // Full ESP target preview with animated number-only damage popup.
    DrawEmbeddedPreview(right, ry, colW, 178.0f);
}

void App::DrawPlayers(float x, float y, float width)
{
    Text(L"Aimbot", x, y, width, 32, g_theme.text, bold_);

    const float gap = 26.0f;
    const float colW = (width - gap) / 2.0f;
    const float right = x + colW + gap;

    SectionLabel(L"General", x, y + 44.0f, colW);
    float ly = y + 78.0f;
    AimbotGearToggle(L"Enabled", state_.aimbotEnabled, 1, x, ly, colW); ly += 43.0f;
    AimbotGearToggle(L"Silent aim", state_.silentAim, 2, x, ly, colW); ly += 43.0f;
    AimbotGearToggle(L"Visible only", state_.visibleOnly, 3, x, ly, colW); ly += 43.0f;
    AimbotGearToggle(L"Recoil control", state_.recoilControl, 4, x, ly, colW); ly += 43.0f;
    AimbotGearToggle(L"Auto fire", state_.autoFire, 5, x, ly, colW); ly += 43.0f;
    AimbotGearToggle(L"Auto scope", state_.autoScope, 6, x, ly, colW); ly += 43.0f;
    AimbotGearToggle(L"Target lock", state_.targetLock, 7, x, ly, colW); ly += 43.0f;
    Combo(L"aim.target_selection", L"Target selection",
          { L"FOV", L"Distance", L"Health", L"Threat" },
          state_.targetSelection, x, ly, colW); ly += 43.0f;
    Combo(L"aim.hitbox", L"Hitbox",
          { L"Head", L"Chest", L"Stomach", L"Nearest" },
          state_.hitbox, x, ly, colW); ly += 43.0f;
    ValueSlider(L"Aim FOV", state_.aimFov, 1, 30, 1, L"°", x, ly, colW); ly += 53.0f;
    ValueSlider(L"Smoothing", state_.smoothing, 0, 20, 1, L"", x, ly, colW);

    SectionLabel(L"Accuracy & overrides", right, y + 44.0f, colW);
    float ry = y + 78.0f;
    ValueSlider(L"Hit chance", state_.hitChance, 0, 100, 1, L"%", right, ry, colW); ry += 53.0f;
    ValueSlider(L"Minimum damage", state_.minimumDamage, 0, 130, 1, L"", right, ry, colW); ry += 53.0f;
    AimbotGearToggle(L"Minimum damage override", state_.minDamageOverride, 8, right, ry, colW); ry += 43.0f;
    ValueSlider(L"Override damage", state_.overrideDamage, 0, 130, 1, L"", right, ry, colW); ry += 53.0f;
    Combo(L"aim.override_key", L"Override key",
          { L"ALT", L"SHIFT", L"MOUSE4", L"MOUSE5" },
          state_.overrideKey, right, ry, colW); ry += 43.0f;
    AimbotGearToggle(L"Force body aim", state_.forceBodyAim, 9, right, ry, colW); ry += 43.0f;
    AimbotGearToggle(L"Force safe point", state_.forceSafePoint, 10, right, ry, colW); ry += 43.0f;
    AimbotGearToggle(L"Ignore smoke", state_.ignoreSmoke, 11, right, ry, colW); ry += 43.0f;

    // The full live preview is shown on the Visuals page so this page stays uncluttered.
}

void App::DrawWorld(float x, float y, float width)
{
    Text(L"World", x, y, width, 32, g_theme.text, bold_);
    const float gap = 26.0f, colW = (width - gap) / 2.0f, right = x + colW + gap;
    SectionLabel(L"Objects", x, y + 44.0f, colW);
    float ly = y + 78.0f;
    GearToggle(L"World items", state_.worldItems, x, ly, colW); ly += 43;
    GearToggle(L"Objectives", state_.worldObjectives, x, ly, colW); ly += 43;
    Toggle(L"Show distance", state_.worldDistance, x, ly, colW); ly += 43;
    Combo(L"world.quality", L"Render quality", {L"Low", L"Medium", L"High"}, state_.worldQuality, x, ly, colW);
    SectionLabel(L"Scanner", right, y + 44.0f, colW);
    Fill(right, y + 78.0f, right + colW, y + 290.0f, g_theme.rowDark);
    Rect(right, y + 78.0f, right + colW, y + 290.0f, g_theme.border);
    Circle(right + colW / 2.0f, y + 184.0f, 72.0f, Col(0x2B3339), false, 1.0f);
    Circle(right + colW / 2.0f, y + 184.0f, 42.0f, Col(0x252D33), false, 1.0f);
    Line(right + colW / 2.0f, y + 104.0f, right + colW / 2.0f, y + 264.0f, Col(0x242C31));
    Line(right + colW / 2.0f - 80.0f, y + 184.0f, right + colW / 2.0f + 80.0f, y + 184.0f, Col(0x242C31));
    Circle(right + colW / 2.0f + 28.0f, y + 150.0f, 4.0f, g_theme.red, true);
    Circle(right + colW / 2.0f - 44.0f, y + 205.0f, 3.0f, g_theme.cyan, true);
}

void App::DrawView(float x, float y, float width)
{
    Text(L"View", x, y, width, 32, g_theme.text, bold_);
    const float gap = 26.0f;
    const float colW = (width - gap) / 2.0f;
    const float right = x + colW + gap;

    // No preview card on this page. This page is now entirely real variables.
    SectionLabel(L"Watermark", x, y + 44.0f, colW);
    float ly = y + 78.0f;
    GearToggle(L"Enable watermark", state_.watermark, x, ly, colW); ly += 43.0f;
    Toggle(L"Show product name", state_.showName, x, ly, colW); ly += 43.0f;
    Toggle(L"Show real-time FPS", state_.showFps, x, ly, colW); ly += 43.0f;
    Toggle(L"Show clock", state_.showClock, x, ly, colW); ly += 43.0f;
    Toggle(L"Status indicator", state_.statusIndicator, x, ly, colW); ly += 43.0f;
    Toggle(L"Compact rectangle", state_.compactMode, x, ly, colW); ly += 43.0f;
    Combo(L"view.watermark_position", L"Position", {L"Top right", L"Top left", L"Bottom right"}, state_.watermarkPosition, x, ly, colW); ly += 43.0f;
    ValueSlider(L"UI scale", state_.viewScale, 90, 110, 5, L"%", x, ly, colW);

    SectionLabel(L"Menu interface", right, y + 44.0f, colW);
    float ry = y + 78.0f;
    AimbotGearToggle(L"Notifications", state_.notifications, 21, right, ry, colW); ry += 43.0f;
    AimbotGearToggle(L"Keybind list", state_.keybindList, 22, right, ry, colW); ry += 43.0f;
    AimbotGearToggle(L"Spectator list", state_.spectatorList, 23, right, ry, colW); ry += 43.0f;
    AimbotGearToggle(L"Background blur", state_.menuBlur, 24, right, ry, colW); ry += 43.0f;
    AimbotGearToggle(L"Stream-proof capture", state_.streamProof, 25, right, ry, colW); ry += 43.0f;
    AimbotGearToggle(L"Menu particles", state_.menuParticles, 26, right, ry, colW); ry += 43.0f;
    AimbotGearToggle(L"Remember position", state_.rememberPosition, 27, right, ry, colW); ry += 43.0f;
    AimbotGearToggle(L"Animation effects", state_.animationEffects, 28, right, ry, colW); ry += 43.0f;
    Combo(L"view.theme", L"Theme", {L"Midnight", L"Graphite", L"Obsidian"}, state_.menuTheme, right, ry, colW); ry += 43.0f;
    Combo(L"view.animation_speed", L"Animation speed", {L"Slow", L"Normal", L"Fast"}, state_.animationSpeed, right, ry, colW);
}


void App::DrawConfigs(float x, float y, float width)
{
    Text(L"Configs", x, y, width, 32.0f, g_theme.text, bold_);
    SectionLabel(L"Configuration manager", x, y + 44.0f, width);

    RefreshConfigs();

    const float panelY = y + 78.0f;
    const float panelH = 326.0f;
    Round(x, panelY, x + width, panelY + panelH, 1.0f, g_theme.rowDark);
    RoundRect(x, panelY, x + width, panelY + panelH, 1.0f, g_theme.border, 1.0f);

    const float inputX = x + 24.0f;
    const float inputY = panelY + 24.0f;
    const float inputW = width - 48.0f;
    const float inputH = 42.0f;
    const bool inputHover = Hover(inputX, inputY, inputX + inputW, inputY + inputH);

    if (!colorPickerOpen_ && openCombo_.empty() && pressed_)
    {
        if (inputHover)
        {
            configNameFocused_ = true;
            SetFocus(hwnd_);
        }
        else
        {
            configNameFocused_ = false;
        }
    }

    Round(inputX, inputY, inputX + inputW, inputY + inputH, 1.0f,
          inputHover || configNameFocused_ ? Col(0x171D22) : Col(0x11161A));
    RoundRect(inputX, inputY, inputX + inputW, inputY + inputH, 1.0f,
              configNameFocused_ ? g_theme.red : Col(0x343B41),
              configNameFocused_ ? 1.4f : 1.0f);

    Text(L"Config name", inputX + 14.0f, inputY - 21.0f,
         inputW, 18.0f, g_theme.sub, tiny_);

    std::wstring shownName = configName_.empty() ? L"Type a config name..." : configName_;
    if (configNameFocused_ && (GetTickCount64() / 500ULL) % 2ULL == 0ULL)
        shownName += L"|";
    Text(shownName, inputX + 14.0f, inputY, inputW - 28.0f, inputH,
         configName_.empty() ? g_theme.faint : g_theme.text, small_);

    const float saveY = panelY + 84.0f;
    if (ActionButton(L"Create / Save", inputX, saveY, 150.0f, true))
    {
        const std::wstring clean = SanitizeConfigName(configName_);
        if (clean.empty())
        {
            configStatus_ = L"Enter a valid configuration name.";
        }
        else if (SaveConfig(clean))
        {
            configName_ = clean;
            RefreshConfigs();
            const auto it = std::find(configNames_.begin(), configNames_.end(), clean);
            if (it != configNames_.end())
                selectedConfig_ = static_cast<int>(std::distance(configNames_.begin(), it));
            configStatus_ = L"Saved: " + clean;
        }
        else
        {
            configStatus_ = L"Could not save this configuration.";
        }
        configStatusTime_ = 3.0f;
    }

    const float listY = panelY + 144.0f;
    if (!configNames_.empty())
    {
        Combo(L"configs.named_list", L"Saved configuration",
              configNames_, selectedConfig_, inputX, listY, inputW);
    }
    else
    {
        RowBase(inputX, listY, inputW, 42.0f, false);
        Text(L"Saved configuration", inputX + 16.0f, listY,
             inputW * 0.48f, 42.0f, g_theme.text, small_);
        Text(L"No configs yet", inputX + inputW * 0.51f, listY,
             inputW * 0.40f, 42.0f, g_theme.faint, tiny_,
             DWRITE_TEXT_ALIGNMENT_TRAILING);
    }

    const float bw = (width - 58.0f) / 2.0f;
    const float actionY = panelY + 204.0f;
    const bool hasSelection = !configNames_.empty() &&
        selectedConfig_ >= 0 && selectedConfig_ < static_cast<int>(configNames_.size());

    if (ActionButton(L"Load selected", inputX, actionY, bw))
    {
        if (hasSelection && LoadConfig(configNames_[selectedConfig_]))
        {
            configName_ = configNames_[selectedConfig_];
            configStatus_ = L"Loaded: " + configName_;
        }
        else
        {
            configStatus_ = L"Select an existing configuration first.";
        }
        configStatusTime_ = 3.0f;
    }

    if (ActionButton(L"Delete selected", inputX + bw + 10.0f,
                     actionY, bw, false, true))
    {
        if (hasSelection && DeleteConfig(configNames_[selectedConfig_]))
        {
            configStatus_ = L"Deleted: " + configNames_[selectedConfig_];
            RefreshConfigs();
        }
        else
        {
            configStatus_ = L"Select an existing configuration first.";
        }
        configStatusTime_ = 3.0f;
    }

    Text(L"Unlimited named configs are stored in %APPDATA%\\SpectraUI\\Configs.",
         inputX, panelY + 258.0f, inputW, 22.0f, g_theme.sub, tiny_);

    if (!configStatus_.empty())
    {
        Round(inputX, panelY + 284.0f, inputX + inputW,
              panelY + 316.0f, 1.0f, Col(0x151B1F));
        Text(configStatus_, inputX + 14.0f, panelY + 284.0f,
             inputW - 28.0f, 32.0f, g_theme.text, tiny_);
    }
}

void App::DrawPlaceholder(float x, float y, float width, const std::wstring& title)
{
    Text(title, x, y, width, 34, g_theme.text, bold_);
}

void App::DrawContent(float width, float height)
{
    Fill(Ui::Sidebar, Ui::Topbar, width, height, g_theme.bg);
    float x = Ui::Sidebar + 28.0f;
    float y = Ui::Topbar + 18.0f;
    float w = width - Ui::Sidebar - 56.0f;
    switch (state_.page)
    {
    case Page::Visuals: DrawVisualsPage(x, y, w, height - y - 16.0f); break;
    case Page::Players: DrawPlayers(x, y, w); break;
    case Page::World: DrawWorld(x, y, w); break;
    case Page::View: DrawView(x, y, w); break;
    case Page::Configs: DrawConfigs(x, y, w); break;
    }
}


void App::DrawRuntimeOverlay(float width)
{
    const ULONGLONG now = GetTickCount64();
    const float seconds = static_cast<float>(now) / 1000.0f;
    const float speed = state_.animationSpeed == 0 ? 0.55f : state_.animationSpeed == 2 ? 1.8f : 1.0f;

    if (state_.menuParticles)
    {
        for (int i = 0; i < 22; ++i)
        {
            const float seed = static_cast<float>(i * 73 + 19);
            const float px = Ui::Sidebar + 18.0f + std::fmod(seed * 9.37f, width - Ui::Sidebar - 36.0f);
            const float range = static_cast<float>(Ui::Height - Ui::Topbar - 30);
            const float py = Ui::Topbar + 15.0f + std::fmod(seed * 5.1f + seconds * (8.0f + i % 5) * speed, range);
            const float alpha = 0.10f + 0.10f * (std::sin(seconds * speed + i) * 0.5f + 0.5f);
            Circle(px, py, 1.2f + static_cast<float>(i % 3) * 0.35f, Col(0xFFFFFF, alpha), true);
        }
    }


    // Keybind and spectator lists are separate draggable desktop windows.

}

std::filesystem::path App::InterfacePath() const
{
    wchar_t appData[MAX_PATH]{};
    const DWORD n = GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH);
    std::filesystem::path dir = n ? std::filesystem::path(appData) / L"SpectraUI"
                                  : std::filesystem::current_path();
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir / L"interface.ini";
}

void App::LoadWindowPosition()
{
    std::wifstream file(InterfacePath());
    int mx = -1, my = -1, kx = -1, ky = -1, sx = -1, sy = -1;
    if (file >> mx >> my)
        savedWindowPos_ = POINT{mx, my};
    if (file >> kx >> ky)
        savedKeybindPos_ = POINT{kx, ky};
    if (file >> sx >> sy)
        savedSpectatorPos_ = POINT{sx, sy};
}

void App::SaveWindowPosition()
{
    if (!state_.rememberPosition) return;
    SaveOverlayPositions();
}

void App::SaveOverlayPositions()
{
    RECT mainRc{}, keyRc{}, specRc{};
    if (hwnd_ && !IsIconic(hwnd_)) GetWindowRect(hwnd_, &mainRc);
    if (keybindHwnd_) GetWindowRect(keybindHwnd_, &keyRc);
    if (spectatorHwnd_) GetWindowRect(spectatorHwnd_, &specRc);
    std::wofstream file(InterfacePath(), std::ios::trunc);
    if (file)
        file << mainRc.left << L' ' << mainRc.top << L' '
             << keyRc.left << L' ' << keyRc.top << L' '
             << specRc.left << L' ' << specRc.top;
}

void App::ApplyTheme()
{
    state_.menuTheme = std::clamp(state_.menuTheme, 0, 2);
    if (state_.menuTheme == lastTheme_) return;
    lastTheme_ = state_.menuTheme;

    if (state_.menuTheme == 0) // Midnight
    {
        g_theme.bg = Col(0x090B0D, 0.93f); g_theme.sidebar = Col(0x0B0E11, 0.96f);
        g_theme.top = Col(0x0B0E11, 0.94f); g_theme.row = Col(0x101417, 0.86f);
        g_theme.rowHover = Col(0x14191D, 0.96f); g_theme.rowDark = Col(0x0D1013, 0.84f);
        g_theme.border = Col(0x1A2025, 0.88f); g_theme.red = Col(0xFF174C);
    }
    else if (state_.menuTheme == 1) // Graphite
    {
        g_theme.bg = Col(0x151719, 0.97f); g_theme.sidebar = Col(0x111315, 0.99f);
        g_theme.top = Col(0x181A1D, 0.98f); g_theme.row = Col(0x202326, 0.96f);
        g_theme.rowHover = Col(0x292D31, 0.98f); g_theme.rowDark = Col(0x1A1D20, 0.96f);
        g_theme.border = Col(0x353A3F, 0.95f); g_theme.red = Col(0xFF315F);
    }
    else // Obsidian
    {
        g_theme.bg = Col(0x030405, 0.98f); g_theme.sidebar = Col(0x050607, 1.0f);
        g_theme.top = Col(0x070809, 0.99f); g_theme.row = Col(0x0B0D0F, 0.98f);
        g_theme.rowHover = Col(0x111418, 0.99f); g_theme.rowDark = Col(0x080A0C, 0.98f);
        g_theme.border = Col(0x20252A, 0.95f); g_theme.red = Col(0xE91646);
    }
}

void App::ApplyBlurSetting()
{
    if (state_.menuBlur == lastBlur_ && interfaceInitialized_) return;
    lastBlur_ = state_.menuBlur;
    DWM_BLURBEHIND blur{};
    blur.dwFlags = DWM_BB_ENABLE;
    blur.fEnable = state_.menuBlur ? TRUE : FALSE;
    DwmEnableBlurBehindWindow(hwnd_, &blur);
}

void App::ApplyStreamProofSetting()
{
    if (state_.streamProof == lastStreamProof_ && interfaceInitialized_) return;
    lastStreamProof_ = state_.streamProof;
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif
    SetWindowDisplayAffinity(hwnd_, state_.streamProof ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
}

void App::ApplyScaleSetting()
{
    state_.viewScale = std::clamp(state_.viewScale, 90, 110);
    if (state_.viewScale == lastViewScale_ && interfaceInitialized_) return;
    lastViewScale_ = state_.viewScale;
    const float scale = static_cast<float>(state_.viewScale) / 100.0f;
    RECT rc{};
    GetWindowRect(hwnd_, &rc);
    SetWindowPos(hwnd_, nullptr, rc.left, rc.top,
                 static_cast<int>(Ui::Width * scale), static_cast<int>(Ui::Height * scale),
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void App::ApplyInterfaceSettings()
{
    ApplyTheme();
    ApplyBlurSetting();
    ApplyStreamProofSetting();
    ApplyScaleSetting();
    interfaceInitialized_ = true;
}

std::wstring App::BuildWatermarkText() const
{
    std::wstring value;
    if (state_.showName)
        value += state_.compactMode ? L"SPECTRA" : L"SPECTRA UI";
    if (state_.statusIndicator)
    {
        if (!value.empty()) value += L"  |  ";
        value += L"ACTIVE";
    }
    if (state_.showFps)
    {
        if (!value.empty()) value += L"  |  ";
        value += std::to_wstring(currentFps_) + L" FPS";
    }
    if (state_.showClock)
    {
        std::time_t now = std::time(nullptr);
        std::tm local{};
        localtime_s(&local, &now);
        wchar_t buffer[16]{};
        wcsftime(buffer, ARRAYSIZE(buffer), L"%H:%M:%S", &local);
        if (!value.empty()) value += L"  |  ";
        value += buffer;
    }
    return value.empty() ? L"SPECTRA" : value;
}

void App::UpdateCompanionWindows()
{
    if (!hwnd_) return;

    RECT mainRect{};
    GetWindowRect(hwnd_, &mainRect);

    if (watermarkHwnd_)
    {
        if (!state_.watermark)
        {
            if (IsWindowVisible(watermarkHwnd_)) ShowWindow(watermarkHwnd_, SW_HIDE);
        }
        else
        {
            HDC dc = GetDC(watermarkHwnd_);
            HFONT font = CreateFontW(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            HGDIOBJ oldFont = SelectObject(dc, font);
            const std::wstring text = BuildWatermarkText();
            SIZE size{};
            GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &size);
            SelectObject(dc, oldFont);
            DeleteObject(font);
            ReleaseDC(watermarkHwnd_, dc);

            const int measuredWidth = static_cast<int>(size.cx) + 34;
            const int desiredW = (measuredWidth > 150) ? measuredWidth : 150;
            const int desiredH = state_.compactMode ? 30 : 35;
            int desiredX = GetSystemMetrics(SM_CXSCREEN) - desiredW - 18;
            int desiredY = 18;
            if (state_.watermarkPosition == 1) desiredX = 18;
            if (state_.watermarkPosition == 2)
                desiredY = GetSystemMetrics(SM_CYSCREEN) - desiredH - 48;

            RECT current{};
            GetWindowRect(watermarkHwnd_, &current);
            const bool moved = current.left != desiredX || current.top != desiredY ||
                (current.right - current.left) != desiredW ||
                (current.bottom - current.top) != desiredH;

            if (moved)
            {
                SetWindowPos(watermarkHwnd_, HWND_TOPMOST, desiredX, desiredY, desiredW, desiredH,
                    SWP_NOACTIVATE | SWP_SHOWWINDOW);
            }
            else if (!IsWindowVisible(watermarkHwnd_))
            {
                ShowWindow(watermarkHwnd_, SW_SHOWNOACTIVATE);
            }
            InvalidateRect(watermarkHwnd_, nullptr, FALSE);
        }
    }
    auto updateOverlay = [](HWND window, bool enabled)
    {
        if (!window) return;
        if (enabled)
        {
            if (!IsWindowVisible(window)) ShowWindow(window, SW_SHOWNOACTIVATE);
            SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            InvalidateRect(window, nullptr, FALSE);
        }
        else if (IsWindowVisible(window))
        {
            ShowWindow(window, SW_HIDE);
        }
    };

    updateOverlay(keybindHwnd_, state_.keybindList);
    updateOverlay(spectatorHwnd_, state_.spectatorList);

}

void App::PaintWatermarkWindow(HWND hwnd)
{
    PAINTSTRUCT ps{};
    HDC windowDc = BeginPaint(hwnd, &ps);
    RECT rc{};
    GetClientRect(hwnd, &rc);

    const int width = rc.right - rc.left;
    const int height = rc.bottom - rc.top;
    HDC dc = CreateCompatibleDC(windowDc);
    HBITMAP bitmap = CreateCompatibleBitmap(windowDc, width, height);
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);

    HBRUSH background = CreateSolidBrush(RGB(8, 11, 13));
    FillRect(dc, &rc, background);
    DeleteObject(background);

    HPEN border = CreatePen(PS_SOLID, 1, RGB(52, 61, 68));
    HGDIOBJ oldPen = SelectObject(dc, border);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc, 0, 0, rc.right, rc.bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(border);

    RECT accent{0, 0, 3, rc.bottom};
    HBRUSH accentBrush = CreateSolidBrush(RGB(255, 23, 76));
    FillRect(dc, &accent, accentBrush);
    DeleteObject(accentBrush);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(238, 241, 244));
    HFONT font = CreateFontW(state_.compactMode ? 13 : 14, 0, 0, 0, FW_SEMIBOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HGDIOBJ oldFont = SelectObject(dc, font);
    RECT textRect{13, 0, rc.right - 8, rc.bottom};
    const std::wstring text = BuildWatermarkText();
    DrawTextW(dc, text.c_str(), -1, &textRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, oldFont);
    DeleteObject(font);

    BitBlt(windowDc, 0, 0, width, height, dc, 0, 0, SRCCOPY);
    SelectObject(dc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(dc);
    EndPaint(hwnd, &ps);
}

void App::PaintPreviewWindow(HWND hwnd)
{
    PAINTSTRUCT ps{};
    HDC windowDc = BeginPaint(hwnd, &ps);
    RECT rc{};
    GetClientRect(hwnd, &rc);
    const int bufferWidth = rc.right - rc.left;
    const int bufferHeight = rc.bottom - rc.top;
    HDC dc = CreateCompatibleDC(windowDc);
    HBITMAP bitmap = CreateCompatibleBitmap(windowDc, bufferWidth, bufferHeight);
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);

    HBRUSH bg = CreateSolidBrush(RGB(9, 12, 14));
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    HPEN border = CreatePen(PS_SOLID, 1, RGB(35, 43, 49));
    HGDIOBJ oldPen = SelectObject(dc, border);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc, 0, 0, rc.right, rc.bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(border);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(218, 222, 225));
    HFONT titleFont = CreateFontW(17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HGDIOBJ oldFont = SelectObject(dc, titleFont);
    RECT titleRc{18, 16, rc.right - 18, 48};
    DrawTextW(dc, L"LIVE PREVIEW", -1, &titleRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SetTextColor(dc, RGB(255, 55, 96));
    HFONT badgeFont = CreateFontW(11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HGDIOBJ oldBadgeFont = SelectObject(dc, badgeFont);
    RECT badgeRc{18, 45, rc.right - 18, 68};
    DrawTextW(dc, L"SEPARATE WINDOW • REV 18", -1, &badgeRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, oldBadgeFont);
    DeleteObject(badgeFont);
    SetTextColor(dc, RGB(218, 222, 225));
    SelectObject(dc, oldFont);
    DeleteObject(titleFont);

    const float seconds = static_cast<float>(GetTickCount64()) / 1000.0f;
    const float wave = (std::sin(seconds * 0.55f) + 1.0f) * 0.5f;
    const float health = 0.10f + wave * 0.90f;

    const int boxW = 118;
    const int boxH = 260;
    const int bx = (rc.right - boxW) / 2;
    const int calculatedY = (static_cast<int>(rc.bottom) - boxH) / 2 - 12;
    const int by = (calculatedY > 100) ? calculatedY : 100;

    if (state_.name)
    {
        SetTextColor(dc, RGB(245, 247, 249));
        HFONT font = CreateFontW(13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        oldFont = SelectObject(dc, font);
        RECT nr{bx - 30, by - 28, bx + boxW + 30, by - 5};
        DrawTextW(dc, L"TARGET", -1, &nr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, oldFont);
        DeleteObject(font);
    }

    if (state_.box)
    {
        const COLORREF boxColor = health < 0.28f ? RGB(255, 53, 93) : RGB(232, 235, 238);
        HPEN pen = CreatePen(PS_SOLID, 2, boxColor);
        oldPen = SelectObject(dc, pen);
        SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
        Rectangle(dc, bx, by, bx + boxW, by + boxH);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    }

    if (state_.health)
    {
        RECT track{bx - 16, by, bx - 9, by + boxH};
        HBRUSH trackBrush = CreateSolidBrush(RGB(31, 38, 43));
        FillRect(dc, &track, trackBrush);
        DeleteObject(trackBrush);

        const int fillTop = by + static_cast<int>(boxH * (1.0f - health));
        COLORREF hp = health > 0.55f ? RGB(53, 208, 127) :
                      health > 0.28f ? RGB(255, 194, 71) : RGB(255, 53, 93);
        RECT fill{bx - 16, fillTop, bx - 9, by + boxH};
        HBRUSH hpBrush = CreateSolidBrush(hp);
        FillRect(dc, &fill, hpBrush);
        DeleteObject(hpBrush);

        SetTextColor(dc, hp);
        HFONT hpFont = CreateFontW(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        oldFont = SelectObject(dc, hpFont);
        wchar_t hpText[16]{};
        swprintf_s(hpText, L"%d", static_cast<int>(health * 100.0f));
        const int hpTextY = ((fillTop - 8) > by) ? (fillTop - 8) : by;
        TextOutW(dc, bx - 45, hpTextY, hpText, lstrlenW(hpText));
        SelectObject(dc, oldFont);
        DeleteObject(hpFont);
    }

    if (state_.skeleton)
    {
        HPEN skeletonPen = CreatePen(PS_SOLID, 1, RGB(238, 241, 244));
        oldPen = SelectObject(dc, skeletonPen);
        const int cx = bx + boxW / 2;
        const int sway = static_cast<int>(std::sin(seconds * 0.85f) * 5.0f);
        SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
        Ellipse(dc, cx - 13, by + 34, cx + 13, by + 60);
        MoveToEx(dc, cx, by + 60, nullptr); LineTo(dc, cx + sway, by + 145);
        MoveToEx(dc, cx + sway / 2, by + 92, nullptr); LineTo(dc, cx - 37, by + 128);
        MoveToEx(dc, cx + sway / 2, by + 92, nullptr); LineTo(dc, cx + 37, by + 128);
        MoveToEx(dc, cx + sway, by + 145, nullptr); LineTo(dc, cx - 29, by + 226);
        MoveToEx(dc, cx + sway, by + 145, nullptr); LineTo(dc, cx + 29, by + 226);
        SelectObject(dc, oldPen);
        DeleteObject(skeletonPen);
    }

    if (state_.weapon)
    {
        SetTextColor(dc, RGB(152, 160, 166));
        HFONT font = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        oldFont = SelectObject(dc, font);
        RECT wr{bx - 20, by + boxH + 8, bx + boxW + 20, by + boxH + 32};
        DrawTextW(dc, L"ITEM", -1, &wr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, oldFont);
        DeleteObject(font);
    }

    SetTextColor(dc, RGB(112, 121, 128));
    HFONT footerFont = CreateFontW(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    oldFont = SelectObject(dc, footerFont);
    RECT footer{14, rc.bottom - 44, rc.right - 14, rc.bottom - 14};
    DrawTextW(dc, L"Animated damage simulation", -1, &footer,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, oldFont);
    DeleteObject(footerFont);

    BitBlt(windowDc, 0, 0, bufferWidth, bufferHeight, dc, 0, 0, SRCCOPY);
    SelectObject(dc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(dc);
    EndPaint(hwnd, &ps);
}


void App::Render()
{
    if (FAILED(CreateDevice()))
        return;

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    float width = static_cast<float>(rc.right);
    float height = static_cast<float>(rc.bottom);

    ++fpsFrameCount_;
    const ULONGLONG fpsNow = GetTickCount64();
    const ULONGLONG fpsElapsed = fpsNow - fpsWindowStart_;
    if (fpsElapsed >= 500)
    {
        currentFps_ = static_cast<int>((fpsFrameCount_ * 1000ULL) / fpsElapsed);
        fpsFrameCount_ = 0;
        fpsWindowStart_ = fpsNow;
    }

    target_->BeginDraw();
    target_->SetTransform(D2D1::Matrix3x2F::Identity());
    target_->Clear(Col(0x000000, 0.0f));

    if (configStatusTime_ > 0.0f)
    {
        configStatusTime_ -= 0.016f;
        if (configStatusTime_ <= 0.0f) configStatus_.clear();
    }

    dropdownOverlayReady_ = false;
    dropdownOverlaySelection_ = nullptr;
    dropdownOverlayItems_.clear();

    DrawSidebar(height);
    DrawTopbar(width);
    DrawContent(width, height);
    DrawRuntimeOverlay(width);
    DrawDropdownOverlay();
    DrawGearPopup(width, height);
    DrawColorPicker(width, height);
    Rect(0.5f, 0.5f, width - 0.5f, height - 0.5f, Col(0x242A2F, 0.85f));

    HRESULT hr = target_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
        DiscardDevice();

    pressed_ = false;
}


void App::PaintOverlayWindow(HWND hwnd, bool keybind)
{
    PAINTSTRUCT ps{};
    HDC windowDc = BeginPaint(hwnd, &ps);
    RECT rc{};
    GetClientRect(hwnd, &rc);
    const int width = rc.right - rc.left;
    const int height = rc.bottom - rc.top;
    HDC dc = CreateCompatibleDC(windowDc);
    HBITMAP bitmap = CreateCompatibleBitmap(windowDc, width, height);
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);

    HBRUSH bg = CreateSolidBrush(RGB(8, 11, 13));
    FillRect(dc, &rc, bg); DeleteObject(bg);
    HPEN border = CreatePen(PS_SOLID, 1, RGB(52, 61, 68));
    HGDIOBJ oldPen = SelectObject(dc, border);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc, 0, 0, rc.right, rc.bottom);
    SelectObject(dc, oldBrush); SelectObject(dc, oldPen); DeleteObject(border);

    RECT accent{0, 0, 3, rc.bottom};
    HBRUSH accentBrush = CreateSolidBrush(keybind ? RGB(255, 23, 76) : RGB(0, 217, 255));
    FillRect(dc, &accent, accentBrush); DeleteObject(accentBrush);

    SetBkMode(dc, TRANSPARENT);
    HFONT titleFont = CreateFontW(13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HGDIOBJ oldFont = SelectObject(dc, titleFont);
    SetTextColor(dc, RGB(242, 244, 246));
    RECT title{13, 7, rc.right - 10, 29};
    DrawTextW(dc, keybind ? L"KEYBINDS" : L"SPECTATORS", -1, &title,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, oldFont); DeleteObject(titleFont);

    HFONT rowFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    oldFont = SelectObject(dc, rowFont);
    if (keybind)
    {
        SetTextColor(dc, RGB(165, 171, 177));
        RECT a{13, 37, 125, 58}; DrawTextW(dc, L"Menu", -1, &a, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        RECT b{13, 64, 135, 86}; DrawTextW(dc, L"Damage override", -1, &b, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SetTextColor(dc, RGB(220, 224, 228));
        RECT ak{120, 37, rc.right - 12, 58}; DrawTextW(dc, L"INSERT", -1, &ak, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        RECT bk{120, 64, rc.right - 12, 86}; DrawTextW(dc, L"ALT", -1, &bk, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }
    else
    {
        SetTextColor(dc, RGB(165, 171, 177));
        RECT a{13, 38, rc.right - 12, 63}; DrawTextW(dc, L"No spectators", -1, &a, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    SelectObject(dc, oldFont); DeleteObject(rowFont);

    BitBlt(windowDc, 0, 0, width, height, dc, 0, 0, SRCCOPY);
    SelectObject(dc, oldBitmap); DeleteObject(bitmap); DeleteDC(dc);
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK App::OverlayProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    App* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    return app ? app->OverlayWindowProc(hwnd, message, wParam, lParam)
               : DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT App::OverlayWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
        PaintOverlayWindow(hwnd, hwnd == keybindHwnd_);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_NCHITTEST:
        // The complete overlay surface acts as a draggable title bar.
        return HTCAPTION;
    case WM_LBUTTONDOWN:
        // Explicit drag handling makes the panel movable even though it has no caption.
        ReleaseCapture();
        SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return 0;
    case WM_EXITSIZEMOVE:
        SaveOverlayPositions();
        return 0;
    case WM_LBUTTONDBLCLK:
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK App::PreviewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    App* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    return app ? app->PreviewWindowProc(hwnd, message, wParam, lParam)
               : DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK App::WatermarkProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    App* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    return app ? app->WatermarkWindowProc(hwnd, message, wParam, lParam)
               : DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT App::PreviewWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT: PaintPreviewWindow(hwnd); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_NCHITTEST: return HTTRANSPARENT;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT App::WatermarkWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT: PaintWatermarkWindow(hwnd); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_NCHITTEST: return HTTRANSPARENT;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK App::StaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    App* app = nullptr;

    if (message == WM_NCCREATE)
    {
        auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        app->hwnd_ = hwnd;
    }
    else
    {
        app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    return app ? app->Proc(message, wParam, lParam)
               : DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT App::Proc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_TIMER:
        ApplyInterfaceSettings();
        InvalidateRect(hwnd_, nullptr, FALSE);
        UpdateCompanionWindows();
        return 0;

    case WM_MOUSEMOVE:
        mouse_.x = GET_X_LPARAM(lParam);
        mouse_.y = GET_Y_LPARAM(lParam);
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDOWN:
    {
        SetCapture(hwnd_);
        down_ = true;
        pressed_ = true;
        mouse_.x = GET_X_LPARAM(lParam);
        mouse_.y = GET_Y_LPARAM(lParam);

        RECT rc{};
        GetClientRect(hwnd_, &rc);
        float x = static_cast<float>(mouse_.x);
        float y = static_cast<float>(mouse_.y);

        // While the modal picker is open, only its own controls should react.
        if (colorPickerOpen_ || gearPopupOpen_)
        {
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        // Match the exact vector-button bounds drawn in DrawTopbar.
        if (x >= rc.right - 45.0f && x <= rc.right - 11.0f &&
            y >= 18.0f && y <= 52.0f)
        {
            PostMessageW(hwnd_, WM_CLOSE, 0, 0);
            return 0;
        }

        if (x >= rc.right - 86.0f && x <= rc.right - 52.0f &&
            y >= 18.0f && y <= 52.0f)
        {
            ShowWindow(hwnd_, SW_MINIMIZE);
            return 0;
        }

        if (y < Ui::Topbar && x > Ui::Sidebar)
        {
            ReleaseCapture();
            down_ = false;
            pressed_ = false;
            SendMessageW(hwnd_, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            return 0;
        }

        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONUP:
        down_ = false;
        activeRgbSlider_ = -1;
        ReleaseCapture();
        return 0;

    case WM_MOVE:
        if (state_.rememberPosition && !IsIconic(hwnd_)) SaveWindowPosition();
        return 0;

    case WM_SIZE:
        if (target_)
            target_->Resize(D2D1::SizeU(LOWORD(lParam), HIWORD(lParam)));
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        BeginPaint(hwnd_, &ps);
        Render();
        EndPaint(hwnd_, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_CHAR:
        if (configNameFocused_ && state_.page == Page::Configs)
        {
            const wchar_t ch = static_cast<wchar_t>(wParam);
            if (ch == L'\b')
            {
                if (!configName_.empty())
                    configName_.pop_back();
            }
            else if (ch == L'\r' || ch == L'\n' || ch == L'\t')
            {
                configNameFocused_ = false;
            }
            else if (ch >= 32 && configName_.size() < 32)
            {
                const bool invalid = ch == L'<' || ch == L'>' || ch == L':' ||
                    ch == L'"' || ch == L'/' || ch == L'\\' || ch == L'|' ||
                    ch == L'?' || ch == L'*';
                if (!invalid)
                    configName_.push_back(ch);
            }
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            if (configNameFocused_)
            {
                configNameFocused_ = false;
            }
            else if (gearPopupOpen_)
            {
                gearPopupOpen_ = false;
                gearPopupValue_ = nullptr;
                gearPopupColor_ = nullptr;
            }
            else if (colorPickerOpen_ && pickerColor_)
            {
                *pickerColor_ = pickerOriginal_;
                colorPickerOpen_ = false;
                pickerColor_ = nullptr;
                activeRgbSlider_ = -1;
            }
            else if (!openCombo_.empty())
                openCombo_.clear();
            else
                PostMessageW(hwnd_, WM_CLOSE, 0, 0);
        }
        return 0;

    case WM_DESTROY:
        SaveWindowPosition();
        KillTimer(hwnd_, 1);
        if (watermarkHwnd_) ShowWindow(watermarkHwnd_, SW_HIDE);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd_, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    App app;
    if (!app.Init(instance))
    {
        MessageBoxW(nullptr, L"Failed to initialize the Direct2D interface.", L"Error", MB_OK | MB_ICONERROR);
        app.Shutdown();
        return 1;
    }

    int result = app.Run();
    app.Shutdown();
    return result;
}
