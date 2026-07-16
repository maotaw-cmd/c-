// Compact purple Direct2D menu UI demo - updated
// Build in Visual Studio 2022 as a Windows Desktop application:
//
// cl /std:c++17 /EHsc /DUNICODE /D_UNICODE compact_menu_updated.cpp ^
//    d2d1.lib dwrite.lib dwmapi.lib user32.lib
//
// UI demonstration only. No game integration, injection, memory access,
// automation, or gameplay functionality.

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <system_error>

#include <Windows.h>
#include <Windowsx.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dwmapi.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

#pragma message("COMPACT_MENU_SCROLL_SETTINGS_DROPDOWNS_ACTIVE")
#pragma message("HSV_RECTANGLE_COLOR_PICKER_ACTIVE")
#pragma message("COLOR_SWATCH_SMALL_BORDERLESS_ACTIVE")
#pragma message("CUSTOM_ESP_RGB_COLOR_PICKER_ACTIVE")
#pragma message("CARD_BOUNDS_FIXED_ALL_CONTROLS_INSIDE")
#pragma message("PARTICLE_BACKGROUND_SETTING_ACTIVE")
#pragma message("ESP_CARD_MULTI_COLOR_CONTROLS_ACTIVE")
#pragma message("REAL_CONFIG_SAVE_LOAD_DELETE_V6")
#pragma message("DROPDOWN_NULL_POINTER_CRASH_FIXED_V7")
#pragma message("HOTKEY_COMBINATIONS_CTRL_ALT_SHIFT_V8")

template <typename T>
static void SafeRelease(T*& value)
{
    if (value)
    {
        value->Release();
        value = nullptr;
    }
}

static D2D1_COLOR_F ColorFromHex(std::uint32_t value, float alpha = 1.0f)
{
    return D2D1::ColorF(
        static_cast<float>((value >> 16) & 0xFF) / 255.0f,
        static_cast<float>((value >> 8) & 0xFF) / 255.0f,
        static_cast<float>(value & 0xFF) / 255.0f,
        alpha);
}

static D2D1_COLOR_F HsvToColor(float hue, float saturation, float value)
{
    hue = hue - std::floor(hue);
    saturation = std::clamp(saturation, 0.0f, 1.0f);
    value = std::clamp(value, 0.0f, 1.0f);

    const float scaled = hue * 6.0f;
    const int sector = static_cast<int>(std::floor(scaled)) % 6;
    const float fraction = scaled - std::floor(scaled);
    const float p = value * (1.0f - saturation);
    const float q = value * (1.0f - fraction * saturation);
    const float t = value * (1.0f - (1.0f - fraction) * saturation);

    switch (sector)
    {
    case 0: return D2D1::ColorF(value, t, p, 1.0f);
    case 1: return D2D1::ColorF(q, value, p, 1.0f);
    case 2: return D2D1::ColorF(p, value, t, 1.0f);
    case 3: return D2D1::ColorF(p, q, value, 1.0f);
    case 4: return D2D1::ColorF(t, p, value, 1.0f);
    default: return D2D1::ColorF(value, p, q, 1.0f);
    }
}

static void ColorToHsv(const D2D1_COLOR_F& color, float& hue, float& saturation, float& value)
{
    const float maximum = std::max({ color.r, color.g, color.b });
    const float minimum = std::min({ color.r, color.g, color.b });
    const float delta = maximum - minimum;

    value = maximum;
    saturation = maximum <= 0.00001f ? 0.0f : delta / maximum;

    if (delta <= 0.00001f)
    {
        hue = 0.0f;
        return;
    }

    if (maximum == color.r)
        hue = std::fmod((color.g - color.b) / delta, 6.0f) / 6.0f;
    else if (maximum == color.g)
        hue = (((color.b - color.r) / delta) + 2.0f) / 6.0f;
    else
        hue = (((color.r - color.g) / delta) + 4.0f) / 6.0f;

    if (hue < 0.0f)
        hue += 1.0f;
}

namespace UI
{
    constexpr int Width = 540;
    constexpr int Height = 430;

    constexpr float HeaderHeight = 34.0f;
    constexpr float Margin = 10.0f;
    constexpr float Gap = 10.0f;
    constexpr float ColumnWidth = 250.0f;

    constexpr float ViewTop = HeaderHeight;
    constexpr float ViewBottom = static_cast<float>(Height);
    constexpr float ViewHeight = ViewBottom - ViewTop;

    constexpr D2D1_COLOR_F Background = { 0.031f, 0.031f, 0.038f, 1.0f };
    constexpr D2D1_COLOR_F Header = { 0.018f, 0.018f, 0.024f, 1.0f };
    constexpr D2D1_COLOR_F Panel = { 0.052f, 0.052f, 0.062f, 1.0f };
    constexpr D2D1_COLOR_F PanelBorder = { 0.17f, 0.17f, 0.19f, 1.0f };
    constexpr D2D1_COLOR_F Control = { 0.032f, 0.032f, 0.040f, 1.0f };
    constexpr D2D1_COLOR_F ControlHover = { 0.078f, 0.064f, 0.098f, 1.0f };
    constexpr D2D1_COLOR_F Text = { 0.90f, 0.90f, 0.92f, 1.0f };
    constexpr D2D1_COLOR_F Muted = { 0.56f, 0.56f, 0.61f, 1.0f };
    constexpr D2D1_COLOR_F Accent = { 0.59f, 0.05f, 0.97f, 1.0f };
    constexpr D2D1_COLOR_F AccentHover = { 0.70f, 0.20f, 1.00f, 1.0f };
    constexpr D2D1_COLOR_F ScrollTrack = { 0.075f, 0.075f, 0.090f, 1.0f };
}


constexpr int HotkeyKeyMask   = 0x00FF;
constexpr int HotkeyCtrlFlag  = 0x0100;
constexpr int HotkeyAltFlag   = 0x0200;
constexpr int HotkeyShiftFlag = 0x0400;

struct State
{
    bool overrideShared = true;
    bool enabled = true;
    bool multipoint = true;
    bool forceShot = true;
    bool damageOverride = true;
    bool autoStop = true;

    bool silent = true;
    bool autowall = true;
    bool doubletap = false;
    bool nospread = false;
    bool fakeDuck = false;

    bool saveWindowPosition = true;
    bool compactText = false;
    bool menuAnimation = true;
    bool particleBackground = true;

    // ESP feature toggles and independently editable colors.
    bool espBox = true;
    bool espHealthBar = true;
    bool espName = true;
    bool espSkeleton = true;
    bool espWeapon = true;
    bool espDistance = false;

    D2D1_COLOR_F espRectangleColor = ColorFromHex(0x9900FF);
    D2D1_COLOR_F espHealthColor = ColorFromHex(0x38D878);
    D2D1_COLOR_F espNameColor = ColorFromHex(0xFFFFFF);
    D2D1_COLOR_F espSkeletonColor = ColorFromHex(0xB8C4FF);
    D2D1_COLOR_F espWeaponColor = ColorFromHex(0xFFB347);
    D2D1_COLOR_F espDistanceColor = ColorFromHex(0x69D2FF);

    int weaponGroup = 0;
    int hitbox = 0;
    int yaw = 0;
    int yawJitter = 0;
    int move = 0;

    int multipointValue = 67;
    int hitchance = 70;
    int minDamage = 1;
    int fieldOfView = 90;
    int yawValue = 0;

    // Editable virtual-key hotkeys.
    int enabledHotkey = VK_INSERT;
    int forceShotHotkey = VK_XBUTTON1;
    int damageOverrideHotkey = HotkeyAltFlag | 'X';
    int doubletapHotkey = 0;
    int nospreadHotkey = 'N';
    int moveOverrideHotkey = 'F';

    std::wstring configName = L"default";
};

class Application
{
public:
    bool Initialize(HINSTANCE instance);
    int Run();
    void Shutdown();

private:
    HWND window_ = nullptr;
    HINSTANCE instance_ = nullptr;

    ID2D1Factory* factory_ = nullptr;
    ID2D1HwndRenderTarget* target_ = nullptr;
    ID2D1SolidColorBrush* brush_ = nullptr;

    IDWriteFactory* writeFactory_ = nullptr;
    IDWriteTextFormat* font_ = nullptr;
    IDWriteTextFormat* smallFont_ = nullptr;
    IDWriteTextFormat* titleFont_ = nullptr;

    State state_{};

    POINT mouse_{ -1000, -1000 };
    bool mouseDown_ = false;
    bool mousePressed_ = false;

    float scrollOffset_ = 0.0f;
    float maximumScroll_ = 0.0f;
    bool draggingScrollbar_ = false;
    float scrollbarGrabOffset_ = 0.0f;

    int activeSlider_ = -1;
    int openDropdown_ = -1;
    int* openDropdownSelection_ = nullptr;
    std::vector<std::wstring> openDropdownValues_;
    float dropdownX_ = 0.0f;
    float dropdownY_ = 0.0f;
    float dropdownWidth_ = 0.0f;

    bool settingsOpen_ = false;
    bool configNameFocused_ = false;
    std::wstring statusText_;

    // Hotkey capture state. Click a hotkey field, then press a key.
    int* hotkeyCaptureTarget_ = nullptr;
    std::wstring hotkeyCaptureLabel_;
    bool hotkeyCaptureArmed_ = false;
    ULONGLONG hotkeyCaptureStartedAt_ = 0;

    // Modal custom RGB color picker.
    bool colorPickerOpen_ = false;
    D2D1_COLOR_F pickerOriginal_{};
    D2D1_COLOR_F pickerWorking_{};
    D2D1_COLOR_F* pickerTarget_ = nullptr;
    int activeColorArea_ = -1; // 0 = saturation/value rectangle, 1 = hue strip
    float pickerHue_ = 0.78f;
    float pickerSaturation_ = 0.90f;
    float pickerValue_ = 0.95f;

    static LRESULT CALLBACK WindowProcedure(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    HRESULT CreateDeviceResources();
    void DiscardDeviceResources();
    void Render();

    void SetColor(D2D1_COLOR_F color);
    void Fill(float left, float top, float right, float bottom, D2D1_COLOR_F color);
    void Stroke(float left, float top, float right, float bottom,
                D2D1_COLOR_F color, float width = 1.0f);
    void Line(float x1, float y1, float x2, float y2,
              D2D1_COLOR_F color, float width = 1.0f);
    void DrawTextLine(
        const std::wstring& text,
        float x,
        float y,
        float width,
        float height,
        D2D1_COLOR_F color,
        IDWriteTextFormat* format = nullptr,
        DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING);

    bool Hover(float left, float top, float right, float bottom) const;
    bool Clicked(float left, float top, float right, float bottom) const;

    float ContentY(float logicalY) const;

    void DrawParticleBackground();
    void DrawHeader();
    void DrawGearIcon(float centerX, float centerY, D2D1_COLOR_F color);
    void DrawMainContent();
    void DrawSettingsContent();
    void DrawScrollbar(float contentHeight);
    void HandleScrollbarInput(float thumbTop, float thumbHeight);

    void Panel(float x, float logicalY, float width, float height, const wchar_t* title);
    void Checkbox(float x, float logicalY, const wchar_t* label,
                  bool& value, int* bindKey = nullptr);
    void Dropdown(float x, float logicalY, float width,
                  const wchar_t* label,
                  const std::vector<std::wstring>& values,
                  int& selected,
                  int dropdownId);
    void DrawDropdownOverlay();
    void Slider(float x, float logicalY, float width,
                const wchar_t* label,
                int& value,
                int minimum,
                int maximum,
                const wchar_t* suffix,
                int sliderId);
    void StaticValue(float x, float logicalY, float width,
                     const wchar_t* label, const wchar_t* value);
    void HotkeyValue(float x, float logicalY, float width,
                     const wchar_t* label, int& key);
    std::wstring KeyName(int encodedHotkey) const;
    std::wstring BaseKeyName(int virtualKey) const;
    int EncodeHotkey(int virtualKey) const;
    bool IsModifierKey(int virtualKey) const;
    bool Button(float x, float logicalY, float width,
                const wchar_t* label, bool accent = false);
    void TextInput(float x, float logicalY, float width,
                   const wchar_t* label, std::wstring& value);
    void ColorSwatchRow(float x, float logicalY, float width,
                        const wchar_t* label, D2D1_COLOR_F& color);
    void ToggleColorRow(float x, float logicalY, float width,
                        const wchar_t* label, bool& enabled,
                        D2D1_COLOR_F& color);
    void OpenColorPicker(D2D1_COLOR_F& color);
    void DrawColorPicker();
    void UpdatePickerColorFromHsv();

    std::filesystem::path ConfigDirectory() const;
    std::filesystem::path ConfigPath(const std::wstring& name) const;
    std::wstring SanitizeConfigName(const std::wstring& name) const;
    bool SaveConfig(const std::wstring& name);
    bool LoadConfig(const std::wstring& name);
    bool DeleteConfig(const std::wstring& name);
};

bool Application::Initialize(HINSTANCE instance)
{
    instance_ = instance;
    SetProcessDPIAware();

    if (FAILED(D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        &factory_)))
    {
        return false;
    }

    if (FAILED(DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&writeFactory_))))
    {
        return false;
    }

    auto createFont = [&](float size,
                          DWRITE_FONT_WEIGHT weight,
                          IDWriteTextFormat** output)
    {
        HRESULT result = writeFactory_->CreateTextFormat(
            L"Tahoma",
            nullptr,
            weight,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            size,
            L"en-US",
            output);

        if (SUCCEEDED(result))
        {
            (*output)->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            (*output)->SetParagraphAlignment(
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        return result;
    };

    if (FAILED(createFont(
        12.0f,
        DWRITE_FONT_WEIGHT_NORMAL,
        &font_)))
    {
        return false;
    }

    if (FAILED(createFont(
        10.0f,
        DWRITE_FONT_WEIGHT_NORMAL,
        &smallFont_)))
    {
        return false;
    }

    if (FAILED(createFont(
        13.0f,
        DWRITE_FONT_WEIGHT_NORMAL,
        &titleFont_)))
    {
        return false;
    }

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = instance_;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = L"CompactPurpleMenuUpdated";

    if (!RegisterClassExW(&windowClass))
        return false;

    const int x =
        (GetSystemMetrics(SM_CXSCREEN) - UI::Width) / 2;
    const int y =
        (GetSystemMetrics(SM_CYSCREEN) - UI::Height) / 2;

    window_ = CreateWindowExW(
        WS_EX_LAYERED,
        windowClass.lpszClassName,
        L"Compact Purple UI",
        WS_POPUP,
        x,
        y,
        UI::Width,
        UI::Height,
        nullptr,
        nullptr,
        instance_,
        this);

    if (!window_)
        return false;

    SetLayeredWindowAttributes(
        window_,
        0,
        255,
        LWA_ALPHA);

    MARGINS margins{ -1 };
    DwmExtendFrameIntoClientArea(window_, &margins);

    ShowWindow(window_, SW_SHOW);
    UpdateWindow(window_);
    SetTimer(window_, 1, 16, nullptr);
    return true;
}

int Application::Run()
{
    MSG message{};

    while (GetMessageW(
        &message,
        nullptr,
        0,
        0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}

void Application::Shutdown()
{
    DiscardDeviceResources();

    SafeRelease(titleFont_);
    SafeRelease(smallFont_);
    SafeRelease(font_);
    SafeRelease(writeFactory_);
    SafeRelease(factory_);
}

HRESULT Application::CreateDeviceResources()
{
    if (target_)
        return S_OK;

    RECT client{};
    GetClientRect(window_, &client);

    HRESULT result = factory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_HARDWARE,
            D2D1::PixelFormat(
                DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_PREMULTIPLIED)),
        D2D1::HwndRenderTargetProperties(
            window_,
            D2D1::SizeU(
                client.right - client.left,
                client.bottom - client.top),
            D2D1_PRESENT_OPTIONS_IMMEDIATELY),
        &target_);

    if (FAILED(result))
        return result;

    return target_->CreateSolidColorBrush(
        UI::Text,
        &brush_);
}

void Application::DiscardDeviceResources()
{
    SafeRelease(brush_);
    SafeRelease(target_);
}

void Application::SetColor(D2D1_COLOR_F color)
{
    brush_->SetColor(color);
}

void Application::Fill(
    float left,
    float top,
    float right,
    float bottom,
    D2D1_COLOR_F color)
{
    SetColor(color);
    target_->FillRectangle(
        D2D1::RectF(left, top, right, bottom),
        brush_);
}

void Application::Stroke(
    float left,
    float top,
    float right,
    float bottom,
    D2D1_COLOR_F color,
    float width)
{
    SetColor(color);
    target_->DrawRectangle(
        D2D1::RectF(left, top, right, bottom),
        brush_,
        width);
}

void Application::Line(
    float x1,
    float y1,
    float x2,
    float y2,
    D2D1_COLOR_F color,
    float width)
{
    SetColor(color);
    target_->DrawLine(
        D2D1::Point2F(x1, y1),
        D2D1::Point2F(x2, y2),
        brush_,
        width);
}

void Application::DrawTextLine(
    const std::wstring& text,
    float x,
    float y,
    float width,
    float height,
    D2D1_COLOR_F color,
    IDWriteTextFormat* format,
    DWRITE_TEXT_ALIGNMENT alignment)
{
    if (!format)
        format = font_;

    format->SetTextAlignment(alignment);
    SetColor(color);

    target_->DrawTextW(
        text.c_str(),
        static_cast<UINT32>(text.size()),
        format,
        D2D1::RectF(
            x,
            y,
            x + width,
            y + height),
        brush_,
        D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

bool Application::Hover(
    float left,
    float top,
    float right,
    float bottom) const
{
    return
        mouse_.x >= left &&
        mouse_.x <= right &&
        mouse_.y >= top &&
        mouse_.y <= bottom;
}

bool Application::Clicked(
    float left,
    float top,
    float right,
    float bottom) const
{
    return !colorPickerOpen_ &&
           mousePressed_ &&
           Hover(left, top, right, bottom);
}

float Application::ContentY(float logicalY) const
{
    return UI::HeaderHeight +
           logicalY -
           scrollOffset_;
}

void Application::DrawParticleBackground()
{
    if (!state_.particleBackground)
        return;

    const float time = static_cast<float>(GetTickCount64() % 100000ULL) / 1000.0f;
    constexpr int particleCount = 34;

    for (int index = 0; index < particleCount; ++index)
    {
        const float seed = static_cast<float>(index);
        const float baseX = std::fmod(seed * 83.0f + 29.0f, static_cast<float>(UI::Width));
        const float speed = 5.0f + std::fmod(seed * 3.7f, 11.0f);
        const float travel = std::fmod(time * speed + seed * 19.0f, static_cast<float>(UI::Height) + 50.0f);
        const float y = static_cast<float>(UI::Height) + 20.0f - travel;
        const float x = baseX + std::sin(time * 0.55f + seed) * 14.0f;
        const float radius = 0.8f + std::fmod(seed, 3.0f) * 0.45f;
        const float alpha = 0.12f + std::fmod(seed * 0.07f, 0.18f);

        SetColor(ColorFromHex(0x9718F7, alpha));
        target_->FillEllipse(
            D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius),
            brush_);

        if ((index % 4) == 0)
        {
            Line(x, y + 3.0f, x, y + 10.0f, ColorFromHex(0x9718F7, alpha * 0.45f), 0.7f);
        }
    }
}

void Application::DrawGearIcon(
    float centerX,
    float centerY,
    D2D1_COLOR_F color)
{
    SetColor(color);

    target_->DrawEllipse(
        D2D1::Ellipse(
            D2D1::Point2F(centerX, centerY),
            5.2f,
            5.2f),
        brush_,
        1.3f);

    target_->DrawEllipse(
        D2D1::Ellipse(
            D2D1::Point2F(centerX, centerY),
            1.8f,
            1.8f),
        brush_,
        1.3f);

    for (int index = 0; index < 8; ++index)
    {
        const float angle =
            6.28318530718f *
            static_cast<float>(index) /
            8.0f;

        const float x1 =
            centerX +
            std::cos(angle) * 6.2f;
        const float y1 =
            centerY +
            std::sin(angle) * 6.2f;

        const float x2 =
            centerX +
            std::cos(angle) * 8.0f;
        const float y2 =
            centerY +
            std::sin(angle) * 8.0f;

        Line(x1, y1, x2, y2, color, 1.3f);
    }
}

void Application::DrawHeader()
{
    Fill(
        0.0f,
        0.0f,
        static_cast<float>(UI::Width),
        UI::HeaderHeight,
        UI::Header);

    Line(
        0.0f,
        UI::HeaderHeight - 1.0f,
        static_cast<float>(UI::Width),
        UI::HeaderHeight - 1.0f,
        UI::Accent,
        1.0f);

    DrawTextLine(
        settingsOpen_ ? L"Configuration - HOTKEY COMBO FIX V8" : L"PlayHooked - HOTKEY COMBO FIX V8",
        10.0f,
        0.0f,
        170.0f,
        UI::HeaderHeight,
        UI::Text,
        titleFont_);

    if (!settingsOpen_)
    {
        DrawTextLine(
            L"(0x51D1)",
            93.0f,
            0.0f,
            78.0f,
            UI::HeaderHeight,
            UI::Muted,
            smallFont_);
    }

    const float gearLeft =
        static_cast<float>(UI::Width) - 47.0f;
    const float closeLeft =
        static_cast<float>(UI::Width) - 24.0f;

    const bool gearHover =
        Hover(
            gearLeft,
            5.0f,
            gearLeft + 20.0f,
            28.0f);

    const bool closeHover =
        Hover(
            closeLeft,
            5.0f,
            closeLeft + 18.0f,
            28.0f);

    if (gearHover)
    {
        Fill(
            gearLeft,
            5.0f,
            gearLeft + 20.0f,
            28.0f,
            ColorFromHex(0x24172C));
    }

    if (closeHover)
    {
        Fill(
            closeLeft,
            5.0f,
            closeLeft + 18.0f,
            28.0f,
            ColorFromHex(0x3A183F));
    }

    DrawGearIcon(
        gearLeft + 10.0f,
        16.5f,
        settingsOpen_
            ? UI::Accent
            : (gearHover
                ? UI::Text
                : UI::Muted));

    Line(
        closeLeft + 5.0f,
        11.0f,
        closeLeft + 13.0f,
        19.0f,
        closeHover
            ? UI::Text
            : UI::Muted,
        1.3f);

    Line(
        closeLeft + 13.0f,
        11.0f,
        closeLeft + 5.0f,
        19.0f,
        closeHover
            ? UI::Text
            : UI::Muted,
        1.3f);

    if (Clicked(
        gearLeft,
        5.0f,
        gearLeft + 20.0f,
        28.0f))
    {
        settingsOpen_ = !settingsOpen_;
        scrollOffset_ = 0.0f;
        openDropdown_ = -1;
        configNameFocused_ = false;
    }

    if (Clicked(
        closeLeft,
        5.0f,
        closeLeft + 18.0f,
        28.0f))
    {
        PostMessageW(
            window_,
            WM_CLOSE,
            0,
            0);
    }
}

void Application::Panel(
    float x,
    float logicalY,
    float width,
    float height,
    const wchar_t* title)
{
    const float y =
        ContentY(logicalY);

    Fill(
        x,
        y,
        x + width,
        y + height,
        UI::Panel);

    Stroke(
        x,
        y,
        x + width,
        y + height,
        UI::PanelBorder,
        1.0f);

    DrawTextLine(
        title,
        x + 7.0f,
        y + 2.0f,
        width - 14.0f,
        22.0f,
        UI::Text,
        titleFont_);
}

void Application::Checkbox(
    float x,
    float logicalY,
    const wchar_t* label,
    bool& value,
    int* bindKey)
{
    const float y = ContentY(logicalY);
    const float size = 10.0f;
    const float bindLeft = x + 137.0f;
    const float bindRight = x + 215.0f;

    const bool rowHover =
        Hover(x, y, x + 215.0f, y + 18.0f);

    const bool bindHover =
        bindKey != nullptr &&
        Hover(bindLeft, y, bindRight, y + 18.0f);

    if (!colorPickerOpen_ &&
        mousePressed_)
    {
        if (bindHover)
        {
            hotkeyCaptureTarget_ = bindKey;
            hotkeyCaptureLabel_ = label;
            hotkeyCaptureArmed_ = false;
            hotkeyCaptureStartedAt_ = GetTickCount64();
            configNameFocused_ = false;

            SetForegroundWindow(window_);
            SetFocus(window_);
        }
        else if (rowHover && hotkeyCaptureTarget_ == nullptr)
        {
            value = !value;
        }
    }

    if (value)
    {
        Fill(x, y + 4.0f, x + size, y + 14.0f, UI::Accent);
        Stroke(x, y + 4.0f, x + size, y + 14.0f, UI::AccentHover, 1.0f);
    }
    else
    {
        Fill(x, y + 4.0f, x + size, y + 14.0f, UI::Control);
        Stroke(
            x,
            y + 4.0f,
            x + size,
            y + 14.0f,
            rowHover ? UI::Accent : UI::PanelBorder,
            1.0f);
    }

    DrawTextLine(
        label,
        x + 19.0f,
        y,
        116.0f,
        18.0f,
        rowHover ? UI::Text : ColorFromHex(0xD4D4D8),
        smallFont_);

    if (bindKey)
    {
        const bool capturing = hotkeyCaptureTarget_ == bindKey;

        if (bindHover || capturing)
        {
            Fill(
                bindLeft,
                y + 1.0f,
                bindRight,
                y + 17.0f,
                capturing ? ColorFromHex(0x35134A) : UI::ControlHover);
        }

        DrawTextLine(
            capturing ? L"[hold modifiers + key]" : (L"[" + KeyName(*bindKey) + L"]"),
            bindLeft + 3.0f,
            y,
            bindRight - bindLeft - 6.0f,
            18.0f,
            capturing ? UI::AccentHover : (bindHover ? UI::Text : UI::Muted),
            smallFont_,
            DWRITE_TEXT_ALIGNMENT_TRAILING);
    }
}

void Application::Dropdown(
    float x,
    float logicalY,
    float width,
    const wchar_t* label,
    const std::vector<std::wstring>& values,
    int& selected,
    int dropdownId)
{
    selected =
        std::clamp(
            selected,
            0,
            static_cast<int>(values.size()) - 1);

    const float y =
        ContentY(logicalY);

    if (label[0] != L'\0')
    {
        DrawTextLine(
            label,
            x,
            y,
            width,
            16.0f,
            UI::Text,
            smallFont_);
    }

    const float boxY =
        y + 16.0f;
    const float boxHeight =
        24.0f;

    const bool hover =
        Hover(
            x,
            boxY,
            x + width,
            boxY + boxHeight);

    if (!colorPickerOpen_ && hover && mousePressed_)
    {
        if (openDropdown_ == dropdownId)
        {
            openDropdown_ = -1;
            openDropdownSelection_ = nullptr;
        }
        else
        {
            openDropdown_ = dropdownId;
            openDropdownSelection_ = &selected;
            openDropdownValues_ = values;
            dropdownX_ = x;
            dropdownY_ = boxY + boxHeight;
            dropdownWidth_ = width;
        }
    }

    Fill(
        x,
        boxY,
        x + width,
        boxY + boxHeight,
        hover
            ? UI::ControlHover
            : UI::Control);

    Stroke(
        x,
        boxY,
        x + width,
        boxY + boxHeight,
        openDropdown_ == dropdownId
            ? UI::Accent
            : (hover
                ? ColorFromHex(0x5D2B78)
                : UI::PanelBorder),
        1.0f);

    DrawTextLine(
        values[selected],
        x + 8.0f,
        boxY,
        width - 35.0f,
        boxHeight,
        UI::Text,
        smallFont_);

    const float arrowX =
        x + width - 14.0f;
    const float arrowY =
        boxY + 10.0f;

    Line(
        arrowX - 3.0f,
        arrowY,
        arrowX,
        arrowY + 3.0f,
        UI::Text,
        1.2f);

    Line(
        arrowX,
        arrowY + 3.0f,
        arrowX + 3.0f,
        arrowY,
        UI::Text,
        1.2f);
}

void Application::DrawDropdownOverlay()
{
    if (openDropdown_ < 0 ||
        openDropdownSelection_ == nullptr ||
        openDropdownValues_.empty())
    {
        openDropdown_ = -1;
        openDropdownSelection_ = nullptr;
        return;
    }

    const float itemHeight =
        23.0f;

    const float totalHeight =
        itemHeight *
        static_cast<float>(
            openDropdownValues_.size());

    Fill(
        dropdownX_,
        dropdownY_,
        dropdownX_ + dropdownWidth_,
        dropdownY_ + totalHeight,
        ColorFromHex(0x09090D));

    Stroke(
        dropdownX_,
        dropdownY_,
        dropdownX_ + dropdownWidth_,
        dropdownY_ + totalHeight,
        UI::Accent,
        1.0f);

    for (std::size_t index = 0;
         index < openDropdownValues_.size();
         ++index)
    {
        const float itemTop =
            dropdownY_ +
            itemHeight *
            static_cast<float>(index);

        const bool hover =
            Hover(
                dropdownX_,
                itemTop,
                dropdownX_ + dropdownWidth_,
                itemTop + itemHeight);

        if (hover)
        {
            Fill(
                dropdownX_ + 1.0f,
                itemTop + 1.0f,
                dropdownX_ + dropdownWidth_ - 1.0f,
                itemTop + itemHeight - 1.0f,
                UI::ControlHover);
        }

        // Cache the selected value before any click can close the popup.
        // This prevents dereferencing a null pointer later in this loop.
        const int selectedIndex =
            openDropdownSelection_
                ? *openDropdownSelection_
                : -1;

        const bool selected =
            static_cast<int>(index) ==
            selectedIndex;

        if (selected)
        {
            Fill(
                dropdownX_ + 1.0f,
                itemTop + 3.0f,
                dropdownX_ + 3.0f,
                itemTop + itemHeight - 3.0f,
                UI::Accent);
        }

        DrawTextLine(
            openDropdownValues_[index],
            dropdownX_ + 8.0f,
            itemTop,
            dropdownWidth_ - 16.0f,
            itemHeight,
            selected
                ? UI::AccentHover
                : UI::Text,
            smallFont_);

        if (hover &&
            mousePressed_ &&
            openDropdownSelection_)
        {
            *openDropdownSelection_ =
                static_cast<int>(index);

            openDropdown_ = -1;
            openDropdownSelection_ = nullptr;
            openDropdownValues_.clear();

            // Stop immediately. Continuing the loop after clearing the
            // pointer/vector caused the dropdown-selection crash.
            return;
        }
    }
}

void Application::Slider(
    float x,
    float logicalY,
    float width,
    const wchar_t* label,
    int& value,
    int minimum,
    int maximum,
    const wchar_t* suffix,
    int sliderId)
{
    const float y =
        ContentY(logicalY);

    if (label[0] != L'\0')
    {
        DrawTextLine(
            label,
            x,
            y,
            width * 0.58f,
            18.0f,
            UI::Text,
            smallFont_);
    }

    std::wstring valueText =
        std::to_wstring(value);

    valueText += suffix;

    DrawTextLine(
        valueText,
        x + width * 0.58f,
        y,
        width * 0.30f,
        18.0f,
        UI::Text,
        smallFont_,
        DWRITE_TEXT_ALIGNMENT_TRAILING);

    const float trackX = x;
    const float trackY = y + 18.0f;
    const float trackWidth = width;

    const bool trackHover =
        Hover(
            trackX - 2.0f,
            trackY - 6.0f,
            trackX + trackWidth + 2.0f,
            trackY + 7.0f);

    if (!colorPickerOpen_ &&
        trackHover &&
        mousePressed_ &&
        openDropdown_ < 0)
    {
        activeSlider_ = sliderId;
    }

    if (!mouseDown_ &&
        activeSlider_ == sliderId)
    {
        activeSlider_ = -1;
    }

    if (!colorPickerOpen_ &&
        mouseDown_ &&
        activeSlider_ == sliderId)
    {
        float amount =
            (static_cast<float>(mouse_.x) -
             trackX) /
            trackWidth;

        amount =
            std::clamp(
                amount,
                0.0f,
                1.0f);

        value =
            minimum +
            static_cast<int>(
                std::round(
                    amount *
                    static_cast<float>(
                        maximum - minimum)));
    }

    float amount =
        static_cast<float>(
            value - minimum) /
        static_cast<float>(
            maximum - minimum);

    amount =
        std::clamp(
            amount,
            0.0f,
            1.0f);

    Fill(
        trackX,
        trackY,
        trackX + trackWidth,
        trackY + 2.0f,
        ColorFromHex(0x24242B));

    Fill(
        trackX,
        trackY,
        trackX + trackWidth * amount,
        trackY + 2.0f,
        UI::Accent);

    const float knobX =
        trackX +
        trackWidth * amount;

    Fill(
        knobX - 3.0f,
        trackY - 3.0f,
        knobX + 3.0f,
        trackY + 5.0f,
        trackHover ||
        activeSlider_ == sliderId
            ? UI::AccentHover
            : UI::Accent);
}

void Application::StaticValue(
    float x,
    float logicalY,
    float width,
    const wchar_t* label,
    const wchar_t* value)
{
    const float y =
        ContentY(logicalY);

    DrawTextLine(
        label,
        x,
        y,
        width * 0.62f,
        18.0f,
        UI::Text,
        smallFont_);

    DrawTextLine(
        value,
        x + width * 0.62f,
        y,
        width * 0.38f,
        18.0f,
        UI::Muted,
        smallFont_,
        DWRITE_TEXT_ALIGNMENT_TRAILING);
}

bool Application::IsModifierKey(int virtualKey) const
{
    return virtualKey == VK_CONTROL ||
           virtualKey == VK_LCONTROL ||
           virtualKey == VK_RCONTROL ||
           virtualKey == VK_MENU ||
           virtualKey == VK_LMENU ||
           virtualKey == VK_RMENU ||
           virtualKey == VK_SHIFT ||
           virtualKey == VK_LSHIFT ||
           virtualKey == VK_RSHIFT;
}

int Application::EncodeHotkey(int virtualKey) const
{
    int encoded = virtualKey & HotkeyKeyMask;

    if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0)
        encoded |= HotkeyCtrlFlag;

    if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0)
        encoded |= HotkeyAltFlag;

    if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)
        encoded |= HotkeyShiftFlag;

    return encoded;
}

std::wstring Application::BaseKeyName(int virtualKey) const
{
    if (virtualKey == 0)
        return L"None";

    switch (virtualKey)
    {
    case VK_LBUTTON: return L"Mouse 1";
    case VK_RBUTTON: return L"Mouse 2";
    case VK_MBUTTON: return L"Mouse 3";
    case VK_XBUTTON1: return L"Mouse 4";
    case VK_XBUTTON2: return L"Mouse 5";
    case VK_INSERT: return L"Insert";
    case VK_DELETE: return L"Delete";
    case VK_HOME: return L"Home";
    case VK_END: return L"End";
    case VK_PRIOR: return L"Page Up";
    case VK_NEXT: return L"Page Down";
    case VK_SPACE: return L"Space";
    case VK_TAB: return L"Tab";
    case VK_RETURN: return L"Enter";
    case VK_ESCAPE: return L"Escape";
    default:
        break;
    }

    UINT scanCode = MapVirtualKeyW(
        static_cast<UINT>(virtualKey),
        MAPVK_VK_TO_VSC);

    if (virtualKey == VK_LEFT ||
        virtualKey == VK_UP ||
        virtualKey == VK_RIGHT ||
        virtualKey == VK_DOWN)
    {
        scanCode |= 0xE000;
    }

    wchar_t name[64]{};
    if (GetKeyNameTextW(
            static_cast<LONG>(scanCode << 16),
            name,
            static_cast<int>(std::size(name))) > 0)
    {
        return name;
    }

    return L"Key " + std::to_wstring(virtualKey);
}

std::wstring Application::KeyName(int encodedHotkey) const
{
    const int virtualKey =
        encodedHotkey & HotkeyKeyMask;

    if (virtualKey == 0)
        return L"None";

    std::wstring result;

    if ((encodedHotkey & HotkeyCtrlFlag) != 0)
        result += L"Ctrl+";

    if ((encodedHotkey & HotkeyAltFlag) != 0)
        result += L"Alt+";

    if ((encodedHotkey & HotkeyShiftFlag) != 0)
        result += L"Shift+";

    result += BaseKeyName(virtualKey);
    return result;
}

void Application::HotkeyValue(
    float x,
    float logicalY,
    float width,
    const wchar_t* label,
    int& key)
{
    const float y = ContentY(logicalY);
    const float fieldLeft = x + width * 0.58f;
    const bool hover = Hover(fieldLeft, y, x + width, y + 18.0f);
    const bool capturing = hotkeyCaptureTarget_ == &key;

    if (!colorPickerOpen_ &&
        openDropdown_ < 0 &&
        hover &&
        mousePressed_)
    {
        hotkeyCaptureTarget_ = &key;
        hotkeyCaptureLabel_ = label;
        hotkeyCaptureArmed_ = false;
        hotkeyCaptureStartedAt_ = GetTickCount64();
        configNameFocused_ = false;

        SetForegroundWindow(window_);
        SetFocus(window_);
    }

    DrawTextLine(
        label,
        x,
        y,
        width * 0.58f,
        18.0f,
        UI::Text,
        smallFont_);

    if (hover || capturing)
    {
        Fill(
            fieldLeft,
            y + 1.0f,
            x + width,
            y + 17.0f,
            capturing ? ColorFromHex(0x35134A) : UI::ControlHover);
    }

    DrawTextLine(
        capturing ? L"[hold modifiers + key]" : (L"[" + KeyName(key) + L"]"),
        fieldLeft + 4.0f,
        y,
        width * 0.42f - 8.0f,
        18.0f,
        capturing ? UI::AccentHover : (hover ? UI::Text : UI::Muted),
        smallFont_,
        DWRITE_TEXT_ALIGNMENT_TRAILING);
}


bool Application::Button(
    float x,
    float logicalY,
    float width,
    const wchar_t* label,
    bool accent)
{
    const float y =
        ContentY(logicalY);

    const float height =
        26.0f;

    const bool hover =
        Hover(
            x,
            y,
            x + width,
            y + height);

    Fill(
        x,
        y,
        x + width,
        y + height,
        accent
            ? (hover
                ? UI::AccentHover
                : UI::Accent)
            : (hover
                ? UI::ControlHover
                : UI::Control));

    Stroke(
        x,
        y,
        x + width,
        y + height,
        accent
            ? UI::AccentHover
            : UI::PanelBorder,
        1.0f);

    DrawTextLine(
        label,
        x,
        y,
        width,
        height,
        UI::Text,
        smallFont_,
        DWRITE_TEXT_ALIGNMENT_CENTER);

    return Clicked(
        x,
        y,
        x + width,
        y + height);
}

void Application::TextInput(
    float x,
    float logicalY,
    float width,
    const wchar_t* label,
    std::wstring& value)
{
    const float y =
        ContentY(logicalY);

    DrawTextLine(
        label,
        x,
        y,
        width,
        17.0f,
        UI::Text,
        smallFont_);

    const float boxY =
        y + 17.0f;

    const bool hover =
        Hover(
            x,
            boxY,
            x + width,
            boxY + 25.0f);

    if (!colorPickerOpen_ && hover && mousePressed_)
        configNameFocused_ = true;

    Fill(
        x,
        boxY,
        x + width,
        boxY + 25.0f,
        UI::Control);

    Stroke(
        x,
        boxY,
        x + width,
        boxY + 25.0f,
        configNameFocused_
            ? UI::Accent
            : (hover
                ? ColorFromHex(0x5D2B78)
                : UI::PanelBorder),
        1.0f);

    DrawTextLine(
        value.empty()
            ? L"config name"
            : value,
        x + 7.0f,
        boxY,
        width - 14.0f,
        25.0f,
        value.empty()
            ? UI::Muted
            : UI::Text,
        smallFont_);
}

void Application::ColorSwatchRow(
    float x,
    float logicalY,
    float width,
    const wchar_t* label,
    D2D1_COLOR_F& color)
{
    const float y = ContentY(logicalY);
    const float height = 30.0f;
    const float swatchWidth = 28.0f;
    const float swatchHeight = 12.0f;
    const float swatchLeft = x + width - swatchWidth;
    const float swatchTop = y + (height - swatchHeight) * 0.5f;
    const float swatchBottom = swatchTop + swatchHeight;

    const bool hover = Hover(x, y, x + width, y + height);
    const bool swatchHover = Hover(
        swatchLeft,
        swatchTop,
        x + width,
        swatchBottom);

    DrawTextLine(
        label,
        x,
        y,
        width - swatchWidth - 12.0f,
        height,
        hover ? UI::Text : ColorFromHex(0xD4D4D8),
        smallFont_);

    Fill(
        swatchLeft,
        swatchTop,
        x + width,
        swatchBottom,
        color);

    if (!colorPickerOpen_ && swatchHover && mousePressed_)
        OpenColorPicker(color);
}


void Application::ToggleColorRow(
    float x,
    float logicalY,
    float width,
    const wchar_t* label,
    bool& enabled,
    D2D1_COLOR_F& color)
{
    const float y = ContentY(logicalY);
    const float height = 30.0f;
    const float checkSize = 10.0f;
    const float swatchWidth = 26.0f;
    const float swatchHeight = 12.0f;
    const float swatchLeft = x + width - swatchWidth;
    const float swatchTop = y + (height - swatchHeight) * 0.5f;
    const float swatchBottom = swatchTop + swatchHeight;

    const bool rowHover = Hover(x, y, x + width, y + height);
    const bool swatchHover = Hover(
        swatchLeft, swatchTop, x + width, swatchBottom);
    const bool toggleHover = Hover(
        x, y, swatchLeft - 8.0f, y + height);

    if (!colorPickerOpen_ && mousePressed_)
    {
        if (swatchHover)
            OpenColorPicker(color);
        else if (toggleHover && openDropdown_ < 0)
            enabled = !enabled;
    }

    if (enabled)
    {
        Fill(x, y + 10.0f, x + checkSize, y + 20.0f, UI::Accent);
        Stroke(x, y + 10.0f, x + checkSize, y + 20.0f, UI::AccentHover, 1.0f);
    }
    else
    {
        Fill(x, y + 10.0f, x + checkSize, y + 20.0f, UI::Control);
        Stroke(x, y + 10.0f, x + checkSize, y + 20.0f,
               toggleHover ? UI::Accent : UI::PanelBorder, 1.0f);
    }

    DrawTextLine(
        label, x + 18.0f, y, width - swatchWidth - 30.0f, height,
        rowHover ? UI::Text : ColorFromHex(0xD4D4D8), smallFont_);

    Fill(
        swatchLeft, swatchTop, x + width, swatchBottom, color);
}

void Application::OpenColorPicker(D2D1_COLOR_F& color)
{
    pickerTarget_ = &color;
    pickerOriginal_ = color;
    pickerWorking_ = color;
    ColorToHsv(color, pickerHue_, pickerSaturation_, pickerValue_);
    colorPickerOpen_ = true;
    activeColorArea_ = -1;
    openDropdown_ = -1;
    openDropdownSelection_ = nullptr;
    configNameFocused_ = false;
}

void Application::UpdatePickerColorFromHsv()
{
    pickerWorking_ = HsvToColor(
        pickerHue_,
        pickerSaturation_,
        pickerValue_);
}

void Application::DrawColorPicker()
{
    if (!colorPickerOpen_ || !pickerTarget_)
        return;

    Fill(
        0.0f,
        UI::HeaderHeight,
        static_cast<float>(UI::Width),
        static_cast<float>(UI::Height),
        ColorFromHex(0x000000, 0.70f));

    const float boxWidth = 350.0f;
    const float boxHeight = 292.0f;
    const float x = (static_cast<float>(UI::Width) - boxWidth) * 0.5f;
    const float y = UI::HeaderHeight +
        (static_cast<float>(UI::Height) - UI::HeaderHeight - boxHeight) * 0.5f;

    Fill(x, y, x + boxWidth, y + boxHeight, ColorFromHex(0x0D0D13));
    Stroke(x, y, x + boxWidth, y + boxHeight, UI::Accent, 1.0f);

    DrawTextLine(
        L"ESP color",
        x + 15.0f,
        y + 3.0f,
        boxWidth - 30.0f,
        31.0f,
        UI::Text,
        titleFont_);

    Line(
        x + 14.0f,
        y + 37.0f,
        x + boxWidth - 14.0f,
        y + 37.0f,
        UI::PanelBorder,
        1.0f);

    const float paletteX = x + 18.0f;
    const float paletteY = y + 53.0f;
    const float paletteWidth = 265.0f;
    const float paletteHeight = 174.0f;
    const float hueX = paletteX + paletteWidth + 10.0f;
    const float hueWidth = 18.0f;

    // Saturation/value rectangle. Horizontal = saturation, vertical = value.
    constexpr int columns = 64;
    constexpr int rows = 42;
    const float cellWidth = paletteWidth / static_cast<float>(columns);
    const float cellHeight = paletteHeight / static_cast<float>(rows);

    for (int row = 0; row < rows; ++row)
    {
        const float value = 1.0f -
            static_cast<float>(row) / static_cast<float>(rows - 1);

        for (int column = 0; column < columns; ++column)
        {
            const float saturation =
                static_cast<float>(column) / static_cast<float>(columns - 1);

            const float left = paletteX + cellWidth * static_cast<float>(column);
            const float top = paletteY + cellHeight * static_cast<float>(row);

            Fill(
                left,
                top,
                left + cellWidth + 0.8f,
                top + cellHeight + 0.8f,
                HsvToColor(pickerHue_, saturation, value));
        }
    }

    // Vertical hue strip.
    constexpr int hueSteps = 90;
    const float hueStepHeight = paletteHeight / static_cast<float>(hueSteps);
    for (int step = 0; step < hueSteps; ++step)
    {
        const float hue = static_cast<float>(step) /
                          static_cast<float>(hueSteps - 1);
        const float top = paletteY + hueStepHeight * static_cast<float>(step);
        Fill(
            hueX,
            top,
            hueX + hueWidth,
            top + hueStepHeight + 0.8f,
            HsvToColor(hue, 1.0f, 1.0f));
    }

    const bool paletteHover = Hover(
        paletteX,
        paletteY,
        paletteX + paletteWidth,
        paletteY + paletteHeight);

    const bool hueHover = Hover(
        hueX,
        paletteY,
        hueX + hueWidth,
        paletteY + paletteHeight);

    if (mousePressed_ && paletteHover)
        activeColorArea_ = 0;
    else if (mousePressed_ && hueHover)
        activeColorArea_ = 1;

    if (mouseDown_ && activeColorArea_ == 0)
    {
        pickerSaturation_ = std::clamp(
            (static_cast<float>(mouse_.x) - paletteX) / paletteWidth,
            0.0f,
            1.0f);

        pickerValue_ = 1.0f - std::clamp(
            (static_cast<float>(mouse_.y) - paletteY) / paletteHeight,
            0.0f,
            1.0f);

        UpdatePickerColorFromHsv();
    }
    else if (mouseDown_ && activeColorArea_ == 1)
    {
        pickerHue_ = std::clamp(
            (static_cast<float>(mouse_.y) - paletteY) / paletteHeight,
            0.0f,
            1.0f);

        UpdatePickerColorFromHsv();
    }

    // Selection point in the main rectangle.
    const float pointX = paletteX + pickerSaturation_ * paletteWidth;
    const float pointY = paletteY + (1.0f - pickerValue_) * paletteHeight;

    SetColor(ColorFromHex(0x000000));
    target_->DrawEllipse(
        D2D1::Ellipse(D2D1::Point2F(pointX, pointY), 5.2f, 5.2f),
        brush_,
        3.0f);
    SetColor(ColorFromHex(0xFFFFFF));
    target_->DrawEllipse(
        D2D1::Ellipse(D2D1::Point2F(pointX, pointY), 5.2f, 5.2f),
        brush_,
        1.4f);

    // Hue marker.
    const float hueMarkerY = paletteY + pickerHue_ * paletteHeight;
    Fill(
        hueX - 3.0f,
        hueMarkerY - 2.0f,
        hueX + hueWidth + 3.0f,
        hueMarkerY + 2.0f,
        ColorFromHex(0xFFFFFF));
    Stroke(
        hueX - 4.0f,
        hueMarkerY - 3.0f,
        hueX + hueWidth + 4.0f,
        hueMarkerY + 3.0f,
        ColorFromHex(0x000000),
        1.0f);

    // Small current-color preview and clean HEX text.
    Fill(
        x + 18.0f,
        y + 239.0f,
        x + 43.0f,
        y + 258.0f,
        pickerWorking_);

    wchar_t hexText[16]{};
    const int red = std::clamp(
        static_cast<int>(std::round(pickerWorking_.r * 255.0f)), 0, 255);
    const int green = std::clamp(
        static_cast<int>(std::round(pickerWorking_.g * 255.0f)), 0, 255);
    const int blue = std::clamp(
        static_cast<int>(std::round(pickerWorking_.b * 255.0f)), 0, 255);
    swprintf_s(hexText, L"#%02X%02X%02X", red, green, blue);

    DrawTextLine(
        hexText,
        x + 51.0f,
        y + 232.0f,
        112.0f,
        31.0f,
        UI::Text,
        smallFont_);

    const float buttonY = y + boxHeight - 36.0f;
    const float cancelX = x + boxWidth - 150.0f;
    const float applyX = x + boxWidth - 76.0f;

    const bool cancelHover = Hover(
        cancelX, buttonY, cancelX + 64.0f, buttonY + 24.0f);
    const bool applyHover = Hover(
        applyX, buttonY, applyX + 58.0f, buttonY + 24.0f);

    Fill(
        cancelX,
        buttonY,
        cancelX + 64.0f,
        buttonY + 24.0f,
        cancelHover ? UI::ControlHover : UI::Control);
    Stroke(
        cancelX,
        buttonY,
        cancelX + 64.0f,
        buttonY + 24.0f,
        UI::PanelBorder,
        1.0f);
    DrawTextLine(
        L"Cancel",
        cancelX,
        buttonY,
        64.0f,
        24.0f,
        UI::Text,
        smallFont_,
        DWRITE_TEXT_ALIGNMENT_CENTER);

    Fill(
        applyX,
        buttonY,
        applyX + 58.0f,
        buttonY + 24.0f,
        applyHover ? UI::AccentHover : UI::Accent);
    DrawTextLine(
        L"Apply",
        applyX,
        buttonY,
        58.0f,
        24.0f,
        UI::Text,
        smallFont_,
        DWRITE_TEXT_ALIGNMENT_CENTER);

    if (mousePressed_ && cancelHover)
    {
        *pickerTarget_ = pickerOriginal_;
        colorPickerOpen_ = false;
        pickerTarget_ = nullptr;
        activeColorArea_ = -1;
    }
    else if (mousePressed_ && applyHover)
    {
        *pickerTarget_ = pickerWorking_;
        colorPickerOpen_ = false;
        pickerTarget_ = nullptr;
        activeColorArea_ = -1;
    }
}

void Application::DrawMainContent()
{
    constexpr float leftX =
        UI::Margin;

    constexpr float rightX =
        UI::Margin +
        UI::ColumnWidth +
        UI::Gap;

    constexpr float width =
        UI::ColumnWidth;

    Panel(
        leftX,
        10.0f,
        width,
        92.0f,
        L"Weapon group");

    Dropdown(
        leftX + 30.0f,
        35.0f,
        190.0f,
        L"",
        {
            L"SSG",
            L"AWP",
            L"Rifle",
            L"Pistol"
        },
        state_.weaponGroup,
        1);

    Checkbox(
        leftX + 14.0f,
        78.0f,
        L"Override shared",
        state_.overrideShared);

    Panel(
        leftX,
        111.0f,
        width,
        346.0f,
        L"Aimbot");

    Checkbox(
        leftX + 14.0f,
        139.0f,
        L"Enabled",
        state_.enabled,
        &state_.enabledHotkey);

    Dropdown(
        leftX + 30.0f,
        161.0f,
        190.0f,
        L"Hitboxes",
        {
            L"Head",
            L"Body",
            L"Nearest"
        },
        state_.hitbox,
        2);

    Checkbox(
        leftX + 14.0f,
        213.0f,
        L"Multipoint",
        state_.multipoint);

    Slider(
        leftX + 30.0f,
        234.0f,
        190.0f,
        L"",
        state_.multipointValue,
        0,
        100,
        L"%",
        1);

    Slider(
        leftX + 30.0f,
        267.0f,
        190.0f,
        L"Hitchance",
        state_.hitchance,
        0,
        100,
        L"%",
        2);

    Checkbox(
        leftX + 14.0f,
        300.0f,
        L"Force shoot",
        state_.forceShot,
        &state_.forceShotHotkey);

    Slider(
        leftX + 30.0f,
        323.0f,
        190.0f,
        L"Min damage",
        state_.minDamage,
        0,
        100,
        L" HP",
        3);

    HotkeyValue(
        leftX + 30.0f,
        356.0f,
        190.0f,
        L"Damage override",
        state_.damageOverrideHotkey);

    Checkbox(
        leftX + 14.0f,
        380.0f,
        L"Enable override",
        state_.damageOverride);

    Checkbox(
        leftX + 14.0f,
        405.0f,
        L"Auto stop",
        state_.autoStop);

    Panel(
        rightX,
        10.0f,
        width,
        180.0f,
        L"Accuracy");

    Checkbox(
        rightX + 14.0f,
        41.0f,
        L"Silent",
        state_.silent);

    Checkbox(
        rightX + 14.0f,
        65.0f,
        L"Autowall",
        state_.autowall);

    Checkbox(
        rightX + 14.0f,
        90.0f,
        L"Doubletap",
        state_.doubletap,
        &state_.doubletapHotkey);

    Checkbox(
        rightX + 14.0f,
        114.0f,
        L"Nospread",
        state_.nospread,
        &state_.nospreadHotkey);

    Slider(
        rightX + 30.0f,
        139.0f,
        194.0f,
        L"Field of view",
        state_.fieldOfView,
        0,
        180,
        L" deg",
        4);

    Panel(
        rightX,
        199.0f,
        width,
        284.0f,
        L"Antiaim");

    Dropdown(
        rightX + 30.0f,
        226.0f,
        194.0f,
        L"Yaw",
        {
            L"At targets",
            L"Backward",
            L"Spin",
            L"None"
        },
        state_.yaw,
        3);

    Slider(
        rightX + 30.0f,
        276.0f,
        194.0f,
        L"Yaw offset",
        state_.yawValue,
        -180,
        180,
        L" deg",
        5);

    StaticValue(
        rightX + 30.0f,
        310.0f,
        194.0f,
        L"Spin",
        L"[None]");

    Dropdown(
        rightX + 30.0f,
        333.0f,
        194.0f,
        L"Yaw jitter",
        {
            L"None",
            L"Offset",
            L"Center",
            L"Random"
        },
        state_.yawJitter,
        4);

    Dropdown(
        rightX + 30.0f,
        382.0f,
        194.0f,
        L"Move",
        {
            L"None",
            L"Static",
            L"Jitter"
        },
        state_.move,
        5);

    HotkeyValue(
        rightX + 30.0f,
        432.0f,
        194.0f,
        L"Move override",
        state_.moveOverrideHotkey);

    Checkbox(
        rightX + 14.0f,
        452.0f,
        L"Fake duck",
        state_.fakeDuck);

    // Full-width ESP card. Every feature has its own toggle and color swatch.
    Panel(
        UI::Margin,
        495.0f,
        static_cast<float>(UI::Width) - UI::Margin * 2.0f - 10.0f,
        146.0f,
        L"ESP");

    const float espLeft = UI::Margin + 18.0f;
    const float espRight = UI::Margin + 270.0f;
    const float espRowWidth = 220.0f;

    ToggleColorRow(espLeft, 526.0f, espRowWidth,
                   L"Box", state_.espBox, state_.espRectangleColor);
    ToggleColorRow(espLeft, 558.0f, espRowWidth,
                   L"Health bar", state_.espHealthBar, state_.espHealthColor);
    ToggleColorRow(espLeft, 590.0f, espRowWidth,
                   L"Name", state_.espName, state_.espNameColor);

    ToggleColorRow(espRight, 526.0f, espRowWidth,
                   L"Skeleton", state_.espSkeleton, state_.espSkeletonColor);
    ToggleColorRow(espRight, 558.0f, espRowWidth,
                   L"Weapon", state_.espWeapon, state_.espWeaponColor);
    ToggleColorRow(espRight, 590.0f, espRowWidth,
                   L"Distance", state_.espDistance, state_.espDistanceColor);
}


std::filesystem::path Application::ConfigDirectory() const
{
    wchar_t appData[MAX_PATH]{};
    const DWORD length =
        GetEnvironmentVariableW(
            L"APPDATA",
            appData,
            MAX_PATH);

    std::filesystem::path directory =
        length > 0
            ? std::filesystem::path(appData) /
                  L"CompactPurpleMenu" /
                  L"Configs"
            : std::filesystem::current_path() /
                  L"Configs";

    std::error_code error;
    std::filesystem::create_directories(
        directory,
        error);

    return directory;
}

std::wstring Application::SanitizeConfigName(
    const std::wstring& name) const
{
    std::wstring clean;
    clean.reserve(name.size());

    for (wchar_t character : name)
    {
        const bool invalid =
            character < 32 ||
            character == L'<' ||
            character == L'>' ||
            character == L':' ||
            character == L'"' ||
            character == L'/' ||
            character == L'\\' ||
            character == L'|' ||
            character == L'?' ||
            character == L'*';

        if (!invalid)
            clean.push_back(character);
    }

    while (!clean.empty() &&
           (clean.front() == L' ' ||
            clean.front() == L'.'))
    {
        clean.erase(clean.begin());
    }

    while (!clean.empty() &&
           (clean.back() == L' ' ||
            clean.back() == L'.'))
    {
        clean.pop_back();
    }

    if (clean.size() > 32)
        clean.resize(32);

    return clean;
}

std::filesystem::path Application::ConfigPath(
    const std::wstring& name) const
{
    return ConfigDirectory() /
        (SanitizeConfigName(name) + L".cfg");
}

bool Application::SaveConfig(
    const std::wstring& name)
{
    const std::wstring cleanName =
        SanitizeConfigName(name);

    if (cleanName.empty())
        return false;

    std::ofstream file(
        ConfigPath(cleanName),
        std::ios::binary |
        std::ios::trunc);

    if (!file)
        return false;

    const std::uint32_t magic = 0x43505549;
    const std::uint32_t version = 2;

    auto writeValue =
        [&](const auto& value)
        {
            file.write(
                reinterpret_cast<const char*>(&value),
                sizeof(value));
        };

    auto writeColor =
        [&](const D2D1_COLOR_F& color)
        {
            writeValue(color.r);
            writeValue(color.g);
            writeValue(color.b);
            writeValue(color.a);
        };

    writeValue(magic);
    writeValue(version);

    writeValue(state_.overrideShared);
    writeValue(state_.enabled);
    writeValue(state_.multipoint);
    writeValue(state_.forceShot);
    writeValue(state_.damageOverride);
    writeValue(state_.autoStop);

    writeValue(state_.silent);
    writeValue(state_.autowall);
    writeValue(state_.doubletap);
    writeValue(state_.nospread);
    writeValue(state_.fakeDuck);

    writeValue(state_.saveWindowPosition);
    writeValue(state_.compactText);
    writeValue(state_.menuAnimation);
    writeValue(state_.particleBackground);

    writeValue(state_.espBox);
    writeValue(state_.espHealthBar);
    writeValue(state_.espName);
    writeValue(state_.espSkeleton);
    writeValue(state_.espWeapon);
    writeValue(state_.espDistance);

    writeColor(state_.espRectangleColor);
    writeColor(state_.espHealthColor);
    writeColor(state_.espNameColor);
    writeColor(state_.espSkeletonColor);
    writeColor(state_.espWeaponColor);
    writeColor(state_.espDistanceColor);

    writeValue(state_.weaponGroup);
    writeValue(state_.hitbox);
    writeValue(state_.yaw);
    writeValue(state_.yawJitter);
    writeValue(state_.move);

    writeValue(state_.multipointValue);
    writeValue(state_.hitchance);
    writeValue(state_.minDamage);
    writeValue(state_.fieldOfView);
    writeValue(state_.yawValue);

    writeValue(state_.enabledHotkey);
    writeValue(state_.forceShotHotkey);
    writeValue(state_.damageOverrideHotkey);
    writeValue(state_.doubletapHotkey);
    writeValue(state_.nospreadHotkey);
    writeValue(state_.moveOverrideHotkey);

    file.flush();
    return file.good();
}

bool Application::LoadConfig(
    const std::wstring& name)
{
    const std::wstring cleanName =
        SanitizeConfigName(name);

    if (cleanName.empty())
        return false;

    std::ifstream file(
        ConfigPath(cleanName),
        std::ios::binary);

    if (!file)
        return false;

    State loaded = state_;

    auto readValue =
        [&](auto& value) -> bool
        {
            file.read(
                reinterpret_cast<char*>(&value),
                sizeof(value));

            return file.good();
        };

    auto readColor =
        [&](D2D1_COLOR_F& color) -> bool
        {
            return
                readValue(color.r) &&
                readValue(color.g) &&
                readValue(color.b) &&
                readValue(color.a);
        };

    std::uint32_t magic = 0;
    std::uint32_t version = 0;

    if (!readValue(magic) ||
        !readValue(version) ||
        magic != 0x43505549 ||
        version != 2)
    {
        return false;
    }

#define READ_CONFIG_VALUE(member) \
    if (!readValue(loaded.member)) return false

    READ_CONFIG_VALUE(overrideShared);
    READ_CONFIG_VALUE(enabled);
    READ_CONFIG_VALUE(multipoint);
    READ_CONFIG_VALUE(forceShot);
    READ_CONFIG_VALUE(damageOverride);
    READ_CONFIG_VALUE(autoStop);

    READ_CONFIG_VALUE(silent);
    READ_CONFIG_VALUE(autowall);
    READ_CONFIG_VALUE(doubletap);
    READ_CONFIG_VALUE(nospread);
    READ_CONFIG_VALUE(fakeDuck);

    READ_CONFIG_VALUE(saveWindowPosition);
    READ_CONFIG_VALUE(compactText);
    READ_CONFIG_VALUE(menuAnimation);
    READ_CONFIG_VALUE(particleBackground);

    READ_CONFIG_VALUE(espBox);
    READ_CONFIG_VALUE(espHealthBar);
    READ_CONFIG_VALUE(espName);
    READ_CONFIG_VALUE(espSkeleton);
    READ_CONFIG_VALUE(espWeapon);
    READ_CONFIG_VALUE(espDistance);

    if (!readColor(loaded.espRectangleColor) ||
        !readColor(loaded.espHealthColor) ||
        !readColor(loaded.espNameColor) ||
        !readColor(loaded.espSkeletonColor) ||
        !readColor(loaded.espWeaponColor) ||
        !readColor(loaded.espDistanceColor))
    {
        return false;
    }

    READ_CONFIG_VALUE(weaponGroup);
    READ_CONFIG_VALUE(hitbox);
    READ_CONFIG_VALUE(yaw);
    READ_CONFIG_VALUE(yawJitter);
    READ_CONFIG_VALUE(move);

    READ_CONFIG_VALUE(multipointValue);
    READ_CONFIG_VALUE(hitchance);
    READ_CONFIG_VALUE(minDamage);
    READ_CONFIG_VALUE(fieldOfView);
    READ_CONFIG_VALUE(yawValue);

    READ_CONFIG_VALUE(enabledHotkey);
    READ_CONFIG_VALUE(forceShotHotkey);
    READ_CONFIG_VALUE(damageOverrideHotkey);
    READ_CONFIG_VALUE(doubletapHotkey);
    READ_CONFIG_VALUE(nospreadHotkey);
    READ_CONFIG_VALUE(moveOverrideHotkey);

#undef READ_CONFIG_VALUE

    loaded.weaponGroup =
        std::clamp(loaded.weaponGroup, 0, 3);
    loaded.hitbox =
        std::clamp(loaded.hitbox, 0, 2);
    loaded.yaw =
        std::clamp(loaded.yaw, 0, 3);
    loaded.yawJitter =
        std::clamp(loaded.yawJitter, 0, 3);
    loaded.move =
        std::clamp(loaded.move, 0, 2);

    loaded.multipointValue =
        std::clamp(loaded.multipointValue, 0, 100);
    loaded.hitchance =
        std::clamp(loaded.hitchance, 0, 100);
    loaded.minDamage =
        std::clamp(loaded.minDamage, 0, 100);
    loaded.fieldOfView =
        std::clamp(loaded.fieldOfView, 0, 180);
    loaded.yawValue =
        std::clamp(loaded.yawValue, -180, 180);

    loaded.configName = cleanName;
    state_ = loaded;

    openDropdown_ = -1;
    openDropdownSelection_ = nullptr;
    hotkeyCaptureTarget_ = nullptr;
    colorPickerOpen_ = false;

    InvalidateRect(
        window_,
        nullptr,
        FALSE);

    return true;
}

bool Application::DeleteConfig(
    const std::wstring& name)
{
    const std::wstring cleanName =
        SanitizeConfigName(name);

    if (cleanName.empty())
        return false;

    std::error_code error;
    const bool removed =
        std::filesystem::remove(
            ConfigPath(cleanName),
            error);

    return removed && !error;
}


void Application::DrawSettingsContent()
{
    const float panelX =
        UI::Margin;

    const float panelWidth =
        static_cast<float>(UI::Width) -
        UI::Margin * 2.0f -
        10.0f;

    Panel(
        panelX,
        10.0f,
        panelWidth,
        238.0f,
        L"Config manager");

    TextInput(
        panelX + 18.0f,
        43.0f,
        panelWidth - 36.0f,
        L"Configuration name",
        state_.configName);

    if (Button(
        panelX + 18.0f,
        95.0f,
        145.0f,
        L"Create / Save",
        true))
    {
        const std::wstring cleanName =
            SanitizeConfigName(
                state_.configName);

        if (cleanName.empty())
        {
            statusText_ =
                L"Enter a valid config name.";
        }
        else if (SaveConfig(cleanName))
        {
            state_.configName = cleanName;
            statusText_ =
                L"Saved to AppData successfully.";
        }
        else
        {
            statusText_ =
                L"Save failed. Check folder permissions.";
        }
    }

    if (Button(
        panelX + 173.0f,
        95.0f,
        145.0f,
        L"Load"))
    {
        const std::wstring cleanName =
            SanitizeConfigName(
                state_.configName);

        if (cleanName.empty())
        {
            statusText_ =
                L"Enter a valid config name.";
        }
        else if (LoadConfig(cleanName))
        {
            statusText_ =
                L"Configuration loaded successfully.";
        }
        else
        {
            statusText_ =
                L"Config not found or invalid.";
        }
    }

    if (Button(
        panelX + 328.0f,
        95.0f,
        145.0f,
        L"Delete"))
    {
        const std::wstring cleanName =
            SanitizeConfigName(
                state_.configName);

        if (cleanName.empty())
        {
            statusText_ =
                L"Enter a valid config name.";
        }
        else if (DeleteConfig(cleanName))
        {
            statusText_ =
                L"Configuration deleted.";
        }
        else
        {
            statusText_ =
                L"Config file was not found.";
        }
    }

    StaticValue(
        panelX + 18.0f,
        134.0f,
        panelWidth - 36.0f,
        L"Current config",
        state_.configName.empty()
            ? L"none"
            : state_.configName.c_str());

    StaticValue(
        panelX + 18.0f,
        161.0f,
        panelWidth - 36.0f,
        L"Storage",
        L"%APPDATA%\\CompactPurpleMenu\\Configs");

    DrawTextLine(
        statusText_.empty()
            ? L"Type a name, then choose an action."
            : statusText_,
        panelX + 18.0f,
        ContentY(195.0f),
        panelWidth - 36.0f,
        24.0f,
        statusText_.empty()
            ? UI::Muted
            : UI::AccentHover,
        smallFont_);

    Panel(
        panelX,
        259.0f,
        panelWidth,
        232.0f,
        L"Interface");

    Checkbox(
        panelX + 18.0f,
        291.0f,
        L"Save window position",
        state_.saveWindowPosition);

    Checkbox(
        panelX + 18.0f,
        321.0f,
        L"Compact text",
        state_.compactText);

    Checkbox(
        panelX + 18.0f,
        351.0f,
        L"Menu animation",
        state_.menuAnimation);

    Checkbox(
        panelX + 18.0f,
        381.0f,
        L"Particle background",
        state_.particleBackground);

    ColorSwatchRow(
        panelX + 18.0f,
        408.0f,
        panelWidth - 36.0f,
        L"ESP rectangle color",
        state_.espRectangleColor);

    DrawTextLine(
        L"Click the color rectangle to open the custom RGB picker.",
        panelX + 18.0f,
        ContentY(443.0f),
        panelWidth - 36.0f,
        22.0f,
        UI::Muted,
        smallFont_);
}

void Application::HandleScrollbarInput(
    float thumbTop,
    float thumbHeight)
{
    const float trackTop =
        UI::HeaderHeight + 6.0f;

    const float trackBottom =
        static_cast<float>(UI::Height) - 6.0f;

    const float trackHeight =
        trackBottom - trackTop;

    if (Clicked(
        static_cast<float>(UI::Width) - 10.0f,
        thumbTop,
        static_cast<float>(UI::Width) - 4.0f,
        thumbTop + thumbHeight))
    {
        draggingScrollbar_ = true;
        scrollbarGrabOffset_ =
            static_cast<float>(mouse_.y) -
            thumbTop;
    }

    if (draggingScrollbar_ && mouseDown_)
    {
        float newTop =
            static_cast<float>(mouse_.y) -
            scrollbarGrabOffset_;

        newTop =
            std::clamp(
                newTop,
                trackTop,
                trackBottom - thumbHeight);

        const float available =
            trackHeight - thumbHeight;

        const float fraction =
            available > 0.0f
                ? (newTop - trackTop) /
                  available
                : 0.0f;

        scrollOffset_ =
            fraction *
            maximumScroll_;
    }

    if (!mouseDown_)
        draggingScrollbar_ = false;
}

void Application::DrawScrollbar(float contentHeight)
{
    maximumScroll_ =
        std::max(
            0.0f,
            contentHeight -
            UI::ViewHeight);

    scrollOffset_ =
        std::clamp(
            scrollOffset_,
            0.0f,
            maximumScroll_);

    if (maximumScroll_ <= 0.0f)
        return;

    const float trackLeft =
        static_cast<float>(UI::Width) - 10.0f;

    const float trackRight =
        static_cast<float>(UI::Width) - 4.0f;

    const float trackTop =
        UI::HeaderHeight + 6.0f;

    const float trackBottom =
        static_cast<float>(UI::Height) - 6.0f;

    const float trackHeight =
        trackBottom - trackTop;

    const float visibleRatio =
        std::clamp(
            UI::ViewHeight /
            contentHeight,
            0.12f,
            1.0f);

    const float thumbHeight =
        trackHeight *
        visibleRatio;

    const float scrollRatio =
        maximumScroll_ > 0.0f
            ? scrollOffset_ /
              maximumScroll_
            : 0.0f;

    const float thumbTop =
        trackTop +
        (trackHeight - thumbHeight) *
        scrollRatio;

    Fill(
        trackLeft,
        trackTop,
        trackRight,
        trackBottom,
        UI::ScrollTrack);

    Fill(
        trackLeft,
        thumbTop,
        trackRight,
        thumbTop + thumbHeight,
        draggingScrollbar_
            ? UI::AccentHover
            : UI::Accent);

    HandleScrollbarInput(
        thumbTop,
        thumbHeight);
}

void Application::Render()
{
    if (FAILED(CreateDeviceResources()))
        return;

    target_->BeginDraw();
    target_->Clear(UI::Background);

    DrawParticleBackground();
    DrawHeader();

    target_->PushAxisAlignedClip(
        D2D1::RectF(
            0.0f,
            UI::HeaderHeight,
            static_cast<float>(UI::Width),
            static_cast<float>(UI::Height)),
        D2D1_ANTIALIAS_MODE_ALIASED);

    if (settingsOpen_)
        DrawSettingsContent();
    else
        DrawMainContent();

    target_->PopAxisAlignedClip();

    DrawScrollbar(
        settingsOpen_
            ? 535.0f
            : 685.0f);

    DrawDropdownOverlay();
    DrawColorPicker();

    const HRESULT result =
        target_->EndDraw();

    if (result == D2DERR_RECREATE_TARGET)
        DiscardDeviceResources();

    mousePressed_ = false;
}

LRESULT CALLBACK Application::WindowProcedure(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    Application* app = nullptr;

    if (message == WM_NCCREATE)
    {
        auto* create =
            reinterpret_cast<CREATESTRUCTW*>(
                lParam);

        app =
            static_cast<Application*>(
                create->lpCreateParams);

        SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(app));

        app->window_ = hwnd;
    }
    else
    {
        app =
            reinterpret_cast<Application*>(
                GetWindowLongPtrW(
                    hwnd,
                    GWLP_USERDATA));
    }

    if (app)
    {
        return app->HandleMessage(
            message,
            wParam,
            lParam);
    }

    return DefWindowProcW(
        hwnd,
        message,
        wParam,
        lParam);
}

LRESULT Application::HandleMessage(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_ERASEBKGND:
        return 1;

    case WM_TIMER:
        // Timer 1 drives both menu animation and reliable hotkey capture.
        if (state_.particleBackground || state_.menuAnimation)
        {
            InvalidateRect(window_, nullptr, FALSE);
        }

        if (hotkeyCaptureTarget_ &&
            hotkeyCaptureArmed_)
        {
            for (int virtualKey = 1;
                 virtualKey <= 255;
                 ++virtualKey)
            {
                if (IsModifierKey(virtualKey))
                    continue;

                if (virtualKey == VK_LBUTTON &&
                    GetTickCount64() -
                        hotkeyCaptureStartedAt_ < 220)
                {
                    continue;
                }

                if ((GetAsyncKeyState(virtualKey) &
                     0x0001) != 0)
                {
                    if (virtualKey == VK_ESCAPE ||
                        virtualKey == VK_BACK ||
                        virtualKey == VK_DELETE)
                    {
                        *hotkeyCaptureTarget_ = 0;
                    }
                    else
                    {
                        *hotkeyCaptureTarget_ =
                            EncodeHotkey(virtualKey);
                    }

                    hotkeyCaptureTarget_ = nullptr;
                    hotkeyCaptureLabel_.clear();
                    hotkeyCaptureArmed_ = false;

                    InvalidateRect(
                        window_,
                        nullptr,
                        FALSE);

                    break;
                }
            }
        }

        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT paint{};
        BeginPaint(window_, &paint);
        Render();
        EndPaint(window_, &paint);
        return 0;
    }

    case WM_MOUSEMOVE:
        mouse_.x =
            GET_X_LPARAM(lParam);

        mouse_.y =
            GET_Y_LPARAM(lParam);

        InvalidateRect(
            window_,
            nullptr,
            FALSE);

        return 0;

    case WM_RBUTTONDOWN:
        if (hotkeyCaptureTarget_ && hotkeyCaptureArmed_)
        {
            *hotkeyCaptureTarget_ = EncodeHotkey(VK_RBUTTON);
            hotkeyCaptureTarget_ = nullptr;
            hotkeyCaptureLabel_.clear();
            hotkeyCaptureArmed_ = false;
            InvalidateRect(window_, nullptr, FALSE);
            return 0;
        }
        break;

    case WM_MBUTTONDOWN:
        if (hotkeyCaptureTarget_ && hotkeyCaptureArmed_)
        {
            *hotkeyCaptureTarget_ = EncodeHotkey(VK_MBUTTON);
            hotkeyCaptureTarget_ = nullptr;
            hotkeyCaptureLabel_.clear();
            hotkeyCaptureArmed_ = false;
            InvalidateRect(window_, nullptr, FALSE);
            return 0;
        }
        break;

    case WM_XBUTTONDOWN:
        if (hotkeyCaptureTarget_ && hotkeyCaptureArmed_)
        {
            *hotkeyCaptureTarget_ =
                EncodeHotkey(
                    GET_XBUTTON_WPARAM(wParam) == XBUTTON1
                        ? VK_XBUTTON1
                        : VK_XBUTTON2);

            hotkeyCaptureTarget_ = nullptr;
            hotkeyCaptureLabel_.clear();
            hotkeyCaptureArmed_ = false;
            InvalidateRect(window_, nullptr, FALSE);
            return TRUE;
        }
        break;

    case WM_MOUSEWHEEL:
    {
        if (colorPickerOpen_)
            return 0;

        const int delta =
            GET_WHEEL_DELTA_WPARAM(wParam);

        scrollOffset_ -=
            static_cast<float>(delta) /
            static_cast<float>(WHEEL_DELTA) *
            36.0f;

        scrollOffset_ =
            std::clamp(
                scrollOffset_,
                0.0f,
                maximumScroll_);

        openDropdown_ = -1;
        openDropdownSelection_ = nullptr;

        InvalidateRect(
            window_,
            nullptr,
            FALSE);

        return 0;
    }

    case WM_LBUTTONDOWN:
        if (hotkeyCaptureTarget_ && hotkeyCaptureArmed_)
        {
            *hotkeyCaptureTarget_ = EncodeHotkey(VK_LBUTTON);
            hotkeyCaptureTarget_ = nullptr;
            hotkeyCaptureLabel_.clear();
            hotkeyCaptureArmed_ = false;
            InvalidateRect(window_, nullptr, FALSE);
            return 0;
        }

        SetCapture(window_);

        mouseDown_ = true;
        mousePressed_ = true;

        mouse_.x =
            GET_X_LPARAM(lParam);

        mouse_.y =
            GET_Y_LPARAM(lParam);

        if (openDropdown_ >= 0)
        {
            const float itemHeight = 23.0f;
            const float popupHeight =
                itemHeight *
                static_cast<float>(openDropdownValues_.size());

            const bool insidePopup =
                Hover(
                    dropdownX_,
                    dropdownY_,
                    dropdownX_ + dropdownWidth_,
                    dropdownY_ + popupHeight);

            const bool insideSource =
                Hover(
                    dropdownX_,
                    dropdownY_ - 24.0f,
                    dropdownX_ + dropdownWidth_,
                    dropdownY_);

            if (!insidePopup && !insideSource)
            {
                openDropdown_ = -1;
                openDropdownSelection_ = nullptr;
                openDropdownValues_.clear();
            }
        }

        if (!colorPickerOpen_ &&
            mouse_.y <=
                UI::HeaderHeight &&
            mouse_.x <
                UI::Width - 58)
        {
            ReleaseCapture();

            SendMessageW(
                window_,
                WM_NCLBUTTONDOWN,
                HTCAPTION,
                0);

            return 0;
        }

        InvalidateRect(
            window_,
            nullptr,
            FALSE);

        return 0;

    case WM_LBUTTONUP:
        ReleaseCapture();

        mouseDown_ = false;
        draggingScrollbar_ = false;
        activeSlider_ = -1;

        if (hotkeyCaptureTarget_ && !hotkeyCaptureArmed_)
        {
            // The click used to select the hotkey field is now finished.
            // Future keyboard or mouse input can safely be captured.
            hotkeyCaptureArmed_ = true;

            // Clear the transition bit from the selection click.
            GetAsyncKeyState(VK_LBUTTON);
        }

        InvalidateRect(
            window_,
            nullptr,
            FALSE);

        return 0;

    case WM_CHAR:
        if (configNameFocused_)
        {
            const wchar_t character =
                static_cast<wchar_t>(wParam);

            if (character == L'\b')
            {
                if (!state_.configName.empty())
                    state_.configName.pop_back();
            }
            else if (character == L'\r')
            {
                configNameFocused_ = false;
            }
            else if (character >= 32 &&
                     character != L'/' &&
                     character != L'\\' &&
                     character != L':' &&
                     character != L'*' &&
                     character != L'?' &&
                     character != L'"' &&
                     character != L'<' &&
                     character != L'>' &&
                     character != L'|' &&
                     state_.configName.size() < 24)
            {
                state_.configName.push_back(
                    character);
            }

            InvalidateRect(
                window_,
                nullptr,
                FALSE);
        }

        return 0;

    case WM_SIZE:
        if (target_)
        {
            const UINT width =
                LOWORD(lParam);

            const UINT height =
                HIWORD(lParam);

            target_->Resize(
                D2D1::SizeU(
                    width,
                    height));
        }

        return 0;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        if (hotkeyCaptureTarget_)
        {
            const int virtualKey =
                static_cast<int>(wParam);

            // Modifier keys are allowed as part of a combination.
            // Do not finish capture until a non-modifier key is pressed.
            if (IsModifierKey(virtualKey))
            {
                InvalidateRect(
                    window_,
                    nullptr,
                    FALSE);

                return 0;
            }

            if (virtualKey == VK_ESCAPE ||
                virtualKey == VK_BACK ||
                virtualKey == VK_DELETE)
            {
                *hotkeyCaptureTarget_ = 0;
            }
            else
            {
                *hotkeyCaptureTarget_ =
                    EncodeHotkey(virtualKey);
            }

            hotkeyCaptureTarget_ = nullptr;
            hotkeyCaptureLabel_.clear();
            hotkeyCaptureArmed_ = false;

            InvalidateRect(
                window_,
                nullptr,
                FALSE);

            return 0;
        }

        if (wParam == VK_ESCAPE)
        {
            if (colorPickerOpen_)
            {
                if (pickerTarget_)
                    *pickerTarget_ = pickerOriginal_;
                colorPickerOpen_ = false;
                pickerTarget_ = nullptr;
                activeColorArea_ = -1;
            }
            else if (openDropdown_ >= 0)
            {
                openDropdown_ = -1;
                openDropdownSelection_ = nullptr;
            }
            else if (settingsOpen_)
            {
                settingsOpen_ = false;
                scrollOffset_ = 0.0f;
            }
            else
            {
                PostMessageW(
                    window_,
                    WM_CLOSE,
                    0,
                    0);
            }

            InvalidateRect(
                window_,
                nullptr,
                FALSE);
        }

        return 0;

    case WM_CLOSE:
        DestroyWindow(window_);
        return 0;

    case WM_DESTROY:
        KillTimer(window_, 1);
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(
            window_,
            message,
            wParam,
            lParam);
    }
}

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int)
{
    Application app;

    if (!app.Initialize(instance))
    {
        MessageBoxW(
            nullptr,
            L"Failed to initialize the Direct2D interface.",
            L"Error",
            MB_OK | MB_ICONERROR);

        return 1;
    }

    const int result =
        app.Run();

    app.Shutdown();
    return result;
}
