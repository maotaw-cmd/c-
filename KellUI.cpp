// ============================================================================
// Dark Direct2D menu mockup - single-file C++17 / Win32 / Direct2D
// Dark transparent full-width Direct2D menu UI with independent Aimbot/Visuals controls.
//
// Build (Visual Studio x64, Unicode, Windows subsystem):
//   d2d1.lib
//   dwrite.lib
//
// Notes:
// - This is a UI template only.
// - Drag the top bar to move the window.
// - Esc closes the window. Backspace clears a keybind while capturing.
// - The old left navbar and version label were removed.
// - UI demonstration only: no game process, memory, aiming, or overlay integration.
// - Visual preview removed; top save/settings buttons use large reliable hit areas.
// ============================================================================

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include <windows.h>
#include <windowsx.h>
#include <d2d1.h>
#include <dwrite.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

template <typename T>
static void SafeRelease(T*& value)
{
    if (value)
    {
        value->Release();
        value = nullptr;
    }
}

static float Clamp(float value, float minimum, float maximum)
{
    return std::max(minimum, std::min(value, maximum));
}

static bool Hit(float x, float y, float left, float top, float right, float bottom)
{
    return x >= left && x <= right && y >= top && y <= bottom;
}

static D2D1_COLOR_F MakeColor(std::uint32_t rgb, float alpha = 1.0f)
{
    return D2D1::ColorF(
        static_cast<float>((rgb >> 16) & 0xFF) / 255.0f,
        static_cast<float>((rgb >> 8) & 0xFF) / 255.0f,
        static_cast<float>(rgb & 0xFF) / 255.0f,
        alpha);
}

static D2D1_COLOR_F WithAlpha(const D2D1_COLOR_F& color, float alpha)
{
    return D2D1::ColorF(color.r, color.g, color.b, alpha);
}

static D2D1_COLOR_F BlendColor(
    const D2D1_COLOR_F& base,
    const D2D1_COLOR_F& tint,
    float amount)
{
    amount = Clamp(amount, 0.0f, 1.0f);
    const float inverse = 1.0f - amount;

    return D2D1::ColorF(
        base.r * inverse + tint.r * amount,
        base.g * inverse + tint.g * amount,
        base.b * inverse + tint.b * amount,
        1.0f);
}

static D2D1_COLOR_F HsvToColor(float hue, float saturation, float value)
{
    hue = hue - std::floor(hue);
    saturation = Clamp(saturation, 0.0f, 1.0f);
    value = Clamp(value, 0.0f, 1.0f);

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
    constexpr int Width = 796;
    constexpr int Height = 600;

    constexpr float SidebarWidth = 0.0f;
    constexpr float TopBarHeight = 70.0f;
    constexpr float ContentPadding = 18.0f;
    constexpr float DragHeight = 34.0f;
}

namespace Theme
{
    const D2D1_COLOR_F Window = MakeColor(0x010203);
    const D2D1_COLOR_F TopBar = MakeColor(0x020304);
    const D2D1_COLOR_F Sidebar = MakeColor(0x08090B);
    const D2D1_COLOR_F SidebarBorder = MakeColor(0x15171A);

    const D2D1_COLOR_F Card = MakeColor(0x07090C, 0.96f);
    const D2D1_COLOR_F CardBorder = MakeColor(0x111317);
    const D2D1_COLOR_F CardInner = MakeColor(0x0D0F12);
    const D2D1_COLOR_F RowHover = MakeColor(0x111418, 0.72f);
    const D2D1_COLOR_F Divider = MakeColor(0x17191D, 0.8f);

    const D2D1_COLOR_F Text = MakeColor(0xE6E6E6);
    const D2D1_COLOR_F Muted = MakeColor(0x7D8189);
    const D2D1_COLOR_F Muted2 = MakeColor(0x5E636A);

    const D2D1_COLOR_F Accent = MakeColor(0xB31622);
    const D2D1_COLOR_F AccentSoft = MakeColor(0x63141B, 0.42f);
    const D2D1_COLOR_F AccentBright = MakeColor(0xD62936);
    const D2D1_COLOR_F SliderKnob = MakeColor(0x393939);
    const D2D1_COLOR_F Input = MakeColor(0x090B0E);
    const D2D1_COLOR_F InputHover = MakeColor(0x101318);
}

namespace Icon
{
    constexpr const wchar_t* Search = L"\xE721";
    constexpr const wchar_t* Gear = L"\xE713";
    constexpr const wchar_t* Mouse = L"\xE962";
    constexpr const wchar_t* Skull = L"\xE9A1";
    constexpr const wchar_t* Anti = L"\xE72D";
    constexpr const wchar_t* Eye = L"\xE890";
    constexpr const wchar_t* Menu = L"\xE700";
    constexpr const wchar_t* People = L"\xE716";
    constexpr const wchar_t* Phone = L"\xE717";
    constexpr const wchar_t* Code = L"\xE943";
    constexpr const wchar_t* Folder = L"\xE8B7";
    constexpr const wchar_t* Save = L"\xE74E";
    constexpr const wchar_t* Load = L"\xE72C";
    constexpr const wchar_t* Delete = L"\xE74D";
    constexpr const wchar_t* Add = L"\xE710";
    constexpr const wchar_t* File = L"\xE8A5";
    constexpr const wchar_t* Check = L"\xE73E";
    constexpr const wchar_t* Chevron = L"\xE70D";
    constexpr const wchar_t* Help = L"\xE897";
    constexpr const wchar_t* Drop = L"\xE790";
}

constexpr int HotkeyKeyMask = 0x00FF;
constexpr int HotkeyCtrlFlag = 0x0100;
constexpr int HotkeyAltFlag = 0x0200;
constexpr int HotkeyShiftFlag = 0x0400;

struct NavItem
{
    std::wstring label;
    const wchar_t* icon;
};

class App
{
public:
    bool Initialize(HINSTANCE instance);
    int Run();
    void Shutdown();

private:

    struct DropdownOverlay
    {
        bool valid = false;
        float buttonLeft = 0.0f;
        float buttonTop = 0.0f;
        float buttonRight = 0.0f;
        float buttonBottom = 0.0f;
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float itemHeight = 28.0f;
        std::vector<std::wstring> items;
        int* selected = nullptr;
    };

    HWND hwnd_ = nullptr;
    ID2D1Factory* d2dFactory_ = nullptr;
    ID2D1HwndRenderTarget* target_ = nullptr;
    ID2D1SolidColorBrush* brush_ = nullptr;
    IDWriteFactory* writeFactory_ = nullptr;
    IDWriteTextFormat* font_ = nullptr;
    IDWriteTextFormat* smallFont_ = nullptr;
    IDWriteTextFormat* titleFont_ = nullptr;
    IDWriteTextFormat* logoFont_ = nullptr;
    IDWriteTextFormat* iconFont_ = nullptr;

    POINT mouse_{ -1000, -1000 };
    bool mouseDown_ = false;
    bool mousePressed_ = false;

    // Aimbot page values. These are UI settings only.
    bool aimEnabled_ = false;
    bool silentAim_ = false;
    bool teamCheck_ = true;
    bool visibleOnly_ = true;
    bool prediction_ = false;
    float aimFov_ = 18.0f;
    float aimSmooth_ = 6.0f;
    float aimDistance_ = 250.0f;
    int targetBone_ = 0;

    // Independent visual options. Enabling one never changes another.
    bool espBox_ = true;
    bool espCornerBox_ = false;
    bool espHealthBar_ = true;
    bool espName_ = true;
    bool espDistance_ = false;
    bool espSnapline_ = false;
    bool espSkeleton_ = false;
    bool espWeapon_ = false;

    D2D1_COLOR_F boxColor_ = MakeColor(0xFFFFFF);
    D2D1_COLOR_F healthColor_ = MakeColor(0x42D66B);
    D2D1_COLOR_F lineColor_ = MakeColor(0xFFFFFF);
    D2D1_COLOR_F skeletonColor_ = MakeColor(0xD5DBFF);

    bool dropdownOpen_ = false;
    DropdownOverlay dropdownOverlay_{};
    int activeSlider_ = -1;

    bool settingsOpen_ = false;
    bool configOpen_ = false;
    bool particlesEnabled_ = true;
    float particleSpeed_ = 22.0f;
    float particleAmount_ = 34.0f;

    std::wstring configName_;
    bool configNameFocused_ = false;
    std::vector<std::wstring> configs_;
    int selectedConfig_ = -1;
    float configScroll_ = 0.0f;
    std::wstring configStatus_;
    bool configStatusOk_ = true;
    float configStatusTime_ = 0.0f;

    // Window tint and UI accent are independent color settings.
    D2D1_COLOR_F backgroundColor_ = MakeColor(0xFFFFFF);
    D2D1_COLOR_F accentColor_ = MakeColor(0xB31622);

    int aimbotKeybind_ = 0;
    bool keybindCapture_ = false;
    bool hotkeyRegistered_ = false;

    bool colorPickerOpen_ = false;
    D2D1_COLOR_F* pickerTarget_ = nullptr;
    D2D1_COLOR_F pickerOriginal_{};
    D2D1_COLOR_F pickerWorking_{};
    float pickerHue_ = 0.0f;
    float pickerSaturation_ = 0.0f;
    float pickerValue_ = 1.0f;
    int activeColorArea_ = -1; // 0 = saturation/value, 1 = hue


    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

    HRESULT CreateFactories();
    HRESULT CreateDeviceResources();
    void DiscardDeviceResources();

    void Render();
    void DrawShell(float width, float height);
    void DrawTopBar(float width);
    void DrawCards();
    void DrawSettingsPage();
    void DrawConfigPage();
    void DrawParticles(float width, float height);

    void DrawAimbotCard(float x, float y, float w, float h);
    void DrawVisualsCard(float x, float y, float w, float h);

    void SetBrush(D2D1_COLOR_F color);
    void FillRect(float left, float top, float right, float bottom, D2D1_COLOR_F color);
    void DrawRect(float left, float top, float right, float bottom, D2D1_COLOR_F color, float stroke = 1.0f);
    void FillRound(float left, float top, float right, float bottom, float radius, D2D1_COLOR_F color);
    void DrawRound(float left, float top, float right, float bottom, float radius, D2D1_COLOR_F color, float stroke = 1.0f);
    void DrawLine(float x1, float y1, float x2, float y2, D2D1_COLOR_F color, float stroke = 1.0f);
    void DrawTextLine(const std::wstring& text, float left, float top, float right, float bottom,
        D2D1_COLOR_F color, IDWriteTextFormat* format = nullptr,
        DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING);
    void DrawIcon(const wchar_t* icon, float left, float top, float right, float bottom, D2D1_COLOR_F color);
    void DrawTriangleLogo(float x, float y);

    bool Hover(float left, float top, float right, float bottom) const;
    D2D1_COLOR_F AccentBright() const;
    D2D1_COLOR_F AccentSoft(float alpha = 0.22f) const;
    void Card(float x, float y, float w, float h, const std::wstring& title);
    void Divider(float x, float y, float w);
    void Toggle(const std::wstring& label, bool& value, float x, float y, float w);
    void Slider(const std::wstring& label, float& value, float minimum, float maximum,
        bool isFloat, float x, float y, float w, int sliderId);
    void Dropdown(const std::wstring& label, const std::vector<std::wstring>& items, int& selected, float x, float y, float w);
    void DrawDropdownOverlay();
    void ColorRow(const std::wstring& label, D2D1_COLOR_F& color, float x, float y, float w, bool second = false);
    void KeybindControl(const std::wstring& label, int& encodedKey, float x, float y, float w);
    bool Button(const std::wstring& label, const wchar_t* icon, float x, float y, float w, float h, bool accent = false, bool danger = false);
    void TextInput(const std::wstring& label, std::wstring& value, bool& focused, float x, float y, float w);
    std::filesystem::path ConfigDirectory() const;
    std::filesystem::path ConfigPath(const std::wstring& name) const;
    std::wstring CleanConfigName(const std::wstring& name) const;
    bool SaveConfig(const std::wstring& name);
    bool LoadConfig(const std::wstring& name);
    bool DeleteConfig(const std::wstring& name);
    void RefreshConfigs();
    void ShowConfigStatus(const std::wstring& text, bool ok);
    std::wstring BaseKeyName(int virtualKey) const;
    std::wstring KeybindName(int encodedKey) const;
    int EncodeHotkey(int virtualKey) const;
    bool IsModifierKey(int virtualKey) const;
    bool MatchesHotkey(int encodedKey, int virtualKey) const;
    void UpdateRegisteredHotkey();
    void OpenColorPicker(D2D1_COLOR_F& color);
    void UpdatePickerColorFromHsv();
    void DrawColorPicker();
};

HRESULT App::CreateFactories()
{
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory_);
    if (FAILED(hr))
        return hr;

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&writeFactory_));
    if (FAILED(hr))
        return hr;

    auto createFont = [&](const wchar_t* family, float size, DWRITE_FONT_WEIGHT weight, IDWriteTextFormat** out)
        {
            HRESULT result = writeFactory_->CreateTextFormat(
                family,
                nullptr,
                weight,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                size,
                L"en-US",
                out);

            if (SUCCEEDED(result))
            {
                (*out)->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                (*out)->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            }

            return result;
        };

    if (FAILED(createFont(L"Segoe UI", 12.0f, DWRITE_FONT_WEIGHT_NORMAL, &font_)))
        return E_FAIL;
    if (FAILED(createFont(L"Segoe UI", 10.0f, DWRITE_FONT_WEIGHT_NORMAL, &smallFont_)))
        return E_FAIL;
    if (FAILED(createFont(L"Segoe UI", 12.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, &titleFont_)))
        return E_FAIL;
    if (FAILED(createFont(L"Segoe UI", 19.0f, DWRITE_FONT_WEIGHT_BOLD, &logoFont_)))
        return E_FAIL;
    if (FAILED(createFont(L"Segoe MDL2 Assets", 15.0f, DWRITE_FONT_WEIGHT_NORMAL, &iconFont_)))
        return E_FAIL;

    return S_OK;
}

HRESULT App::CreateDeviceResources()
{
    if (target_)
        return S_OK;

    RECT client{};
    GetClientRect(hwnd_, &client);

    HRESULT hr = d2dFactory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_HARDWARE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
        D2D1::HwndRenderTargetProperties(
            hwnd_,
            D2D1::SizeU(client.right - client.left, client.bottom - client.top),
            D2D1_PRESENT_OPTIONS_IMMEDIATELY),
        &target_);

    if (FAILED(hr))
        return hr;

    return target_->CreateSolidColorBrush(Theme::Text, &brush_);
}

void App::DiscardDeviceResources()
{
    SafeRelease(brush_);
    SafeRelease(target_);
}

bool App::Initialize(HINSTANCE instance)
{
    SetProcessDPIAware();

    if (FAILED(CreateFactories()))
        return false;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = StaticWindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"EvictedIndependentVisualMenuV2";
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hIconSm = wc.hIcon;

    if (!RegisterClassExW(&wc))
        return false;

    const int x = (GetSystemMetrics(SM_CXSCREEN) - UI::Width) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - UI::Height) / 2;

    hwnd_ = CreateWindowExW(
        WS_EX_APPWINDOW | WS_EX_LAYERED,
        wc.lpszClassName,
        L"Direct2D Menu UI",
        WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU,
        x,
        y,
        UI::Width,
        UI::Height,
        nullptr,
        nullptr,
        instance,
        this);

    if (!hwnd_)
        return false;

    // Uniform layered alpha gives the entire dark window a subtle transparent look.
    SetLayeredWindowAttributes(hwnd_, 0, 222, LWA_ALPHA);

    HRGN region = CreateRoundRectRgn(0, 0, UI::Width + 1, UI::Height + 1, 10, 10);
    SetWindowRgn(hwnd_, region, TRUE);

    RefreshConfigs();

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    SetTimer(hwnd_, 1, 16, nullptr);
    return true;
}

int App::Run()
{
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

void App::Shutdown()
{
    if (hwnd_)
    {
        KillTimer(hwnd_, 1);
        UnregisterHotKey(hwnd_, 1);
    }

    DiscardDeviceResources();
    SafeRelease(iconFont_);
    SafeRelease(logoFont_);
    SafeRelease(titleFont_);
    SafeRelease(smallFont_);
    SafeRelease(font_);
    SafeRelease(writeFactory_);
    SafeRelease(d2dFactory_);
}

void App::SetBrush(D2D1_COLOR_F color)
{
    brush_->SetColor(color);
}

void App::FillRect(float left, float top, float right, float bottom, D2D1_COLOR_F color)
{
    SetBrush(color);
    target_->FillRectangle(D2D1::RectF(left, top, right, bottom), brush_);
}

void App::DrawRect(float left, float top, float right, float bottom, D2D1_COLOR_F color, float stroke)
{
    SetBrush(color);
    target_->DrawRectangle(D2D1::RectF(left, top, right, bottom), brush_, stroke);
}

void App::FillRound(float left, float top, float right, float bottom, float radius, D2D1_COLOR_F color)
{
    SetBrush(color);
    target_->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), radius, radius), brush_);
}

void App::DrawRound(float left, float top, float right, float bottom, float radius, D2D1_COLOR_F color, float stroke)
{
    SetBrush(color);
    target_->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), radius, radius), brush_, stroke);
}

void App::DrawLine(float x1, float y1, float x2, float y2, D2D1_COLOR_F color, float stroke)
{
    SetBrush(color);
    target_->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush_, stroke);
}

void App::DrawTextLine(const std::wstring& text, float left, float top, float right, float bottom,
    D2D1_COLOR_F color, IDWriteTextFormat* format, DWRITE_TEXT_ALIGNMENT alignment)
{
    if (!format)
        format = font_;

    format->SetTextAlignment(alignment);
    SetBrush(color);
    target_->DrawTextW(
        text.c_str(),
        static_cast<UINT32>(text.size()),
        format,
        D2D1::RectF(left, top, right, bottom),
        brush_,
        D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

void App::DrawIcon(const wchar_t* icon, float left, float top, float right, float bottom, D2D1_COLOR_F color)
{
    DrawTextLine(icon, left, top, right, bottom, color, iconFont_, DWRITE_TEXT_ALIGNMENT_CENTER);
}

bool App::Hover(float left, float top, float right, float bottom) const
{
    return Hit(static_cast<float>(mouse_.x), static_cast<float>(mouse_.y), left, top, right, bottom);
}

D2D1_COLOR_F App::AccentBright() const
{
    return D2D1::ColorF(
        Clamp(accentColor_.r * 1.22f + 0.03f, 0.0f, 1.0f),
        Clamp(accentColor_.g * 1.22f + 0.03f, 0.0f, 1.0f),
        Clamp(accentColor_.b * 1.22f + 0.03f, 0.0f, 1.0f),
        1.0f);
}

D2D1_COLOR_F App::AccentSoft(float alpha) const
{
    return WithAlpha(accentColor_, alpha);
}

void App::DrawTriangleLogo(float x, float y)
{
    DrawLine(x + 2.0f, y + 21.0f, x + 12.5f, y + 3.0f, Theme::Muted, 1.7f);
    DrawLine(x + 12.5f, y + 3.0f, x + 23.0f, y + 21.0f, Theme::Muted, 1.7f);
    DrawLine(x + 23.0f, y + 21.0f, x + 2.0f, y + 21.0f, Theme::Muted, 1.7f);

    DrawLine(x + 7.0f, y + 18.0f, x + 12.5f, y + 9.0f, Theme::Muted2, 1.2f);
    DrawLine(x + 12.5f, y + 9.0f, x + 18.0f, y + 18.0f, Theme::Muted2, 1.2f);
    DrawLine(x + 18.0f, y + 18.0f, x + 7.0f, y + 18.0f, Theme::Muted2, 1.2f);
}

void App::Card(float x, float y, float w, float h, const std::wstring& title)
{
    // Fully transparent section: no card fill and no card border.
    // Only the section title and controls remain on the window background.
    DrawTextLine(title, x + 4.0f, y + 8.0f, x + w - 4.0f, y + 34.0f, Theme::Muted, titleFont_);
}

void App::Divider(float x, float y, float w)
{
    DrawLine(x, y, x + w, y, Theme::Divider, 1.0f);
}


void App::Toggle(const std::wstring& label, bool& value, float x, float y, float w)
{
    const float rowH = 28.0f;
    const bool hovered = Hover(x, y, x + w, y + rowH);

    DrawTextLine(label, x, y, x + w - 48.0f, y + rowH,
        hovered ? Theme::Text : MakeColor(0xD6D6D6), font_);

    const float trackW = 31.0f;
    const float trackH = 14.0f;
    const float trackX = x + w - trackW;
    const float trackY = y + (rowH - trackH) * 0.5f;

    const D2D1_COLOR_F trackColor = value
        ? WithAlpha(accentColor_, hovered ? 0.58f : 0.44f)
        : (hovered ? MakeColor(0x232832) : MakeColor(0x191D24));

    FillRound(trackX, trackY, trackX + trackW, trackY + trackH, 7.0f, trackColor);
    DrawRound(trackX, trackY, trackX + trackW, trackY + trackH, 7.0f,
        value ? WithAlpha(AccentBright(), 0.42f) : MakeColor(0x252A33), 1.0f);

    const float knobSize = 12.0f;
    const float knobX = value ? trackX + trackW - knobSize - 1.0f : trackX + 1.0f;
    const float knobY = trackY + 1.0f;

    FillRound(knobX, knobY, knobX + knobSize, knobY + knobSize, 6.0f,
        value ? AccentBright() : MakeColor(0x343A45));

    if (value)
        DrawRound(knobX, knobY, knobX + knobSize, knobY + knobSize, 6.0f,
            WithAlpha(MakeColor(0xFFFFFF), 0.15f), 1.0f);

    if (!colorPickerOpen_ && !dropdownOpen_ && hovered && mousePressed_)
        value = !value;
}

void App::Slider(const std::wstring& label, float& value, float minimum, float maximum,
    bool isFloat, float x, float y, float w, int sliderId)
{
    DrawTextLine(label, x, y, x + 126.0f, y + 24.0f, Theme::Text, font_);

    wchar_t buffer[64]{};
    if (isFloat)
        swprintf_s(buffer, L"%.1f", value);
    else
        swprintf_s(buffer, L"%.0f", value);

    DrawTextLine(buffer, x + 126.0f, y, x + 170.0f, y + 24.0f,
        Theme::Text, font_, DWRITE_TEXT_ALIGNMENT_TRAILING);

    const float trackX = x + 181.0f;
    const float trackY = y + 14.0f;
    const float trackW = std::max(30.0f, w - 191.0f);
    const float ratio = Clamp((value - minimum) / (maximum - minimum), 0.0f, 1.0f);
    const float knobX = trackX + ratio * trackW;

    DrawLine(trackX, trackY, trackX + trackW, trackY, Theme::Divider, 3.0f);
    DrawLine(trackX, trackY, knobX, trackY, accentColor_, 3.0f);
    FillRound(knobX - 5.0f, trackY - 5.0f, knobX + 5.0f, trackY + 5.0f,
        5.0f, activeSlider_ == sliderId ? AccentBright() : MakeColor(0xE7E7E7));

    const bool hovered = Hover(trackX - 5.0f, trackY - 9.0f,
        trackX + trackW + 5.0f, trackY + 9.0f);

    if (!colorPickerOpen_ && !dropdownOpen_ && hovered && mousePressed_)
        activeSlider_ = sliderId;

    if (!colorPickerOpen_ && !dropdownOpen_ && mouseDown_ && activeSlider_ == sliderId)
    {
        const float t = Clamp((static_cast<float>(mouse_.x) - trackX) / trackW, 0.0f, 1.0f);
        value = minimum + t * (maximum - minimum);
        if (!isFloat)
            value = std::round(value);
    }
}

void App::Dropdown(const std::wstring& label, const std::vector<std::wstring>& items, int& selected, float x, float y, float w)
{
    if (items.empty())
        return;

    selected = std::clamp(selected, 0, static_cast<int>(items.size()) - 1);

    DrawTextLine(label, x, y, x + 120.0f, y + 24.0f, Theme::Text, font_);

    const float boxX = x + 120.0f;
    const float boxW = w - 120.0f;
    const float boxTop = y;
    const float boxBottom = y + 30.0f;
    const bool hovered = Hover(boxX, boxTop, boxX + boxW, boxBottom);

    FillRound(
        boxX,
        boxTop,
        boxX + boxW,
        boxBottom,
        8.0f,
        hovered || dropdownOpen_ ? Theme::InputHover : Theme::Input);

    DrawRound(
        boxX,
        boxTop,
        boxX + boxW,
        boxBottom,
        8.0f,
        dropdownOpen_ ? WithAlpha(accentColor_, 0.75f) : Theme::CardBorder,
        1.0f);

    DrawTextLine(
        items[selected],
        boxX + 12.0f,
        boxTop,
        boxX + boxW - 28.0f,
        boxBottom,
        Theme::Text,
        font_);

    DrawIcon(
        Icon::Chevron,
        boxX + boxW - 26.0f,
        boxTop,
        boxX + boxW - 6.0f,
        boxBottom,
        dropdownOpen_ ? AccentBright() : Theme::Muted);

    if (!colorPickerOpen_ && hovered && mousePressed_)
    {
        dropdownOpen_ = !dropdownOpen_;
        keybindCapture_ = false;
        configNameFocused_ = false;
    }

    // Store the popup for a dedicated overlay pass. The list is intentionally
    // NOT drawn here because later cards would paint over it.
    if (dropdownOpen_)
    {
        dropdownOverlay_.valid = true;
        dropdownOverlay_.buttonLeft = boxX;
        dropdownOverlay_.buttonTop = boxTop;
        dropdownOverlay_.buttonRight = boxX + boxW;
        dropdownOverlay_.buttonBottom = boxBottom;
        dropdownOverlay_.left = boxX;
        dropdownOverlay_.top = boxBottom + 4.0f;
        dropdownOverlay_.right = boxX + boxW;
        dropdownOverlay_.itemHeight = 29.0f;
        dropdownOverlay_.items = items;
        dropdownOverlay_.selected = &selected;
    }
}

void App::DrawDropdownOverlay()
{
    if (!dropdownOpen_ || !dropdownOverlay_.valid ||
        dropdownOverlay_.selected == nullptr || dropdownOverlay_.items.empty())
    {
        return;
    }

    const float left = dropdownOverlay_.left;
    const float top = dropdownOverlay_.top;
    const float right = dropdownOverlay_.right;
    const float itemHeight = dropdownOverlay_.itemHeight;
    const float bottom = top + itemHeight * static_cast<float>(dropdownOverlay_.items.size());

    // Opaque popup plus a small shadow makes it visually top-most over every
    // card and prevents text from the UI below showing through.
    FillRound(left + 4.0f, top + 5.0f, right + 4.0f, bottom + 5.0f,
        8.0f, MakeColor(0x000000, 0.42f));
    FillRound(left, top, right, bottom, 8.0f, MakeColor(0x090B0E));
    DrawRound(left, top, right, bottom, 8.0f,
        WithAlpha(accentColor_, 0.72f), 1.0f);

    for (std::size_t index = 0; index < dropdownOverlay_.items.size(); ++index)
    {
        const float itemTop = top + itemHeight * static_cast<float>(index);
        const float itemBottom = itemTop + itemHeight;
        const bool hovered = Hover(left, itemTop, right, itemBottom);
        const bool selected = static_cast<int>(index) == *dropdownOverlay_.selected;

        if (hovered)
        {
            FillRound(
                left + 3.0f,
                itemTop + 2.0f,
                right - 3.0f,
                itemBottom - 2.0f,
                5.0f,
                Theme::InputHover);
        }

        if (selected)
        {
            FillRound(
                left + 5.0f,
                itemTop + 7.0f,
                left + 8.0f,
                itemBottom - 7.0f,
                1.5f,
                accentColor_);
        }

        DrawTextLine(
            dropdownOverlay_.items[index],
            left + 13.0f,
            itemTop,
            right - 12.0f,
            itemBottom,
            selected ? AccentBright() : hovered ? Theme::Text : Theme::Muted,
            font_);

        if (!colorPickerOpen_ && hovered && mousePressed_)
        {
            *dropdownOverlay_.selected = static_cast<int>(index);
            dropdownOpen_ = false;
            dropdownOverlay_ = DropdownOverlay{};
            mousePressed_ = false;
            return;
        }
    }
}

bool App::IsModifierKey(int virtualKey) const
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

int App::EncodeHotkey(int virtualKey) const
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

std::wstring App::BaseKeyName(int virtualKey) const
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
    case VK_BACK: return L"Backspace";
    default:
        break;
    }

    UINT scanCode = MapVirtualKeyW(
        static_cast<UINT>(virtualKey),
        MAPVK_VK_TO_VSC);

    if (virtualKey == VK_LEFT ||
        virtualKey == VK_UP ||
        virtualKey == VK_RIGHT ||
        virtualKey == VK_DOWN ||
        virtualKey == VK_INSERT ||
        virtualKey == VK_DELETE ||
        virtualKey == VK_HOME ||
        virtualKey == VK_END ||
        virtualKey == VK_PRIOR ||
        virtualKey == VK_NEXT)
    {
        scanCode |= 0xE000;
    }

    wchar_t name[64]{};
    if (GetKeyNameTextW(
        static_cast<LONG>(scanCode << 16),
        name,
        static_cast<int>(sizeof(name) / sizeof(name[0]))) > 0)
    {
        return name;
    }

    return L"Key " + std::to_wstring(virtualKey);
}

std::wstring App::KeybindName(int encodedKey) const
{
    const int virtualKey = encodedKey & HotkeyKeyMask;
    if (virtualKey == 0)
        return L"None";

    std::wstring result;

    if ((encodedKey & HotkeyCtrlFlag) != 0)
        result += L"Ctrl+";

    if ((encodedKey & HotkeyAltFlag) != 0)
        result += L"Alt+";

    if ((encodedKey & HotkeyShiftFlag) != 0)
        result += L"Shift+";

    result += BaseKeyName(virtualKey);
    return result;
}

bool App::MatchesHotkey(int encodedKey, int virtualKey) const
{
    if ((encodedKey & HotkeyKeyMask) == 0 ||
        (encodedKey & HotkeyKeyMask) != (virtualKey & HotkeyKeyMask))
    {
        return false;
    }

    const bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool altDown = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    const bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    const bool needsCtrl = (encodedKey & HotkeyCtrlFlag) != 0;
    const bool needsAlt = (encodedKey & HotkeyAltFlag) != 0;
    const bool needsShift = (encodedKey & HotkeyShiftFlag) != 0;

    return ctrlDown == needsCtrl &&
        altDown == needsAlt &&
        shiftDown == needsShift;
}

void App::UpdateRegisteredHotkey()
{
    UnregisterHotKey(hwnd_, 1);
    hotkeyRegistered_ = false;

    const int virtualKey = aimbotKeybind_ & HotkeyKeyMask;
    if (virtualKey == 0)
        return;

    UINT modifiers = MOD_NOREPEAT;

    if ((aimbotKeybind_ & HotkeyCtrlFlag) != 0)
        modifiers |= MOD_CONTROL;

    if ((aimbotKeybind_ & HotkeyAltFlag) != 0)
        modifiers |= MOD_ALT;

    if ((aimbotKeybind_ & HotkeyShiftFlag) != 0)
        modifiers |= MOD_SHIFT;

    hotkeyRegistered_ = RegisterHotKey(
        hwnd_,
        1,
        modifiers,
        static_cast<UINT>(virtualKey)) != FALSE;
}

void App::KeybindControl(
    const std::wstring& label,
    int& encodedKey,
    float x,
    float y,
    float w)
{
    const float rowHeight = 31.0f;
    const float fieldWidth = 118.0f;
    const float fieldLeft = x + w - fieldWidth;
    const float fieldRight = x + w;

    const bool rowHover = Hover(x, y, x + w, y + rowHeight);
    const bool fieldHover = Hover(fieldLeft, y + 2.0f, fieldRight, y + rowHeight - 2.0f);

    if (rowHover)
    {
        FillRound(
            x - 5.0f,
            y + 1.0f,
            x + w + 1.0f,
            y + rowHeight - 1.0f,
            5.0f,
            WithAlpha(Theme::RowHover, 0.28f));
    }

    DrawTextLine(
        label,
        x,
        y,
        fieldLeft - 10.0f,
        y + rowHeight,
        rowHover ? Theme::Text : MakeColor(0xD5D5D5),
        font_);

    FillRound(
        fieldLeft,
        y + 4.0f,
        fieldRight,
        y + rowHeight - 4.0f,
        6.0f,
        keybindCapture_
        ? WithAlpha(accentColor_, 0.22f)
        : fieldHover
        ? Theme::InputHover
        : Theme::Input);

    DrawRound(
        fieldLeft,
        y + 4.0f,
        fieldRight,
        y + rowHeight - 4.0f,
        6.0f,
        keybindCapture_ ? accentColor_ : Theme::CardBorder,
        1.0f);

    const std::wstring valueText = keybindCapture_
        ? L"Press key..."
        : KeybindName(encodedKey);

    DrawTextLine(
        valueText,
        fieldLeft + 7.0f,
        y + 4.0f,
        fieldRight - 7.0f,
        y + rowHeight - 4.0f,
        keybindCapture_ ? AccentBright() : Theme::Text,
        smallFont_,
        DWRITE_TEXT_ALIGNMENT_CENTER);

    if (!colorPickerOpen_ && !dropdownOpen_ && fieldHover && mousePressed_)
    {
        keybindCapture_ = true;
        dropdownOpen_ = false;
        SetForegroundWindow(hwnd_);
        SetFocus(hwnd_);
    }
}


bool App::Button(
    const std::wstring& label,
    const wchar_t* icon,
    float x,
    float y,
    float w,
    float h,
    bool accent,
    bool danger)
{
    const bool hovered = Hover(x, y, x + w, y + h);

    D2D1_COLOR_F fill = hovered ? Theme::InputHover : Theme::Input;
    D2D1_COLOR_F border = Theme::CardBorder;
    D2D1_COLOR_F textColor = hovered ? Theme::Text : MakeColor(0xD5D5D5);

    if (accent)
    {
        fill = hovered ? AccentBright() : accentColor_;
        border = AccentBright();
        textColor = MakeColor(0xFFFFFF);
    }
    else if (danger)
    {
        fill = hovered ? MakeColor(0x54151C) : MakeColor(0x261014);
        border = MakeColor(0x7D1B25);
        textColor = hovered ? MakeColor(0xFFFFFF) : MakeColor(0xDF8A92);
    }

    FillRound(x, y, x + w, y + h, 7.0f, fill);
    DrawRound(x, y, x + w, y + h, 7.0f, border, 1.0f);

    if (icon)
        DrawIcon(icon, x + 8.0f, y, x + 32.0f, y + h, textColor);

    DrawTextLine(
        label,
        x + (icon ? 35.0f : 8.0f),
        y,
        x + w - 8.0f,
        y + h,
        textColor,
        smallFont_,
        icon ? DWRITE_TEXT_ALIGNMENT_LEADING : DWRITE_TEXT_ALIGNMENT_CENTER);

    return !colorPickerOpen_ && !dropdownOpen_ && hovered && mousePressed_;
}

void App::TextInput(
    const std::wstring& label,
    std::wstring& value,
    bool& focused,
    float x,
    float y,
    float w)
{
    DrawTextLine(label, x, y, x + 110.0f, y + 31.0f, Theme::Text, font_);

    const float inputX = x + 112.0f;
    const float inputW = w - 112.0f;
    const bool hovered = Hover(inputX, y, inputX + inputW, y + 31.0f);

    if (!colorPickerOpen_ && !dropdownOpen_ && mousePressed_)
        focused = hovered;

    FillRound(
        inputX,
        y,
        inputX + inputW,
        y + 31.0f,
        7.0f,
        focused || hovered ? Theme::InputHover : Theme::Input);

    DrawRound(
        inputX,
        y,
        inputX + inputW,
        y + 31.0f,
        7.0f,
        focused ? accentColor_ : Theme::CardBorder,
        1.0f);

    DrawTextLine(
        value.empty() ? L"Enter config name" : value,
        inputX + 10.0f,
        y,
        inputX + inputW - 10.0f,
        y + 31.0f,
        value.empty() ? Theme::Muted2 : Theme::Text,
        smallFont_);

    if (focused && ((GetTickCount64() / 500ULL) % 2ULL) == 0ULL)
    {
        const float caretX = std::min(
            inputX + 11.0f + static_cast<float>(value.size()) * 6.2f,
            inputX + inputW - 12.0f);
        DrawLine(caretX, y + 8.0f, caretX, y + 23.0f, AccentBright(), 1.0f);
    }
}

std::filesystem::path App::ConfigDirectory() const
{
    wchar_t appData[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH);

    std::filesystem::path directory = length > 0
        ? std::filesystem::path(appData) / L"EvictedMenu" / L"Configs"
        : std::filesystem::current_path() / L"Configs";

    std::error_code error;
    std::filesystem::create_directories(directory, error);
    return directory;
}

std::wstring App::CleanConfigName(const std::wstring& name) const
{
    std::wstring clean;
    clean.reserve(name.size());

    for (wchar_t character : name)
    {
        if (std::iswalnum(character) ||
            character == L' ' ||
            character == L'-' ||
            character == L'_')
        {
            clean.push_back(character);
        }
    }

    while (!clean.empty() && clean.front() == L' ')
        clean.erase(clean.begin());
    while (!clean.empty() && clean.back() == L' ')
        clean.pop_back();

    if (clean.size() > 36)
        clean.resize(36);

    return clean;
}

std::filesystem::path App::ConfigPath(const std::wstring& name) const
{
    return ConfigDirectory() / (CleanConfigName(name) + L".cfg");
}

bool App::SaveConfig(const std::wstring& name)
{
    const std::wstring cleanName = CleanConfigName(name);
    if (cleanName.empty())
        return false;

    std::wofstream file(ConfigPath(cleanName), std::ios::trunc);
    if (!file)
        return false;

    auto writeBool = [&](const wchar_t* key, bool value)
        {
            file << key << L"=" << (value ? 1 : 0) << L"\n";
        };
    auto writeFloat = [&](const wchar_t* key, float value)
        {
            file << key << L"=" << value << L"\n";
        };
    auto writeInt = [&](const wchar_t* key, int value)
        {
            file << key << L"=" << value << L"\n";
        };
    auto writeColor = [&](const wchar_t* prefix, const D2D1_COLOR_F& color)
        {
            file << prefix << L"R=" << color.r << L"\n";
            file << prefix << L"G=" << color.g << L"\n";
            file << prefix << L"B=" << color.b << L"\n";
        };

    file << L"[EvictedMenuV2]\n";
    writeBool(L"aimEnabled", aimEnabled_);
    writeBool(L"silentAim", silentAim_);
    writeBool(L"teamCheck", teamCheck_);
    writeBool(L"visibleOnly", visibleOnly_);
    writeBool(L"prediction", prediction_);
    writeFloat(L"aimFov", aimFov_);
    writeFloat(L"aimSmooth", aimSmooth_);
    writeFloat(L"aimDistance", aimDistance_);
    writeInt(L"targetBone", targetBone_);
    writeInt(L"aimbotKeybind", aimbotKeybind_);

    writeBool(L"espBox", espBox_);
    writeBool(L"espCornerBox", espCornerBox_);
    writeBool(L"espHealthBar", espHealthBar_);
    writeBool(L"espName", espName_);
    writeBool(L"espDistance", espDistance_);
    writeBool(L"espSnapline", espSnapline_);
    writeBool(L"espSkeleton", espSkeleton_);
    writeBool(L"espWeapon", espWeapon_);

    writeBool(L"particlesEnabled", particlesEnabled_);
    writeFloat(L"particleSpeed", particleSpeed_);
    writeFloat(L"particleAmount", particleAmount_);

    writeColor(L"background", backgroundColor_);
    writeColor(L"accent", accentColor_);
    writeColor(L"box", boxColor_);
    writeColor(L"health", healthColor_);
    writeColor(L"line", lineColor_);
    writeColor(L"skeleton", skeletonColor_);

    file.flush();
    return file.good();
}

bool App::LoadConfig(const std::wstring& name)
{
    std::wifstream file(ConfigPath(name));
    if (!file)
        return false;

    std::wstring line;
    while (std::getline(file, line))
    {
        if (line.empty() || line.front() == L'[')
            continue;

        const std::size_t separator = line.find(L'=');
        if (separator == std::wstring::npos)
            continue;

        const std::wstring key = line.substr(0, separator);
        const std::wstring value = line.substr(separator + 1);

        try
        {
            const bool booleanValue = std::stoi(value) != 0;
            const float floatValue = std::stof(value);

            if (key == L"aimEnabled") aimEnabled_ = booleanValue;
            else if (key == L"silentAim") silentAim_ = booleanValue;
            else if (key == L"teamCheck") teamCheck_ = booleanValue;
            else if (key == L"visibleOnly") visibleOnly_ = booleanValue;
            else if (key == L"prediction") prediction_ = booleanValue;
            else if (key == L"aimFov") aimFov_ = floatValue;
            else if (key == L"aimSmooth") aimSmooth_ = floatValue;
            else if (key == L"aimDistance") aimDistance_ = floatValue;
            else if (key == L"targetBone") targetBone_ = std::stoi(value);
            else if (key == L"aimbotKeybind") aimbotKeybind_ = std::stoi(value);

            else if (key == L"espBox") espBox_ = booleanValue;
            else if (key == L"espCornerBox") espCornerBox_ = booleanValue;
            else if (key == L"espHealthBar") espHealthBar_ = booleanValue;
            else if (key == L"espName") espName_ = booleanValue;
            else if (key == L"espDistance") espDistance_ = booleanValue;
            else if (key == L"espSnapline") espSnapline_ = booleanValue;
            else if (key == L"espSkeleton") espSkeleton_ = booleanValue;
            else if (key == L"espWeapon") espWeapon_ = booleanValue;

            else if (key == L"particlesEnabled") particlesEnabled_ = booleanValue;
            else if (key == L"particleSpeed") particleSpeed_ = floatValue;
            else if (key == L"particleAmount") particleAmount_ = floatValue;

            else if (key == L"backgroundR") backgroundColor_.r = floatValue;
            else if (key == L"backgroundG") backgroundColor_.g = floatValue;
            else if (key == L"backgroundB") backgroundColor_.b = floatValue;
            else if (key == L"accentR") accentColor_.r = floatValue;
            else if (key == L"accentG") accentColor_.g = floatValue;
            else if (key == L"accentB") accentColor_.b = floatValue;
            else if (key == L"boxR") boxColor_.r = floatValue;
            else if (key == L"boxG") boxColor_.g = floatValue;
            else if (key == L"boxB") boxColor_.b = floatValue;
            else if (key == L"healthR") healthColor_.r = floatValue;
            else if (key == L"healthG") healthColor_.g = floatValue;
            else if (key == L"healthB") healthColor_.b = floatValue;
            else if (key == L"lineR") lineColor_.r = floatValue;
            else if (key == L"lineG") lineColor_.g = floatValue;
            else if (key == L"lineB") lineColor_.b = floatValue;
            else if (key == L"skeletonR") skeletonColor_.r = floatValue;
            else if (key == L"skeletonG") skeletonColor_.g = floatValue;
            else if (key == L"skeletonB") skeletonColor_.b = floatValue;
        }
        catch (...)
        {
        }
    }

    aimFov_ = Clamp(aimFov_, 1.0f, 180.0f);
    aimSmooth_ = Clamp(aimSmooth_, 0.0f, 30.0f);
    aimDistance_ = Clamp(aimDistance_, 10.0f, 1000.0f);
    targetBone_ = std::clamp(targetBone_, 0, 4);
    particleSpeed_ = Clamp(particleSpeed_, 4.0f, 60.0f);
    particleAmount_ = Clamp(particleAmount_, 8.0f, 70.0f);

    auto clampColor = [](D2D1_COLOR_F& color)
        {
            color.r = Clamp(color.r, 0.0f, 1.0f);
            color.g = Clamp(color.g, 0.0f, 1.0f);
            color.b = Clamp(color.b, 0.0f, 1.0f);
            color.a = 1.0f;
        };

    clampColor(backgroundColor_);
    clampColor(accentColor_);
    clampColor(boxColor_);
    clampColor(healthColor_);
    clampColor(lineColor_);
    clampColor(skeletonColor_);

    UpdateRegisteredHotkey();
    return true;
}

bool App::DeleteConfig(const std::wstring& name)
{
    std::error_code error;
    return std::filesystem::remove(ConfigPath(name), error);
}

void App::RefreshConfigs()
{
    const std::wstring previous =
        selectedConfig_ >= 0 && selectedConfig_ < static_cast<int>(configs_.size())
        ? configs_[selectedConfig_]
        : L"";

    configs_.clear();
    std::error_code error;

    for (const auto& entry : std::filesystem::directory_iterator(ConfigDirectory(), error))
    {
        if (error)
            break;

        if (entry.is_regular_file() && entry.path().extension() == L".cfg")
            configs_.push_back(entry.path().stem().wstring());
    }

    std::sort(configs_.begin(), configs_.end());
    selectedConfig_ = -1;

    if (!previous.empty())
    {
        const auto iterator = std::find(configs_.begin(), configs_.end(), previous);
        if (iterator != configs_.end())
            selectedConfig_ = static_cast<int>(std::distance(configs_.begin(), iterator));
    }

    if (selectedConfig_ < 0 && !configs_.empty())
        selectedConfig_ = 0;

    const float maxScroll = std::max(0.0f, static_cast<float>(configs_.size()) - 8.0f);
    configScroll_ = Clamp(configScroll_, 0.0f, maxScroll);
}

void App::ShowConfigStatus(const std::wstring& text, bool ok)
{
    configStatus_ = text;
    configStatusOk_ = ok;
    configStatusTime_ = 3.0f;
}

void App::ColorRow(const std::wstring& label, D2D1_COLOR_F& color, float x, float y, float w, bool second)
{
    const float rowH = 29.0f;
    const float iconLeft = x + w - 31.0f;
    const float iconRight = x + w - 2.0f;
    const bool rowHover = Hover(x, y, x + w, y + rowH);
    const bool iconHover = Hover(iconLeft, y, iconRight, y + rowH);

    if (rowHover)
        FillRound(x - 5.0f, y + 1.0f, x + w + 1.0f, y + rowH - 1.0f, 5.0f,
            WithAlpha(Theme::RowHover, 0.32f));

    DrawTextLine(label, x, y, x + w - 36.0f, y + rowH,
        rowHover ? Theme::Text : MakeColor(0xD5D5D5), font_);

    if (iconHover)
        FillRound(iconLeft + 2.0f, y + 3.0f, iconRight - 1.0f, y + rowH - 3.0f,
            6.0f, WithAlpha(color, 0.12f));

    DrawIcon(Icon::Drop, iconLeft, y + (second ? -1.0f : 0.0f), iconRight, y + rowH,
        color);

    if (!colorPickerOpen_ && !dropdownOpen_ && iconHover && mousePressed_)
        OpenColorPicker(color);
}

void App::OpenColorPicker(D2D1_COLOR_F& color)
{
    pickerTarget_ = &color;
    pickerOriginal_ = color;
    pickerWorking_ = color;
    ColorToHsv(color, pickerHue_, pickerSaturation_, pickerValue_);
    colorPickerOpen_ = true;
    activeColorArea_ = -1;
    activeSlider_ = -1;
    dropdownOpen_ = false;
}

void App::UpdatePickerColorFromHsv()
{
    pickerWorking_ = HsvToColor(pickerHue_, pickerSaturation_, pickerValue_);
}

void App::DrawColorPicker()
{
    if (!colorPickerOpen_ || !pickerTarget_)
        return;

    FillRect(0.0f, 0.0f, static_cast<float>(UI::Width),
        static_cast<float>(UI::Height), MakeColor(0x000000, 0.76f));

    const float boxWidth = 364.0f;
    const float boxHeight = 300.0f;
    const float x = (static_cast<float>(UI::Width) - boxWidth) * 0.5f;
    const float y = (static_cast<float>(UI::Height) - boxHeight) * 0.5f;

    FillRound(x, y, x + boxWidth, y + boxHeight, 12.0f, MakeColor(0x090A0D));
    DrawRound(x, y, x + boxWidth, y + boxHeight, 12.0f, Theme::CardBorder, 1.0f);
    DrawLine(x + 15.0f, y + 42.0f, x + boxWidth - 15.0f, y + 42.0f, accentColor_, 1.2f);

    DrawTextLine(L"Custom Color Picker", x + 16.0f, y + 5.0f,
        x + boxWidth - 16.0f, y + 38.0f, Theme::Text, titleFont_);

    const float paletteX = x + 18.0f;
    const float paletteY = y + 57.0f;
    const float paletteWidth = 274.0f;
    const float paletteHeight = 174.0f;
    const float hueX = paletteX + paletteWidth + 11.0f;
    const float hueWidth = 20.0f;

    constexpr int columns = 64;
    constexpr int rows = 42;
    const float cellW = paletteWidth / static_cast<float>(columns);
    const float cellH = paletteHeight / static_cast<float>(rows);

    for (int row = 0; row < rows; ++row)
    {
        const float value = 1.0f - static_cast<float>(row) / static_cast<float>(rows - 1);
        for (int column = 0; column < columns; ++column)
        {
            const float saturation = static_cast<float>(column) / static_cast<float>(columns - 1);
            const float left = paletteX + cellW * static_cast<float>(column);
            const float top = paletteY + cellH * static_cast<float>(row);
            FillRect(left, top, left + cellW + 0.8f, top + cellH + 0.8f,
                HsvToColor(pickerHue_, saturation, value));
        }
    }

    constexpr int hueSteps = 96;
    const float hueStep = paletteHeight / static_cast<float>(hueSteps);
    for (int i = 0; i < hueSteps; ++i)
    {
        const float hue = static_cast<float>(i) / static_cast<float>(hueSteps - 1);
        const float top = paletteY + hueStep * static_cast<float>(i);
        FillRect(hueX, top, hueX + hueWidth, top + hueStep + 0.8f,
            HsvToColor(hue, 1.0f, 1.0f));
    }

    DrawRect(paletteX, paletteY, paletteX + paletteWidth, paletteY + paletteHeight,
        MakeColor(0x202329), 1.0f);
    DrawRect(hueX, paletteY, hueX + hueWidth, paletteY + paletteHeight,
        MakeColor(0x202329), 1.0f);

    const bool paletteHover = Hover(paletteX, paletteY,
        paletteX + paletteWidth, paletteY + paletteHeight);
    const bool hueHover = Hover(hueX, paletteY,
        hueX + hueWidth, paletteY + paletteHeight);

    if (mousePressed_ && paletteHover)
        activeColorArea_ = 0;
    else if (mousePressed_ && hueHover)
        activeColorArea_ = 1;

    if (mouseDown_ && activeColorArea_ == 0 && paletteHover)
    {
        pickerSaturation_ = Clamp(
            (static_cast<float>(mouse_.x) - paletteX) / paletteWidth, 0.0f, 1.0f);
        pickerValue_ = 1.0f - Clamp(
            (static_cast<float>(mouse_.y) - paletteY) / paletteHeight, 0.0f, 1.0f);
        UpdatePickerColorFromHsv();
    }
    else if (mouseDown_ && activeColorArea_ == 1 && hueHover)
    {
        pickerHue_ = Clamp(
            (static_cast<float>(mouse_.y) - paletteY) / paletteHeight, 0.0f, 1.0f);
        UpdatePickerColorFromHsv();
    }

    const float pointX = paletteX + pickerSaturation_ * paletteWidth;
    const float pointY = paletteY + (1.0f - pickerValue_) * paletteHeight;

    SetBrush(MakeColor(0x000000));
    target_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(pointX, pointY), 5.5f, 5.5f), brush_, 3.0f);
    SetBrush(MakeColor(0xFFFFFF));
    target_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(pointX, pointY), 5.5f, 5.5f), brush_, 1.4f);

    const float hueMarkerY = paletteY + pickerHue_ * paletteHeight;
    FillRect(hueX - 3.0f, hueMarkerY - 2.0f,
        hueX + hueWidth + 3.0f, hueMarkerY + 2.0f, MakeColor(0xFFFFFF));
    DrawRect(hueX - 4.0f, hueMarkerY - 3.0f,
        hueX + hueWidth + 4.0f, hueMarkerY + 3.0f, MakeColor(0x000000), 1.0f);

    FillRound(x + 18.0f, y + 244.0f, x + 49.0f, y + 266.0f, 4.0f, pickerWorking_);
    DrawRound(x + 18.0f, y + 244.0f, x + 49.0f, y + 266.0f, 4.0f,
        WithAlpha(MakeColor(0xFFFFFF), 0.18f), 1.0f);

    wchar_t hexText[16]{};
    const int red = std::clamp(static_cast<int>(std::round(pickerWorking_.r * 255.0f)), 0, 255);
    const int green = std::clamp(static_cast<int>(std::round(pickerWorking_.g * 255.0f)), 0, 255);
    const int blue = std::clamp(static_cast<int>(std::round(pickerWorking_.b * 255.0f)), 0, 255);
    swprintf_s(hexText, L"#%02X%02X%02X", red, green, blue);

    DrawTextLine(hexText, x + 58.0f, y + 239.0f,
        x + 165.0f, y + 270.0f, Theme::Text, smallFont_);

    const float buttonY = y + boxHeight - 37.0f;
    const float cancelX = x + boxWidth - 158.0f;
    const float applyX = x + boxWidth - 81.0f;

    const bool cancelHover = Hover(cancelX, buttonY, cancelX + 68.0f, buttonY + 25.0f);
    const bool applyHover = Hover(applyX, buttonY, applyX + 63.0f, buttonY + 25.0f);

    FillRound(cancelX, buttonY, cancelX + 68.0f, buttonY + 25.0f, 6.0f,
        cancelHover ? Theme::InputHover : Theme::Input);
    DrawRound(cancelX, buttonY, cancelX + 68.0f, buttonY + 25.0f, 6.0f,
        Theme::CardBorder, 1.0f);
    DrawTextLine(L"Cancel", cancelX, buttonY, cancelX + 68.0f, buttonY + 25.0f,
        Theme::Text, smallFont_, DWRITE_TEXT_ALIGNMENT_CENTER);

    FillRound(applyX, buttonY, applyX + 63.0f, buttonY + 25.0f, 6.0f,
        applyHover ? AccentBright() : accentColor_);
    DrawTextLine(L"Apply", applyX, buttonY, applyX + 63.0f, buttonY + 25.0f,
        MakeColor(0xFFFFFF), smallFont_, DWRITE_TEXT_ALIGNMENT_CENTER);

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

void App::DrawAimbotCard(float x, float y, float w, float h)
{
    Card(x, y, w, h, L"Aimbot");

    Toggle(L"Enabled", aimEnabled_, x + 12.0f, y + 47.0f, w - 24.0f);
    Toggle(L"Silent aim", silentAim_, x + 12.0f, y + 78.0f, w - 24.0f);
    Toggle(L"Team check", teamCheck_, x + 12.0f, y + 109.0f, w - 24.0f);
    Toggle(L"Visible only", visibleOnly_, x + 12.0f, y + 140.0f, w - 24.0f);
    Toggle(L"Prediction", prediction_, x + 12.0f, y + 171.0f, w - 24.0f);

    Divider(x + 12.0f, y + 207.0f, w - 24.0f);

    Dropdown(L"Target bone",
        { L"Head", L"Neck", L"Chest", L"Stomach", L"Closest" },
        targetBone_, x + 12.0f, y + 222.0f, w - 24.0f);

    Slider(L"Field of view", aimFov_, 1.0f, 180.0f, false,
        x + 12.0f, y + 274.0f, w - 24.0f, 1);
    Slider(L"Smoothing", aimSmooth_, 0.0f, 30.0f, true,
        x + 12.0f, y + 312.0f, w - 24.0f, 2);
    Slider(L"Max distance", aimDistance_, 10.0f, 1000.0f, false,
        x + 12.0f, y + 350.0f, w - 24.0f, 3);

    KeybindControl(L"Activation key", aimbotKeybind_,
        x + 12.0f, y + 393.0f, w - 24.0f);

    DrawTextLine(
        L"UI demonstration only — no game targeting or process integration.",
        x + 12.0f, y + h - 42.0f, x + w - 12.0f, y + h - 14.0f,
        Theme::Muted2, smallFont_);
}

void App::DrawVisualsCard(float x, float y, float w, float h)
{
    Card(x, y, w, h, L"Visuals");

    const float left = x + 12.0f;
    const float half = (w - 38.0f) * 0.5f;
    const float right = left + half + 14.0f;

    Toggle(L"Box", espBox_, left, y + 47.0f, half);
    Toggle(L"Corner box", espCornerBox_, left, y + 78.0f, half);
    Toggle(L"Health bar", espHealthBar_, left, y + 109.0f, half);
    Toggle(L"Name", espName_, left, y + 140.0f, half);

    Toggle(L"Distance", espDistance_, right, y + 47.0f, half);
    Toggle(L"Snap line", espSnapline_, right, y + 78.0f, half);
    Toggle(L"Skeleton", espSkeleton_, right, y + 109.0f, half);
    Toggle(L"Weapon", espWeapon_, right, y + 140.0f, half);

    Divider(x + 12.0f, y + 177.0f, w - 24.0f);
    ColorRow(L"Box color", boxColor_, x + 12.0f, y + 190.0f, w - 24.0f);
    ColorRow(L"Health color", healthColor_, x + 12.0f, y + 222.0f, w - 24.0f);
    ColorRow(L"Line color", lineColor_, x + 12.0f, y + 254.0f, w - 24.0f);
    ColorRow(L"Skeleton color", skeletonColor_, x + 12.0f, y + 286.0f, w - 24.0f);
}

void App::DrawParticles(float width, float height)
{
    if (!particlesEnabled_)
        return;

    const float time = static_cast<float>(GetTickCount64() % 100000ULL) / 1000.0f;
    const int count = std::clamp(static_cast<int>(std::round(particleAmount_)), 8, 70);
    const float contentLeft = 10.0f;
    const float contentTop = UI::TopBarHeight + 6.0f;
    const float contentWidth = std::max(1.0f, width - contentLeft - 8.0f);
    const float contentHeight = std::max(1.0f, height - contentTop - 8.0f);

    for (int index = 0; index < count; ++index)
    {
        const float seed = static_cast<float>(index + 1);
        const float baseX = std::fmod(seed * 91.73f + 27.0f, contentWidth);
        const float speed = particleSpeed_ * (0.42f + std::fmod(seed * 0.173f, 0.72f));
        const float travel = std::fmod(time * speed + seed * 31.0f, contentHeight + 24.0f);
        const float x = contentLeft + baseX + std::sin(time * 0.36f + seed * 1.7f) * 8.0f;
        const float y = contentTop + contentHeight + 12.0f - travel;
        const float radius = 0.75f + std::fmod(seed * 0.39f, 1.35f);
        const float alpha = 0.16f + std::fmod(seed * 0.061f, 0.24f);

        SetBrush(MakeColor(0xFFFFFF, alpha));
        target_->FillEllipse(
            D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius),
            brush_);
    }
}

void App::DrawTopBar(float width)
{
    FillRect(0.0f, 0.0f, width, UI::TopBarHeight, Theme::TopBar);

    // Compact logo only.
    DrawTriangleLogo(24.0f, 23.0f);

    // Larger 44x44 hit areas. Input is handled directly in WM_LBUTTONDOWN,
    // so a click cannot be lost to the draggable title region.
    constexpr float buttonSize = 44.0f;
    constexpr float buttonTop = 10.0f;
    constexpr float buttonGap = 8.0f;

    const float gearL = width - 12.0f - buttonSize;
    const float saveL = gearL - buttonGap - buttonSize;
    const float buttonBottom = buttonTop + buttonSize;

    const bool saveHover = Hover(saveL, buttonTop, saveL + buttonSize, buttonBottom);
    const bool gearHover = Hover(gearL, buttonTop, gearL + buttonSize, buttonBottom);

    if (saveHover || configOpen_)
    {
        FillRound(
            saveL,
            buttonTop,
            saveL + buttonSize,
            buttonBottom,
            9.0f,
            configOpen_ ? AccentSoft() : WithAlpha(Theme::RowHover, 0.55f));
    }

    if (gearHover || settingsOpen_)
    {
        FillRound(
            gearL,
            buttonTop,
            gearL + buttonSize,
            buttonBottom,
            9.0f,
            settingsOpen_ ? AccentSoft() : WithAlpha(Theme::RowHover, 0.55f));
    }

    DrawIcon(
        Icon::Save,
        saveL + 7.0f,
        buttonTop + 7.0f,
        saveL + buttonSize - 7.0f,
        buttonBottom - 7.0f,
        configOpen_ ? AccentBright() : saveHover ? Theme::Text : Theme::Muted);

    DrawIcon(
        Icon::Gear,
        gearL + 7.0f,
        buttonTop + 7.0f,
        gearL + buttonSize - 7.0f,
        buttonBottom - 7.0f,
        settingsOpen_ ? AccentBright() : gearHover ? Theme::Text : Theme::Muted);
}


void App::DrawCards()
{
    const float margin = 20.0f;
    const float gap = 18.0f;
    const float leftWidth = 365.0f;
    const float rightX = margin + leftWidth + gap;
    const float rightWidth = static_cast<float>(UI::Width) - rightX - margin;

    DrawAimbotCard(margin, 82.0f, leftWidth, 494.0f);

    // The old visual preview card was removed. The visuals controls now use
    // the complete right column, keeping both columns balanced and clean.
    DrawVisualsCard(rightX, 82.0f, rightWidth, 494.0f);
}

void App::DrawSettingsPage()
{
    const float x = 24.0f;
    const float y = 84.0f;
    const float w = static_cast<float>(UI::Width) - 48.0f;

    Card(x, y, w, 470.0f, L"Settings");

    DrawTextLine(L"Appearance", x + 8.0f, y + 44.0f,
        x + w - 8.0f, y + 69.0f, Theme::Muted, titleFont_);
    ColorRow(L"Accent color", accentColor_, x + 8.0f, y + 75.0f, w - 16.0f);
    ColorRow(L"Window tint", backgroundColor_, x + 8.0f, y + 108.0f, w - 16.0f);

    Divider(x + 8.0f, y + 149.0f, w - 16.0f);
    DrawTextLine(L"Background", x + 8.0f, y + 161.0f,
        x + w - 8.0f, y + 186.0f, Theme::Muted, titleFont_);

    Toggle(L"Background particles", particlesEnabled_,
        x + 8.0f, y + 194.0f, w - 16.0f);
    Slider(L"Particle speed", particleSpeed_, 4.0f, 60.0f, false,
        x + 8.0f, y + 234.0f, w - 16.0f, 10);
    Slider(L"Particle amount", particleAmount_, 8.0f, 70.0f, false,
        x + 8.0f, y + 276.0f, w - 16.0f, 11);

    Divider(x + 8.0f, y + 322.0f, w - 16.0f);
    DrawTextLine(
        L"Accent color controls sliders, toggles, selection states and config highlights. "
        L"Window tint changes only the background.",
        x + 8.0f, y + 340.0f, x + w - 8.0f, y + 393.0f,
        Theme::Muted2, smallFont_);
}

void App::DrawConfigPage()
{
    const float x = 24.0f;
    const float y = 84.0f;
    const float w = static_cast<float>(UI::Width) - 48.0f;
    const float h = 480.0f;

    Card(x, y, w, h, L"Configurations");

    const float leftW = 262.0f;
    const float gap = 18.0f;
    const float rightX = x + leftW + gap;
    const float rightW = w - leftW - gap;

    FillRound(x + 2.0f, y + 45.0f, x + leftW, y + h - 6.0f,
        10.0f, MakeColor(0x07090C, 0.62f));
    FillRound(rightX, y + 45.0f, x + w - 2.0f, y + h - 6.0f,
        10.0f, MakeColor(0x07090C, 0.62f));

    DrawTextLine(L"Create or update", x + 16.0f, y + 56.0f,
        x + leftW - 16.0f, y + 82.0f, Theme::Muted, titleFont_);

    DrawTextLine(L"Name", x + 16.0f, y + 91.0f,
        x + leftW - 16.0f, y + 113.0f, Theme::Muted2, smallFont_);

    const float inputX = x + 16.0f;
    const float inputY = y + 118.0f;
    const float inputW = leftW - 32.0f;
    const bool inputHover = Hover(inputX, inputY, inputX + inputW, inputY + 34.0f);
    if (!colorPickerOpen_ && !dropdownOpen_ && mousePressed_)
        configNameFocused_ = inputHover;

    FillRound(inputX, inputY, inputX + inputW, inputY + 34.0f,
        7.0f, configNameFocused_ || inputHover ? Theme::InputHover : Theme::Input);
    DrawRound(inputX, inputY, inputX + inputW, inputY + 34.0f,
        7.0f, configNameFocused_ ? accentColor_ : Theme::CardBorder, 1.0f);
    DrawTextLine(configName_.empty() ? L"Config name" : configName_,
        inputX + 10.0f, inputY, inputX + inputW - 10.0f, inputY + 34.0f,
        configName_.empty() ? Theme::Muted2 : Theme::Text, smallFont_);

    if (Button(L"Save config", Icon::Save, inputX, y + 171.0f,
        inputW, 36.0f, true))
    {
        const std::wstring clean = CleanConfigName(configName_);
        if (clean.empty())
            ShowConfigStatus(L"Enter a valid config name.", false);
        else if (SaveConfig(clean))
        {
            configName_ = clean;
            RefreshConfigs();
            const auto it = std::find(configs_.begin(), configs_.end(), clean);
            if (it != configs_.end())
                selectedConfig_ = static_cast<int>(std::distance(configs_.begin(), it));
            ShowConfigStatus(L"Config saved.", true);
        }
        else
            ShowConfigStatus(L"Could not save config.", false);
    }

    if (Button(L"Load selected", Icon::Load, inputX, y + 219.0f,
        inputW, 36.0f))
    {
        if (selectedConfig_ < 0 || selectedConfig_ >= static_cast<int>(configs_.size()))
            ShowConfigStatus(L"Select a config first.", false);
        else if (LoadConfig(configs_[selectedConfig_]))
        {
            configName_ = configs_[selectedConfig_];
            ShowConfigStatus(L"Config loaded.", true);
        }
        else
            ShowConfigStatus(L"Could not load config.", false);
    }

    if (Button(L"Delete selected", Icon::Delete, inputX, y + 267.0f,
        inputW, 36.0f, false, true))
    {
        if (selectedConfig_ < 0 || selectedConfig_ >= static_cast<int>(configs_.size()))
            ShowConfigStatus(L"Select a config first.", false);
        else if (DeleteConfig(configs_[selectedConfig_]))
        {
            configName_.clear();
            RefreshConfigs();
            ShowConfigStatus(L"Config deleted.", true);
        }
        else
            ShowConfigStatus(L"Could not delete config.", false);
    }

    DrawTextLine(L"Saved configs", rightX + 16.0f, y + 56.0f,
        x + w - 18.0f, y + 82.0f, Theme::Muted, titleFont_);

    const float listLeft = rightX + 12.0f;
    const float listTop = y + 91.0f;
    const float listRight = x + w - 14.0f;
    const float listBottom = y + h - 52.0f;
    const float rowH = 38.0f;
    constexpr int visibleRows = 8;

    if (configs_.empty())
    {
        DrawIcon(Icon::File, listLeft, listTop + 75.0f,
            listRight, listTop + 115.0f, Theme::Muted2);
        DrawTextLine(L"No saved configs", listLeft, listTop + 116.0f,
            listRight, listTop + 143.0f, Theme::Muted,
            titleFont_, DWRITE_TEXT_ALIGNMENT_CENTER);
    }
    else
    {
        const int maxStart = std::max(0, static_cast<int>(configs_.size()) - visibleRows);
        const int startIndex = std::clamp(static_cast<int>(std::round(configScroll_)), 0, maxStart);
        const int endIndex = std::min(static_cast<int>(configs_.size()), startIndex + visibleRows);

        float rowY = listTop;
        for (int index = startIndex; index < endIndex; ++index)
        {
            const bool selected = index == selectedConfig_;
            const bool hovered = Hover(listLeft, rowY, listRight - 10.0f, rowY + rowH - 4.0f);

            if (selected || hovered)
                FillRound(listLeft, rowY, listRight - 10.0f, rowY + rowH - 4.0f,
                    7.0f, selected ? AccentSoft(0.24f) : WithAlpha(Theme::RowHover, 0.45f));

            DrawIcon(Icon::Save, listLeft + 8.0f, rowY,
                listLeft + 34.0f, rowY + rowH - 4.0f,
                selected ? AccentBright() : hovered ? Theme::Text : Theme::Muted2);
            DrawTextLine(configs_[index], listLeft + 39.0f, rowY,
                listRight - 18.0f, rowY + rowH - 4.0f,
                selected ? Theme::Text : hovered ? Theme::Text : Theme::Muted, font_);

            if (!colorPickerOpen_ && !dropdownOpen_ && hovered && mousePressed_)
            {
                selectedConfig_ = index;
                configName_ = configs_[index];
                configNameFocused_ = false;
            }
            rowY += rowH;
        }

        if (static_cast<int>(configs_.size()) > visibleRows)
        {
            const float trackTop = listTop + 3.0f;
            const float trackBottom = listBottom - 3.0f;
            const float trackH = trackBottom - trackTop;
            const float thumbH = std::max(42.0f,
                trackH * static_cast<float>(visibleRows) / static_cast<float>(configs_.size()));
            const float maxScroll = static_cast<float>(maxStart);
            const float ratio = maxScroll <= 0.0f ? 0.0f : Clamp(configScroll_ / maxScroll, 0.0f, 1.0f);
            const float thumbTop = trackTop + ratio * (trackH - thumbH);
            const float sx = listRight - 4.0f;

            FillRound(sx, trackTop, sx + 3.0f, trackBottom, 1.5f, MakeColor(0x1A1D22));
            FillRound(sx, thumbTop, sx + 3.0f, thumbTop + thumbH, 1.5f, accentColor_);
        }
    }

    if (!configStatus_.empty())
    {
        const D2D1_COLOR_F c = configStatusOk_ ? MakeColor(0x43D17D) : MakeColor(0xE05261);
        FillRound(x + 14.0f, y + h - 41.0f, x + w - 14.0f, y + h - 10.0f,
            7.0f, WithAlpha(c, 0.10f));
        DrawTextLine(configStatus_, x + 24.0f, y + h - 41.0f,
            x + w - 24.0f, y + h - 10.0f, c, smallFont_);
    }
}

void App::DrawShell(float width, float height)
{
    dropdownOverlay_ = DropdownOverlay{};

    FillRect(0.0f, 0.0f, width, height,
        BlendColor(Theme::Window, backgroundColor_, 0.035f));

    DrawParticles(width, height);
    DrawTopBar(width);

    if (configOpen_)
        DrawConfigPage();
    else if (settingsOpen_)
        DrawSettingsPage();
    else
        DrawCards();

    // Dedicated overlay pass: dropdowns render after every page/card so they
    // can never be covered by controls drawn later in the frame.
    DrawDropdownOverlay();

    DrawRound(0.5f, 0.5f, width - 0.5f, height - 0.5f, 8.0f, Theme::SidebarBorder, 1.0f);
    DrawColorPicker();
}

void App::Render()
{
    if (FAILED(CreateDeviceResources()))
        return;

    RECT client{};
    GetClientRect(hwnd_, &client);
    const float width = static_cast<float>(client.right - client.left);
    const float height = static_cast<float>(client.bottom - client.top);

    if (!mouseDown_)
        activeSlider_ = -1;

    if (configStatusTime_ > 0.0f)
    {
        configStatusTime_ -= 0.016f;
        if (configStatusTime_ <= 0.0f)
        {
            configStatusTime_ = 0.0f;
            configStatus_.clear();
        }
    }

    target_->BeginDraw();
    target_->SetTransform(D2D1::Matrix3x2F::Identity());
    target_->Clear(Theme::Window);

    DrawShell(width, height);

    const HRESULT hr = target_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
        DiscardDeviceResources();

    mousePressed_ = false;
}

LRESULT CALLBACK App::StaticWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    App* app = nullptr;

    if (message == WM_NCCREATE)
    {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        app->hwnd_ = hwnd;
    }
    else
    {
        app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    return app ? app->WindowProc(message, wParam, lParam)
        : DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT App::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_TIMER:
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;

    case WM_NCHITTEST:
    {
        const LRESULT result = DefWindowProcW(hwnd_, message, wParam, lParam);
        if (result != HTCLIENT)
            return result;

        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hwnd_, &pt);

        RECT client{};
        GetClientRect(hwnd_, &client);

        constexpr float buttonSize = 44.0f;
        constexpr float buttonTop = 10.0f;
        constexpr float buttonGap = 8.0f;
        const float gearL = static_cast<float>(client.right) - 12.0f - buttonSize;
        const float saveL = gearL - buttonGap - buttonSize;
        const float buttonBottom = buttonTop + buttonSize;

        // Never treat the save/settings button area as a draggable caption.
        if (Hit(static_cast<float>(pt.x), static_cast<float>(pt.y),
            saveL, buttonTop, saveL + buttonSize, buttonBottom) ||
            Hit(static_cast<float>(pt.x), static_cast<float>(pt.y),
                gearL, buttonTop, gearL + buttonSize, buttonBottom))
        {
            return HTCLIENT;
        }

        if (pt.y >= 0 && pt.y < static_cast<LONG>(UI::DragHeight))
            return HTCAPTION;
        return HTCLIENT;
    }

    case WM_MOUSEMOVE:
        mouse_.x = GET_X_LPARAM(lParam);
        mouse_.y = GET_Y_LPARAM(lParam);
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;

    case WM_MOUSEWHEEL:
    {
        if (configOpen_ && configs_.size() > 8)
        {
            POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd_, &point);

            const float listLeft = 316.0f;
            const float listTop = 175.0f;
            const float listRight = static_cast<float>(UI::Width) - 38.0f;
            const float listBottom = 512.0f;

            if (Hit(static_cast<float>(point.x), static_cast<float>(point.y),
                listLeft, listTop, listRight, listBottom))
            {
                const float direction =
                    GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? -1.0f : 1.0f;
                const float maxScroll =
                    std::max(0.0f, static_cast<float>(configs_.size()) - 8.0f);
                configScroll_ = Clamp(configScroll_ + direction, 0.0f, maxScroll);
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }
        }
        break;
    }

    case WM_LBUTTONDOWN:
    {
        mouse_.x = GET_X_LPARAM(lParam);
        mouse_.y = GET_Y_LPARAM(lParam);

        RECT client{};
        GetClientRect(hwnd_, &client);

        constexpr float buttonSize = 44.0f;
        constexpr float buttonTop = 10.0f;
        constexpr float buttonGap = 8.0f;
        const float gearL = static_cast<float>(client.right) - 12.0f - buttonSize;
        const float saveL = gearL - buttonGap - buttonSize;
        const float buttonBottom = buttonTop + buttonSize;

        const bool saveHit = Hit(
            static_cast<float>(mouse_.x), static_cast<float>(mouse_.y),
            saveL, buttonTop, saveL + buttonSize, buttonBottom);
        const bool gearHit = Hit(
            static_cast<float>(mouse_.x), static_cast<float>(mouse_.y),
            gearL, buttonTop, gearL + buttonSize, buttonBottom);

        // Handle these actions directly instead of waiting for a render pass.
        // This makes every click register, including clicks in the old drag area.
        if (!colorPickerOpen_ && saveHit)
        {
            configOpen_ = !configOpen_;
            settingsOpen_ = false;
            dropdownOpen_ = false;
            dropdownOverlay_ = DropdownOverlay{};
            keybindCapture_ = false;
            configNameFocused_ = false;
            RefreshConfigs();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (!colorPickerOpen_ && gearHit)
        {
            settingsOpen_ = !settingsOpen_;
            configOpen_ = false;
            dropdownOpen_ = false;
            dropdownOverlay_ = DropdownOverlay{};
            keybindCapture_ = false;
            configNameFocused_ = false;
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (!colorPickerOpen_ && dropdownOpen_ && dropdownOverlay_.valid)
        {
            const float popupBottom = dropdownOverlay_.top +
                dropdownOverlay_.itemHeight * static_cast<float>(dropdownOverlay_.items.size());

            const bool insideButton = Hit(
                static_cast<float>(mouse_.x), static_cast<float>(mouse_.y),
                dropdownOverlay_.buttonLeft, dropdownOverlay_.buttonTop,
                dropdownOverlay_.buttonRight, dropdownOverlay_.buttonBottom);

            const bool insidePopup = Hit(
                static_cast<float>(mouse_.x), static_cast<float>(mouse_.y),
                dropdownOverlay_.left, dropdownOverlay_.top,
                dropdownOverlay_.right, popupBottom);

            if (!insideButton && !insidePopup)
            {
                dropdownOpen_ = false;
                dropdownOverlay_ = DropdownOverlay{};
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }
        }

        SetCapture(hwnd_);
        mouseDown_ = true;
        mousePressed_ = true;
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONUP:
        mouseDown_ = false;

        // End color-palette/hue dragging immediately. Without this reset,
        // the next click on Apply could still be treated as a palette drag.
        // Because Apply is below the palette, the value was clamped to 0,
        // which changed the selected color to black.
        activeColorArea_ = -1;

        ReleaseCapture();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;

    case WM_CHAR:
        if (configOpen_ && configNameFocused_)
        {
            const wchar_t character = static_cast<wchar_t>(wParam);

            if (character == L'\b')
            {
                if (!configName_.empty())
                    configName_.pop_back();
            }
            else if (character >= 32 && configName_.size() < 36 &&
                (std::iswalnum(character) || character == L' ' ||
                    character == L'-' || character == L'_'))
            {
                configName_.push_back(character);
            }

            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return 0;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        const int virtualKey = static_cast<int>(wParam);
        const bool isRepeat = (lParam & (1LL << 30)) != 0;

        if (configNameFocused_)
        {
            if (virtualKey == VK_ESCAPE)
            {
                configNameFocused_ = false;
            }
            else if (virtualKey == VK_RETURN)
            {
                configNameFocused_ = false;
                const std::wstring clean = CleanConfigName(configName_);
                if (clean.empty())
                {
                    ShowConfigStatus(L"Enter a valid config name.", false);
                }
                else if (SaveConfig(clean))
                {
                    configName_ = clean;
                    RefreshConfigs();
                    const auto iterator = std::find(configs_.begin(), configs_.end(), clean);
                    if (iterator != configs_.end())
                        selectedConfig_ = static_cast<int>(std::distance(configs_.begin(), iterator));
                    ShowConfigStatus(L"Configuration saved.", true);
                }
                else
                {
                    ShowConfigStatus(L"Could not save the configuration.", false);
                }
            }

            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (keybindCapture_)
        {
            if (virtualKey == VK_ESCAPE)
            {
                keybindCapture_ = false;
            }
            else if (virtualKey == VK_BACK)
            {
                aimbotKeybind_ = 0;
                keybindCapture_ = false;
                UpdateRegisteredHotkey();
            }
            else if (!IsModifierKey(virtualKey))
            {
                aimbotKeybind_ = EncodeHotkey(virtualKey);
                keybindCapture_ = false;
                UpdateRegisteredHotkey();
            }

            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (virtualKey == VK_ESCAPE)
        {
            if (colorPickerOpen_ && pickerTarget_)
            {
                *pickerTarget_ = pickerOriginal_;
                colorPickerOpen_ = false;
                pickerTarget_ = nullptr;
                activeColorArea_ = -1;
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
            else
            {
                PostMessageW(hwnd_, WM_CLOSE, 0, 0);
            }
            return 0;
        }

        // Registered hotkeys work even while another window is focused.
        // Ctrl, Alt and Shift combinations are supported, for example Ctrl+Alt+X.
        if (!hotkeyRegistered_ && !isRepeat && MatchesHotkey(aimbotKeybind_, virtualKey))
        {
            aimEnabled_ = !aimEnabled_;
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        return 0;
    }

    case WM_HOTKEY:
        if (wParam == 1)
        {
            aimEnabled_ = !aimEnabled_;
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
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

    case WM_SETCURSOR:
        SetCursor(LoadCursorW(nullptr, IDC_ARROW));
        return TRUE;

    case WM_DESTROY:
        KillTimer(hwnd_, 1);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd_, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    App app;
    if (!app.Initialize(instance))
    {
        MessageBoxW(nullptr, L"Failed to initialize Direct2D UI.", L"Error", MB_OK | MB_ICONERROR);
        app.Shutdown();
        return 1;
    }

    const int result = app.Run();
    app.Shutdown();
    return result;
}
