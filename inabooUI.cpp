// ============================================================================
// Transparent black Direct2D menu inspired by the supplied reference.
// Single-file C++17 / Win32 / Direct2D example.
//
// Build (Visual Studio, x64, Unicode, Windows subsystem):
//   d2d1.lib
//   dwrite.lib
//
// This is only a visual UI template. It contains no game/process integration.
// Drag the thin strip at the very top to move the window. Press Esc to close.
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
#include <array>
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
void SafeRelease(T*& value)
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

static D2D1_COLOR_F MakeColor(std::uint32_t rgbValue, float alpha = 1.0f)
{
    return D2D1::ColorF(
        static_cast<float>((rgbValue >> 16) & 0xFF) / 255.0f,
        static_cast<float>((rgbValue >> 8) & 0xFF) / 255.0f,
        static_cast<float>(rgbValue & 0xFF) / 255.0f,
        alpha);
}

static D2D1_COLOR_F WithAlpha(const D2D1_COLOR_F& color, float alpha)
{
    return D2D1::ColorF(color.r, color.g, color.b, alpha);
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

namespace Layout
{
    constexpr int Width = 760;
    constexpr int Height = 470;

    constexpr float Sidebar = 72.0f;
    constexpr float SplitX = 418.0f;
    constexpr float DragStrip = 14.0f;

    constexpr float LeftX = 101.0f;
    constexpr float LeftRight = 397.0f;
    constexpr float RightX = 438.0f;
    constexpr float RightRight = 736.0f;
}

namespace Theme
{
    const D2D1_COLOR_F Window = MakeColor(0x090A0D);
    const D2D1_COLOR_F Sidebar = MakeColor(0x0B0C10);
    const D2D1_COLOR_F Row = MakeColor(0x0E1014);
    const D2D1_COLOR_F RowHover = MakeColor(0x13161B);
    const D2D1_COLOR_F Popup = MakeColor(0x0B0C0F);
    const D2D1_COLOR_F PopupHover = MakeColor(0x15171B);

    const D2D1_COLOR_F Line = MakeColor(0x171A20);
    const D2D1_COLOR_F LineSoft = MakeColor(0x12151A);
    const D2D1_COLOR_F Track = MakeColor(0x20242B);

    const D2D1_COLOR_F Text = MakeColor(0xD4D7DC);
    const D2D1_COLOR_F SubText = MakeColor(0x777D87);
    const D2D1_COLOR_F Muted = MakeColor(0x484E58);
    const D2D1_COLOR_F White = MakeColor(0xF4F5F7);

    const D2D1_COLOR_F Green = MakeColor(0x52D628);
    const D2D1_COLOR_F Red = MakeColor(0xE21E2A);
    const D2D1_COLOR_F Light = MakeColor(0xD5D8DE);
    const D2D1_COLOR_F Amber = MakeColor(0x8D5700);
    const D2D1_COLOR_F Blue = MakeColor(0x91A5D4);
    const D2D1_COLOR_F Success = MakeColor(0x55D98A);
    const D2D1_COLOR_F Danger = MakeColor(0xE45A67);
    const D2D1_COLOR_F Panel = MakeColor(0x0C0E12);
}

namespace Glyph
{
    constexpr const wchar_t* Target = L"\xF272";
    constexpr const wchar_t* Eye = L"\xE890";
    constexpr const wchar_t* Settings = L"\xE713";
    constexpr const wchar_t* Folder = L"\xE8B7";
    constexpr const wchar_t* Cloud = L"\xE753";
    constexpr const wchar_t* Chevron = L"\xE70D";
    constexpr const wchar_t* Profile = L"\xE77B";
    constexpr const wchar_t* Save = L"\xE74E";
    constexpr const wchar_t* Load = L"\xE72C";
    constexpr const wchar_t* Delete = L"\xE74D";
    constexpr const wchar_t* Add = L"\xE710";
    constexpr const wchar_t* Check = L"\xE73E";
    constexpr const wchar_t* Info = L"\xE946";
    constexpr const wchar_t* Sparkle = L"\xE945";
}

struct Settings
{
    bool aimbot = false;
    bool triggerbot = false;
    bool enableScroller = true;
    bool disableRain = false;
    bool bunnyhop = false;
    bool fastDuck = false;
    bool longJump = false;
    bool connectedParticles = true;

    float aimbotFov = 54.6f;
    float particleSpeed = 22.0f;
    float dpiScale = 134.7f;

    int hitbox = 0;
    int multipoint = 0;

    D2D1_COLOR_F pickerColor = Theme::Amber;
    D2D1_COLOR_F accentColor = Theme::Blue;
};

class App
{
public:
    bool Initialize(HINSTANCE instance);
    int Run();
    void Shutdown();

private:
    enum class ActiveSlider
    {
        None,
        AimbotFov,
        DpiScale,
        ParticleSpeed
    };


    struct ComboPopup
    {
        int id = 0;
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float itemHeight = 0.0f;
        std::vector<std::wstring> items;
        int* selected = nullptr;
    };

    HWND hwnd_ = nullptr;

    ID2D1Factory* d2dFactory_ = nullptr;
    ID2D1HwndRenderTarget* renderTarget_ = nullptr;
    ID2D1SolidColorBrush* brush_ = nullptr;
    IDWriteFactory* writeFactory_ = nullptr;
    IDWriteTextFormat* text_ = nullptr;
    IDWriteTextFormat* small_ = nullptr;
    IDWriteTextFormat* tiny_ = nullptr;
    IDWriteTextFormat* icon_ = nullptr;

    POINT mouse_{ -1000, -1000 };
    bool mouseDown_ = false;
    bool clicked_ = false;
    bool clickConsumed_ = false;

    int selectedPage_ = 0;
    int openCombo_ = 0;
    ActiveSlider activeSlider_ = ActiveSlider::None;

    std::wstring configName_;
    bool configNameFocused_ = false;
    std::vector<std::wstring> configs_;
    int selectedConfig_ = -1;
    int configScrollIndex_ = 0;
    bool configScrollbarDragging_ = false;
    float configScrollbarGrabOffset_ = 0.0f;
    std::wstring statusText_;
    bool statusSuccess_ = true;
    float statusTimer_ = 0.0f;

    bool colorPickerOpen_ = false;
    D2D1_COLOR_F pickerOriginal_{};
    D2D1_COLOR_F pickerWorking_{};
    D2D1_COLOR_F* pickerTarget_ = nullptr;
    int activeColorArea_ = -1;
    float pickerHue_ = 0.0f;
    float pickerSaturation_ = 0.0f;
    float pickerValue_ = 1.0f;

    Settings settings_{};
    ComboPopup comboPopup_{};

    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

    HRESULT CreateFactories();
    HRESULT CreateDeviceResources();
    void DiscardDeviceResources();

    void Render();
    void DrawShell(float width, float height);
    void DrawSidebar(float height);
    void DrawMainPage();
    void DrawSettingsPage();
    void DrawProfilePage();
    void DrawOtherPage(int page);
    void DrawConnectedParticles(float width, float height);

    void SetBrush(const D2D1_COLOR_F& color);
    void FillRect(float left, float top, float right, float bottom, const D2D1_COLOR_F& color);
    void DrawRect(float left, float top, float right, float bottom, const D2D1_COLOR_F& color, float stroke = 1.0f);
    void FillRound(float left, float top, float right, float bottom, float radius, const D2D1_COLOR_F& color);
    void DrawRound(float left, float top, float right, float bottom, float radius, const D2D1_COLOR_F& color, float stroke = 1.0f);
    void DrawLine(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, float stroke = 1.0f);
    void DrawText(const std::wstring& value, float left, float top, float right, float bottom,
        const D2D1_COLOR_F& color, IDWriteTextFormat* format = nullptr,
        DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING);
    void DrawIcon(const wchar_t* glyph, float left, float top, float right, float bottom,
        const D2D1_COLOR_F& color);

    bool Hover(float left, float top, float right, float bottom) const;
    void Divider(float left, float right, float y);
    void SidebarButton(int index, const wchar_t* glyph, float y);
    void ToggleRow(const std::wstring& label, bool& value, float left, float right, float y, bool dividerBelow = false);
    void SliderControl(const std::wstring& label, float& value, float minimum, float maximum,
        float left, float right, float y, ActiveSlider slider, int decimals = 1);
    void ComboControl(int id, const std::wstring& label, const std::vector<std::wstring>& items,
        int& selected, float left, float right, float y);
    void ColorRow(const std::wstring& label, D2D1_COLOR_F& color,
        float left, float right, float y);
    bool ActionButton(const std::wstring& label, const wchar_t* glyph,
        float left, float top, float right, float bottom, bool accent = false, bool danger = false);
    void TextInput(const std::wstring& label, std::wstring& value, bool& focused,
        float left, float right, float y);
    void StatusMessage(const std::wstring& text, bool success);
    void DrawComboPopup();
    void OpenColorPicker(D2D1_COLOR_F& color);
    void UpdatePickerColorFromHsv();
    void DrawColorPicker();

    std::filesystem::path ConfigDirectory() const;
    std::filesystem::path ConfigPath(const std::wstring& name) const;
    std::wstring SanitizeConfigName(const std::wstring& name) const;
    bool SaveConfig(const std::wstring& name);
    bool LoadConfig(const std::wstring& name);
    bool DeleteConfig(const std::wstring& name);
    void RefreshConfigs();
    void EnsureSelectedConfigVisible();
    bool HasSelectedConfig() const;
    std::wstring SelectedConfig() const;

    static std::wstring FormatNumber(float value, int decimals);
};

HRESULT App::CreateFactories()
{
    HRESULT result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory_);
    if (FAILED(result))
        return result;

    result = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&writeFactory_));
    if (FAILED(result))
        return result;

    auto createFont = [&](float size, DWRITE_FONT_WEIGHT weight, const wchar_t* family, IDWriteTextFormat** output)
        {
            HRESULT hr = writeFactory_->CreateTextFormat(
                family,
                nullptr,
                weight,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                size,
                L"en-US",
                output);

            if (SUCCEEDED(hr))
            {
                (*output)->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                (*output)->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            }
            return hr;
        };

    if (FAILED(result = createFont(11.2f, DWRITE_FONT_WEIGHT_NORMAL, L"Segoe UI", &text_)))
        return result;
    if (FAILED(result = createFont(9.6f, DWRITE_FONT_WEIGHT_NORMAL, L"Segoe UI", &small_)))
        return result;
    if (FAILED(result = createFont(8.4f, DWRITE_FONT_WEIGHT_NORMAL, L"Segoe UI", &tiny_)))
        return result;
    return createFont(15.0f, DWRITE_FONT_WEIGHT_NORMAL, L"Segoe MDL2 Assets", &icon_);
}

HRESULT App::CreateDeviceResources()
{
    if (renderTarget_)
        return S_OK;

    RECT client{};
    GetClientRect(hwnd_, &client);

    HRESULT result = d2dFactory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_HARDWARE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
        D2D1::HwndRenderTargetProperties(
            hwnd_,
            D2D1::SizeU(client.right - client.left, client.bottom - client.top),
            D2D1_PRESENT_OPTIONS_IMMEDIATELY),
        &renderTarget_);

    if (FAILED(result))
        return result;

    return renderTarget_->CreateSolidColorBrush(Theme::White, &brush_);
}

void App::DiscardDeviceResources()
{
    SafeRelease(brush_);
    SafeRelease(renderTarget_);
}

bool App::Initialize(HINSTANCE instance)
{
    SetProcessDPIAware();

    if (FAILED(CreateFactories()))
        return false;

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = StaticWindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.lpszClassName = L"TransparentReferenceMenu";

    if (!RegisterClassExW(&windowClass))
        return false;

    const int x = (GetSystemMetrics(SM_CXSCREEN) - Layout::Width) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - Layout::Height) / 2;

    hwnd_ = CreateWindowExW(
        // WS_EX_APPWINDOW forces this borderless window to appear in the
        // Windows taskbar. WS_EX_TOOLWINDOW intentionally hides it there.
        WS_EX_LAYERED | WS_EX_APPWINDOW,
        windowClass.lpszClassName,
        L"MenuSense",
        WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU,
        x,
        y,
        Layout::Width,
        Layout::Height,
        nullptr,
        nullptr,
        instance,
        this);

    if (!hwnd_)
        return false;

    // Uniform transparency keeps the whole window dark and glass-like.
    SetLayeredWindowAttributes(hwnd_, 0, 235, LWA_ALPHA);

    HRGN region = CreateRoundRectRgn(0, 0, Layout::Width + 1, Layout::Height + 1, 12, 12);
    SetWindowRgn(hwnd_, region, TRUE);

    RefreshConfigs();

    // Give the shell a real taskbar entry and make taskbar clicks restore and
    // activate the window after it has been minimized or covered.
    ShowWindow(hwnd_, SW_SHOWNORMAL);
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
        KillTimer(hwnd_, 1);

    DiscardDeviceResources();
    SafeRelease(icon_);
    SafeRelease(tiny_);
    SafeRelease(small_);
    SafeRelease(text_);
    SafeRelease(writeFactory_);
    SafeRelease(d2dFactory_);
}

void App::SetBrush(const D2D1_COLOR_F& color)
{
    brush_->SetColor(color);
}

void App::FillRect(float left, float top, float right, float bottom, const D2D1_COLOR_F& color)
{
    SetBrush(color);
    renderTarget_->FillRectangle(D2D1::RectF(left, top, right, bottom), brush_);
}

void App::DrawRect(float left, float top, float right, float bottom, const D2D1_COLOR_F& color, float stroke)
{
    SetBrush(color);
    renderTarget_->DrawRectangle(D2D1::RectF(left, top, right, bottom), brush_, stroke);
}

void App::FillRound(float left, float top, float right, float bottom, float radius, const D2D1_COLOR_F& color)
{
    SetBrush(color);
    renderTarget_->FillRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), radius, radius),
        brush_);
}

void App::DrawRound(float left, float top, float right, float bottom, float radius,
    const D2D1_COLOR_F& color, float stroke)
{
    SetBrush(color);
    renderTarget_->DrawRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), radius, radius),
        brush_,
        stroke);
}

void App::DrawLine(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, float stroke)
{
    SetBrush(color);
    renderTarget_->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush_, stroke);
}

void App::DrawText(const std::wstring& value, float left, float top, float right, float bottom,
    const D2D1_COLOR_F& color, IDWriteTextFormat* format, DWRITE_TEXT_ALIGNMENT alignment)
{
    if (!format)
        format = text_;

    format->SetTextAlignment(alignment);
    SetBrush(color);
    renderTarget_->DrawTextW(
        value.c_str(),
        static_cast<UINT32>(value.size()),
        format,
        D2D1::RectF(left, top, right, bottom),
        brush_,
        D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

void App::DrawIcon(const wchar_t* glyph, float left, float top, float right, float bottom,
    const D2D1_COLOR_F& color)
{
    DrawText(glyph, left, top, right, bottom, color, icon_, DWRITE_TEXT_ALIGNMENT_CENTER);
}

bool App::Hover(float left, float top, float right, float bottom) const
{
    return Hit(static_cast<float>(mouse_.x), static_cast<float>(mouse_.y), left, top, right, bottom);
}

void App::Divider(float left, float right, float y)
{
    DrawLine(left, y + 0.5f, right, y + 0.5f, Theme::LineSoft, 1.0f);
}

void App::SidebarButton(int index, const wchar_t* glyph, float y)
{
    const float left = 14.0f;
    const float right = 58.0f;
    const float top = y;
    const float bottom = y + 43.0f;
    const bool hovered = Hover(left, top, right, bottom);
    const bool selected = index == selectedPage_;

    if (hovered)
        FillRound(left, top, right, bottom, 5.0f, Theme::RowHover);

    if (selected)
    {
        FillRound(13.0f, y + 14.0f, 15.0f, y + 29.0f, 1.0f, WithAlpha(settings_.accentColor, 0.90f));
    }

    DrawIcon(
        glyph,
        left,
        top,
        right,
        bottom,
        selected ? Theme::Text : hovered ? Theme::SubText : Theme::Muted);

    if (!colorPickerOpen_ && hovered && clicked_)
    {
        selectedPage_ = index;
        openCombo_ = 0;
        configNameFocused_ = false;
        clickConsumed_ = true;
    }
}

void App::ToggleRow(const std::wstring& label, bool& value, float left, float right, float y, bool dividerBelow)
{
    const float height = 39.0f;
    const bool hovered = Hover(left, y, right, y + height);

    if (hovered)
        FillRect(left, y, right, y + height, WithAlpha(Theme::RowHover, 0.42f));

    DrawText(label, left, y, right - 40.0f, y + height, hovered ? Theme::Text : Theme::SubText, small_);

    const float centerX = right - 9.0f;
    const float centerY = y + height * 0.5f;

    SetBrush(value ? settings_.accentColor : Theme::Muted);
    renderTarget_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(centerX, centerY), 5.2f, 5.2f), brush_, 1.1f);

    if (value)
    {
        SetBrush(settings_.accentColor);
        renderTarget_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(centerX, centerY), 2.3f, 2.3f), brush_);
    }

    if (!colorPickerOpen_ && hovered && clicked_)
    {
        value = !value;
        clickConsumed_ = true;
    }

    if (dividerBelow)
        Divider(left, right, y + height);
}

std::wstring App::FormatNumber(float value, int decimals)
{
    wchar_t buffer[64]{};
    if (decimals <= 0)
        swprintf_s(buffer, L"%.0f", value);
    else
        swprintf_s(buffer, L"%.*f", decimals, value);
    return buffer;
}

void App::SliderControl(const std::wstring& label, float& value, float minimum, float maximum,
    float left, float right, float y, ActiveSlider slider, int decimals)
{
    DrawText(label, left, y, right - 58.0f, y + 21.0f, Theme::SubText, small_);
    DrawText(FormatNumber(value, decimals), right - 56.0f, y, right, y + 21.0f,
        Theme::Text, small_, DWRITE_TEXT_ALIGNMENT_TRAILING);

    const float trackLeft = left;
    const float trackRight = right - 16.0f;
    const float trackY = y + 34.0f;
    const float ratio = Clamp((value - minimum) / (maximum - minimum), 0.0f, 1.0f);
    const float thumbX = trackLeft + (trackRight - trackLeft) * ratio;

    FillRound(trackLeft, trackY - 2.0f, trackRight, trackY + 2.0f, 2.0f, Theme::Track);
    FillRound(trackLeft, trackY - 2.0f, thumbX, trackY + 2.0f, 2.0f, settings_.accentColor);
    FillRound(thumbX - 5.3f, trackY - 5.3f, thumbX + 5.3f, trackY + 5.3f, 2.2f, Theme::White);

    const bool hovered = Hover(trackLeft - 5.0f, trackY - 11.0f, trackRight + 5.0f, trackY + 11.0f);
    if (!colorPickerOpen_ && hovered && clicked_)
    {
        activeSlider_ = slider;
        clickConsumed_ = true;
    }

    if (!colorPickerOpen_ && mouseDown_ && activeSlider_ == slider)
    {
        const float mouseRatio = Clamp((static_cast<float>(mouse_.x) - trackLeft) / (trackRight - trackLeft), 0.0f, 1.0f);
        value = minimum + mouseRatio * (maximum - minimum);
    }
}

void App::ComboControl(int id, const std::wstring& label, const std::vector<std::wstring>& items,
    int& selected, float left, float right, float y)
{
    selected = std::clamp(selected, 0, static_cast<int>(items.size()) - 1);

    DrawText(label, left, y, right, y + 20.0f, Theme::SubText, small_);

    const float boxTop = y + 25.0f;
    const float boxBottom = boxTop + 38.0f;
    const bool hovered = Hover(left, boxTop, right, boxBottom);
    const bool opened = openCombo_ == id;

    FillRect(left, boxTop, right, boxBottom, hovered || opened ? Theme::RowHover : Theme::Row);
    DrawLine(left, boxBottom - 0.5f, right, boxBottom - 0.5f, Theme::LineSoft);

    DrawText(items[selected], left + 13.0f, boxTop, right - 38.0f, boxBottom,
        Theme::Text, small_);
    DrawIcon(Glyph::Chevron, right - 31.0f, boxTop, right - 7.0f, boxBottom,
        opened ? settings_.accentColor : Theme::Muted);

    if (!colorPickerOpen_ && hovered && clicked_)
    {
        openCombo_ = opened ? 0 : id;
        clickConsumed_ = true;
    }

    if (openCombo_ == id)
    {
        comboPopup_.id = id;
        comboPopup_.left = left;
        comboPopup_.top = boxBottom + 3.0f;
        comboPopup_.right = right;
        comboPopup_.itemHeight = 30.0f;
        comboPopup_.items = items;
        comboPopup_.selected = &selected;
    }
}

void App::ColorRow(const std::wstring& label, D2D1_COLOR_F& color,
    float left, float right, float y)
{
    const float height = 41.0f;
    const bool hovered = Hover(left, y, right, y + height);

    if (hovered && !colorPickerOpen_)
        FillRect(left, y, right, y + height, WithAlpha(Theme::RowHover, 0.42f));

    DrawText(label, left, y, right - 45.0f, y + height,
        hovered && !colorPickerOpen_ ? Theme::Text : Theme::SubText, small_);

    const float swatchLeft = right - 22.0f;
    const float swatchTop = y + 15.0f;
    FillRound(swatchLeft, swatchTop, right - 3.0f, swatchTop + 10.0f, 2.0f, color);
    DrawRound(swatchLeft, swatchTop, right - 3.0f, swatchTop + 10.0f,
        2.0f, WithAlpha(Theme::White, 0.10f));

    if (!colorPickerOpen_ && hovered && clicked_)
    {
        OpenColorPicker(color);
        clickConsumed_ = true;
    }
}

bool App::ActionButton(const std::wstring& label, const wchar_t* glyph,
    float left, float top, float right, float bottom, bool accent, bool danger)
{
    const bool hovered = Hover(left, top, right, bottom);

    D2D1_COLOR_F background = Theme::Row;
    D2D1_COLOR_F border = Theme::Line;
    D2D1_COLOR_F foreground = Theme::Text;

    if (accent)
    {
        background = hovered
            ? WithAlpha(settings_.accentColor, 0.90f)
            : WithAlpha(settings_.accentColor, 0.72f);
        border = settings_.accentColor;
        foreground = Theme::White;
    }
    else if (danger)
    {
        background = hovered ? MakeColor(0x35151B) : MakeColor(0x241116);
        border = WithAlpha(Theme::Danger, hovered ? 0.95f : 0.55f);
        foreground = hovered ? Theme::White : Theme::Danger;
    }
    else if (hovered)
    {
        background = Theme::RowHover;
        border = WithAlpha(Theme::White, 0.12f);
    }

    FillRound(left, top, right, bottom, 4.0f, background);
    DrawRound(left, top, right, bottom, 4.0f, border, 1.0f);

    if (glyph)
        DrawIcon(glyph, left + 8.0f, top, left + 32.0f, bottom,
            accent ? Theme::White : danger ? Theme::Danger : hovered ? Theme::Text : Theme::Muted);

    DrawText(label,
        left + (glyph ? 35.0f : 8.0f), top, right - 8.0f, bottom,
        foreground, small_, DWRITE_TEXT_ALIGNMENT_CENTER);

    if (!colorPickerOpen_ && hovered && clicked_)
    {
        clickConsumed_ = true;
        return true;
    }

    return false;
}

void App::TextInput(const std::wstring& label, std::wstring& value, bool& focused,
    float left, float right, float y)
{
    DrawText(label, left, y, right, y + 18.0f, Theme::SubText, small_);

    const float top = y + 23.0f;
    const float bottom = top + 38.0f;
    const bool hovered = Hover(left, top, right, bottom);

    FillRound(left, top, right, bottom, 4.0f,
        hovered || focused ? Theme::RowHover : Theme::Row);
    DrawRound(left, top, right, bottom, 4.0f,
        focused ? settings_.accentColor : hovered ? WithAlpha(Theme::White, 0.14f) : Theme::Line,
        1.0f);

    DrawText(value.empty() ? L"Enter config name" : value,
        left + 12.0f, top, right - 12.0f, bottom,
        value.empty() ? Theme::Muted : Theme::Text, small_);

    if (focused && (GetTickCount64() / 500ULL) % 2ULL == 0ULL)
    {
        const float estimatedWidth = std::min(
            static_cast<float>(value.size()) * 5.6f,
            (right - left) - 31.0f);
        DrawLine(left + 12.0f + estimatedWidth, top + 10.0f,
            left + 12.0f + estimatedWidth, bottom - 10.0f,
            settings_.accentColor, 1.0f);
    }

    if (!colorPickerOpen_ && hovered && clicked_)
    {
        focused = true;
        openCombo_ = 0;
        clickConsumed_ = true;
        SetFocus(hwnd_);
    }
}

void App::StatusMessage(const std::wstring& text, bool success)
{
    statusText_ = text;
    statusSuccess_ = success;
    statusTimer_ = 3.2f;
}

void App::DrawComboPopup()
{
    if (colorPickerOpen_ || openCombo_ == 0 || comboPopup_.id != openCombo_ || !comboPopup_.selected)
        return;

    const float popupBottom = comboPopup_.top + comboPopup_.itemHeight * static_cast<float>(comboPopup_.items.size());

    FillRound(comboPopup_.left, comboPopup_.top, comboPopup_.right, popupBottom, 3.0f, Theme::Popup);
    DrawRound(comboPopup_.left, comboPopup_.top, comboPopup_.right, popupBottom, 3.0f, Theme::Line);

    for (std::size_t index = 0; index < comboPopup_.items.size(); ++index)
    {
        const float itemTop = comboPopup_.top + comboPopup_.itemHeight * static_cast<float>(index);
        const float itemBottom = itemTop + comboPopup_.itemHeight;
        const bool hovered = Hover(comboPopup_.left, itemTop, comboPopup_.right, itemBottom);
        const bool selected = static_cast<int>(index) == *comboPopup_.selected;

        if (hovered)
            FillRect(comboPopup_.left + 2.0f, itemTop + 1.0f,
                comboPopup_.right - 2.0f, itemBottom - 1.0f, Theme::PopupHover);

        if (selected)
            FillRect(comboPopup_.left + 5.0f, itemTop + 8.0f,
                comboPopup_.left + 7.0f, itemBottom - 8.0f, settings_.accentColor);

        DrawText(comboPopup_.items[index], comboPopup_.left + 13.0f, itemTop,
            comboPopup_.right - 10.0f, itemBottom,
            selected ? settings_.accentColor : hovered ? Theme::Text : Theme::SubText,
            small_);

        if (hovered && clicked_)
        {
            *comboPopup_.selected = static_cast<int>(index);
            openCombo_ = 0;
            clickConsumed_ = true;
        }
    }
}

void App::OpenColorPicker(D2D1_COLOR_F& color)
{
    pickerTarget_ = &color;
    pickerOriginal_ = color;
    pickerWorking_ = color;
    ColorToHsv(color, pickerHue_, pickerSaturation_, pickerValue_);
    colorPickerOpen_ = true;
    activeColorArea_ = -1;
    activeSlider_ = ActiveSlider::None;
    openCombo_ = 0;
}

void App::UpdatePickerColorFromHsv()
{
    pickerWorking_ = HsvToColor(pickerHue_, pickerSaturation_, pickerValue_);
}

void App::DrawColorPicker()
{
    if (!colorPickerOpen_ || !pickerTarget_)
        return;

    FillRect(0.0f, 0.0f, static_cast<float>(Layout::Width),
        static_cast<float>(Layout::Height), MakeColor(0x000000, 0.72f));

    const float boxWidth = 350.0f;
    const float boxHeight = 292.0f;
    const float x = (static_cast<float>(Layout::Width) - boxWidth) * 0.5f;
    const float y = (static_cast<float>(Layout::Height) - boxHeight) * 0.5f;

    FillRect(x, y, x + boxWidth, y + boxHeight, MakeColor(0x0D0E12));
    DrawRect(x, y, x + boxWidth, y + boxHeight, settings_.accentColor, 1.0f);

    DrawText(L"Color picker", x + 15.0f, y + 3.0f,
        x + boxWidth - 15.0f, y + 34.0f, Theme::Text, text_);

    DrawLine(x + 14.0f, y + 37.0f, x + boxWidth - 14.0f, y + 37.0f,
        Theme::Line, 1.0f);

    const float paletteX = x + 18.0f;
    const float paletteY = y + 53.0f;
    const float paletteWidth = 265.0f;
    const float paletteHeight = 174.0f;
    const float hueX = paletteX + paletteWidth + 10.0f;
    const float hueWidth = 18.0f;

    constexpr int columns = 64;
    constexpr int rows = 42;
    const float cellWidth = paletteWidth / static_cast<float>(columns);
    const float cellHeight = paletteHeight / static_cast<float>(rows);

    for (int row = 0; row < rows; ++row)
    {
        const float value = 1.0f - static_cast<float>(row) / static_cast<float>(rows - 1);

        for (int column = 0; column < columns; ++column)
        {
            const float saturation = static_cast<float>(column) / static_cast<float>(columns - 1);
            const float left = paletteX + cellWidth * static_cast<float>(column);
            const float top = paletteY + cellHeight * static_cast<float>(row);

            FillRect(left, top, left + cellWidth + 0.8f, top + cellHeight + 0.8f,
                HsvToColor(pickerHue_, saturation, value));
        }
    }

    constexpr int hueSteps = 90;
    const float hueStepHeight = paletteHeight / static_cast<float>(hueSteps);

    for (int step = 0; step < hueSteps; ++step)
    {
        const float hue = static_cast<float>(step) / static_cast<float>(hueSteps - 1);
        const float top = paletteY + hueStepHeight * static_cast<float>(step);

        FillRect(hueX, top, hueX + hueWidth, top + hueStepHeight + 0.8f,
            HsvToColor(hue, 1.0f, 1.0f));
    }

    const bool paletteHover = Hover(
        paletteX, paletteY, paletteX + paletteWidth, paletteY + paletteHeight);
    const bool hueHover = Hover(
        hueX, paletteY, hueX + hueWidth, paletteY + paletteHeight);

    if (clicked_ && paletteHover)
    {
        activeColorArea_ = 0;
        clickConsumed_ = true;
    }
    else if (clicked_ && hueHover)
    {
        activeColorArea_ = 1;
        clickConsumed_ = true;
    }

    if (mouseDown_ && activeColorArea_ == 0)
    {
        pickerSaturation_ = Clamp(
            (static_cast<float>(mouse_.x) - paletteX) / paletteWidth,
            0.0f,
            1.0f);

        pickerValue_ = 1.0f - Clamp(
            (static_cast<float>(mouse_.y) - paletteY) / paletteHeight,
            0.0f,
            1.0f);

        UpdatePickerColorFromHsv();
    }
    else if (mouseDown_ && activeColorArea_ == 1)
    {
        pickerHue_ = Clamp(
            (static_cast<float>(mouse_.y) - paletteY) / paletteHeight,
            0.0f,
            1.0f);

        UpdatePickerColorFromHsv();
    }

    const float pointX = paletteX + pickerSaturation_ * paletteWidth;
    const float pointY = paletteY + (1.0f - pickerValue_) * paletteHeight;

    SetBrush(MakeColor(0x000000));
    renderTarget_->DrawEllipse(
        D2D1::Ellipse(D2D1::Point2F(pointX, pointY), 5.2f, 5.2f),
        brush_,
        3.0f);

    SetBrush(Theme::White);
    renderTarget_->DrawEllipse(
        D2D1::Ellipse(D2D1::Point2F(pointX, pointY), 5.2f, 5.2f),
        brush_,
        1.4f);

    const float hueMarkerY = paletteY + pickerHue_ * paletteHeight;

    FillRect(hueX - 3.0f, hueMarkerY - 2.0f,
        hueX + hueWidth + 3.0f, hueMarkerY + 2.0f, Theme::White);

    DrawRect(hueX - 4.0f, hueMarkerY - 3.0f,
        hueX + hueWidth + 4.0f, hueMarkerY + 3.0f,
        MakeColor(0x000000), 1.0f);

    FillRect(x + 18.0f, y + 239.0f, x + 43.0f, y + 258.0f, pickerWorking_);
    DrawRect(x + 18.0f, y + 239.0f, x + 43.0f, y + 258.0f,
        WithAlpha(Theme::White, 0.12f), 1.0f);

    wchar_t hexText[16]{};
    const int red = std::clamp(
        static_cast<int>(std::round(pickerWorking_.r * 255.0f)), 0, 255);
    const int green = std::clamp(
        static_cast<int>(std::round(pickerWorking_.g * 255.0f)), 0, 255);
    const int blue = std::clamp(
        static_cast<int>(std::round(pickerWorking_.b * 255.0f)), 0, 255);

    swprintf_s(hexText, L"#%02X%02X%02X", red, green, blue);

    DrawText(hexText, x + 51.0f, y + 232.0f,
        x + 163.0f, y + 263.0f, Theme::Text, small_);

    const float buttonY = y + boxHeight - 36.0f;
    const float cancelX = x + boxWidth - 150.0f;
    const float applyX = x + boxWidth - 76.0f;

    const bool cancelHover = Hover(
        cancelX, buttonY, cancelX + 64.0f, buttonY + 24.0f);
    const bool applyHover = Hover(
        applyX, buttonY, applyX + 58.0f, buttonY + 24.0f);

    FillRect(cancelX, buttonY, cancelX + 64.0f, buttonY + 24.0f,
        cancelHover ? Theme::RowHover : Theme::Row);
    DrawRect(cancelX, buttonY, cancelX + 64.0f, buttonY + 24.0f,
        Theme::Line, 1.0f);
    DrawText(L"Cancel", cancelX, buttonY,
        cancelX + 64.0f, buttonY + 24.0f,
        Theme::Text, small_, DWRITE_TEXT_ALIGNMENT_CENTER);

    FillRect(applyX, buttonY, applyX + 58.0f, buttonY + 24.0f,
        applyHover ? WithAlpha(settings_.accentColor, 0.82f) : settings_.accentColor);
    DrawText(L"Apply", applyX, buttonY,
        applyX + 58.0f, buttonY + 24.0f,
        Theme::White, small_, DWRITE_TEXT_ALIGNMENT_CENTER);

    if (clicked_ && cancelHover)
    {
        *pickerTarget_ = pickerOriginal_;
        colorPickerOpen_ = false;
        pickerTarget_ = nullptr;
        activeColorArea_ = -1;
        clickConsumed_ = true;
    }
    else if (clicked_ && applyHover)
    {
        *pickerTarget_ = pickerWorking_;
        colorPickerOpen_ = false;
        pickerTarget_ = nullptr;
        activeColorArea_ = -1;
        clickConsumed_ = true;
    }
}

std::filesystem::path App::ConfigDirectory() const
{
    wchar_t appData[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH);

    std::filesystem::path directory = length > 0
        ? std::filesystem::path(appData) / L"MenuSense" / L"Configs"
        : std::filesystem::current_path() / L"Configs";

    std::error_code error;
    std::filesystem::create_directories(directory, error);
    return directory;
}

std::wstring App::SanitizeConfigName(const std::wstring& name) const
{
    std::wstring clean;
    clean.reserve(name.size());

    for (wchar_t character : name)
    {
        if (std::iswalnum(character) || character == L' ' ||
            character == L'-' || character == L'_')
        {
            clean.push_back(character);
        }
    }

    while (!clean.empty() && clean.front() == L' ')
        clean.erase(clean.begin());
    while (!clean.empty() && clean.back() == L' ')
        clean.pop_back();

    if (clean.size() > 32)
        clean.resize(32);

    return clean;
}

std::filesystem::path App::ConfigPath(const std::wstring& name) const
{
    return ConfigDirectory() / (SanitizeConfigName(name) + L".cfg");
}

bool App::SaveConfig(const std::wstring& name)
{
    const std::wstring cleanName = SanitizeConfigName(name);
    if (cleanName.empty())
        return false;

    std::ofstream file(ConfigPath(cleanName), std::ios::binary | std::ios::trunc);
    if (!file)
        return false;

    const std::uint32_t magic = 0x4D534346; // MSCF
    const std::uint32_t version = 2;

    auto writeValue = [&](const auto& value)
        {
            file.write(reinterpret_cast<const char*>(&value), sizeof(value));
        };

    auto writeColor = [&](const D2D1_COLOR_F& color)
        {
            writeValue(color.r);
            writeValue(color.g);
            writeValue(color.b);
            writeValue(color.a);
        };

    writeValue(magic);
    writeValue(version);

    writeValue(settings_.aimbot);
    writeValue(settings_.triggerbot);
    writeValue(settings_.enableScroller);
    writeValue(settings_.disableRain);
    writeValue(settings_.bunnyhop);
    writeValue(settings_.fastDuck);
    writeValue(settings_.longJump);
    writeValue(settings_.connectedParticles);

    writeValue(settings_.aimbotFov);
    writeValue(settings_.dpiScale);
    writeValue(settings_.particleSpeed);

    writeValue(settings_.hitbox);
    writeValue(settings_.multipoint);

    writeColor(settings_.pickerColor);
    writeColor(settings_.accentColor);

    file.flush();
    return file.good();
}

bool App::LoadConfig(const std::wstring& name)
{
    const std::wstring cleanName = SanitizeConfigName(name);
    if (cleanName.empty())
        return false;

    std::ifstream file(ConfigPath(cleanName), std::ios::binary);
    if (!file)
        return false;

    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    Settings loaded = settings_;

    auto readValue = [&](auto& value) -> bool
        {
            file.read(reinterpret_cast<char*>(&value), sizeof(value));
            return static_cast<bool>(file);
        };

    auto readColor = [&](D2D1_COLOR_F& color) -> bool
        {
            return readValue(color.r) && readValue(color.g) &&
                readValue(color.b) && readValue(color.a);
        };

    if (!readValue(magic) || !readValue(version) ||
        magic != 0x4D534346 || version != 2)
    {
        return false;
    }

    if (!readValue(loaded.aimbot) ||
        !readValue(loaded.triggerbot) ||
        !readValue(loaded.enableScroller) ||
        !readValue(loaded.disableRain) ||
        !readValue(loaded.bunnyhop) ||
        !readValue(loaded.fastDuck) ||
        !readValue(loaded.longJump) ||
        !readValue(loaded.connectedParticles) ||
        !readValue(loaded.aimbotFov) ||
        !readValue(loaded.dpiScale) ||
        !readValue(loaded.particleSpeed) ||
        !readValue(loaded.hitbox) ||
        !readValue(loaded.multipoint) ||
        !readColor(loaded.pickerColor) ||
        !readColor(loaded.accentColor))
    {
        return false;
    }

    loaded.aimbotFov = Clamp(loaded.aimbotFov, 0.0f, 100.0f);
    loaded.dpiScale = Clamp(loaded.dpiScale, 80.0f, 200.0f);
    loaded.particleSpeed = Clamp(loaded.particleSpeed, 0.0f, 60.0f);
    loaded.hitbox = std::clamp(loaded.hitbox, 0, 3);
    loaded.multipoint = std::clamp(loaded.multipoint, 0, 3);
    loaded.pickerColor.a = 1.0f;
    loaded.accentColor.a = 1.0f;

    settings_ = loaded;
    return true;
}

bool App::DeleteConfig(const std::wstring& name)
{
    const std::wstring cleanName = SanitizeConfigName(name);
    if (cleanName.empty())
        return false;

    std::error_code error;
    return std::filesystem::remove(ConfigPath(cleanName), error) && !error;
}

void App::RefreshConfigs()
{
    const std::wstring previous = SelectedConfig();
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

    EnsureSelectedConfigVisible();
}

void App::EnsureSelectedConfigVisible()
{
    constexpr int visibleRows = 7;
    const int maximumScroll = std::max(
        0,
        static_cast<int>(configs_.size()) - visibleRows);

    configScrollIndex_ = std::clamp(
        configScrollIndex_,
        0,
        maximumScroll);

    if (!HasSelectedConfig())
        return;

    if (selectedConfig_ < configScrollIndex_)
    {
        configScrollIndex_ = selectedConfig_;
    }
    else if (selectedConfig_ >= configScrollIndex_ + visibleRows)
    {
        configScrollIndex_ = selectedConfig_ - visibleRows + 1;
    }

    configScrollIndex_ = std::clamp(
        configScrollIndex_,
        0,
        maximumScroll);
}

bool App::HasSelectedConfig() const
{
    return selectedConfig_ >= 0 &&
        selectedConfig_ < static_cast<int>(configs_.size());
}

std::wstring App::SelectedConfig() const
{
    return HasSelectedConfig() ? configs_[selectedConfig_] : L"";
}

void App::DrawConnectedParticles(float width, float height)
{
    if (!settings_.connectedParticles)
        return;

    constexpr std::size_t particleCount = 26;
    constexpr float connectionDistance = 92.0f;
    std::array<D2D1_POINT_2F, particleCount> points{};

    const float time = static_cast<float>(GetTickCount64() % 1000000ULL) / 1000.0f;
    const float speed = settings_.particleSpeed / 22.0f;
    const float left = Layout::Sidebar + 9.0f;
    const float usableWidth = std::max(1.0f, width - left - 10.0f);
    const float usableHeight = std::max(1.0f, height - 20.0f);

    for (std::size_t index = 0; index < particleCount; ++index)
    {
        const float seed = static_cast<float>(index);
        const float baseX = std::fmod(seed * 87.13f + 19.0f, usableWidth);
        const float baseY = std::fmod(seed * 53.71f + 31.0f, usableHeight);
        const float driftX = time * speed * (2.2f + std::fmod(seed * 1.91f, 4.8f));
        const float driftY = time * speed * (1.2f + std::fmod(seed * 1.37f, 3.2f));

        points[index].x = left + std::fmod(
            baseX + driftX + std::sin(time * 0.43f + seed) * 10.0f + usableWidth,
            usableWidth);
        points[index].y = 10.0f + std::fmod(
            baseY + driftY + std::cos(time * 0.37f + seed * 0.7f) * 8.0f + usableHeight,
            usableHeight);
    }

    for (std::size_t first = 0; first < particleCount; ++first)
    {
        for (std::size_t second = first + 1; second < particleCount; ++second)
        {
            const float dx = points[first].x - points[second].x;
            const float dy = points[first].y - points[second].y;
            const float distance = std::sqrt(dx * dx + dy * dy);

            if (distance < connectionDistance)
            {
                const float alpha = 0.13f * (1.0f - distance / connectionDistance);
                DrawLine(points[first].x, points[first].y,
                    points[second].x, points[second].y,
                    WithAlpha(Theme::White, alpha), 0.8f);
            }
        }
    }

    SetBrush(WithAlpha(Theme::White, 0.48f));
    for (const auto& point : points)
    {
        renderTarget_->FillEllipse(
            D2D1::Ellipse(point, 1.25f, 1.25f),
            brush_);
    }
}

void App::DrawSettingsPage()
{
    DrawLine(Layout::SplitX + 0.5f, 13.0f, Layout::SplitX + 0.5f, 447.0f, Theme::LineSoft);

    DrawText(L"Settings", Layout::LeftX, 20.0f, Layout::LeftRight, 48.0f,
        Theme::Text, text_);
    DrawText(L"Interface and animated background", Layout::RightX, 20.0f,
        Layout::RightRight, 48.0f, Theme::SubText, small_, DWRITE_TEXT_ALIGNMENT_TRAILING);

    Divider(Layout::LeftX, Layout::LeftRight, 61.0f);
    Divider(Layout::RightX, Layout::RightRight, 61.0f);

    DrawText(L"BACKGROUND", Layout::LeftX, 72.0f, Layout::LeftRight, 92.0f,
        Theme::Muted, tiny_);

    ToggleRow(L"Connected white particles", settings_.connectedParticles,
        Layout::LeftX, Layout::LeftRight, 96.0f);

    SliderControl(L"Particle speed", settings_.particleSpeed, 0.0f, 60.0f,
        Layout::LeftX, Layout::LeftRight, 143.0f, ActiveSlider::ParticleSpeed, 0);

    Divider(Layout::LeftX, Layout::LeftRight, 202.0f);
    DrawText(L"WINDOW", Layout::LeftX, 214.0f, Layout::LeftRight, 234.0f,
        Theme::Muted, tiny_);

    ToggleRow(L"Enable scroller", settings_.enableScroller,
        Layout::LeftX, Layout::LeftRight, 238.0f);

    ColorRow(L"Accent color", settings_.accentColor,
        Layout::LeftX, Layout::LeftRight, 281.0f);

    FillRound(Layout::RightX, 82.0f, Layout::RightRight, 256.0f,
        6.0f, WithAlpha(Theme::Panel, 0.88f));
    DrawRound(Layout::RightX, 82.0f, Layout::RightRight, 256.0f,
        6.0f, Theme::Line, 1.0f);

    DrawIcon(Glyph::Sparkle, Layout::RightX, 94.0f,
        Layout::RightRight, 135.0f, Theme::White);
    DrawText(L"CONNECTED PARTICLES", Layout::RightX + 18.0f, 137.0f,
        Layout::RightRight - 18.0f, 161.0f, Theme::Text, small_,
        DWRITE_TEXT_ALIGNMENT_CENTER);
    DrawText(L"Animated white points connect when they move close to each other.",
        Layout::RightX + 26.0f, 166.0f, Layout::RightRight - 26.0f, 215.0f,
        Theme::SubText, tiny_, DWRITE_TEXT_ALIGNMENT_CENTER);

    const D2D1_COLOR_F stateColor = settings_.connectedParticles
        ? Theme::Success : Theme::Muted;
    FillRound(Layout::RightX + 83.0f, 220.0f,
        Layout::RightRight - 83.0f, 243.0f, 11.5f,
        WithAlpha(stateColor, 0.14f));
    DrawText(settings_.connectedParticles ? L"ENABLED" : L"DISABLED",
        Layout::RightX + 83.0f, 220.0f, Layout::RightRight - 83.0f, 243.0f,
        stateColor, tiny_, DWRITE_TEXT_ALIGNMENT_CENTER);

    FillRound(Layout::RightX, 278.0f, Layout::RightRight, 382.0f,
        6.0f, WithAlpha(Theme::Panel, 0.80f));
    DrawRound(Layout::RightX, 278.0f, Layout::RightRight, 382.0f,
        6.0f, Theme::Line, 1.0f);
    DrawText(L"Transparent black window", Layout::RightX + 18.0f, 292.0f,
        Layout::RightRight - 18.0f, 318.0f, Theme::Text, small_);
    DrawText(L"The background stays dark and glass-like while the white network moves behind every page.",
        Layout::RightX + 18.0f, 320.0f, Layout::RightRight - 18.0f, 367.0f,
        Theme::SubText, tiny_);
}

void App::DrawProfilePage()
{
    DrawLine(Layout::SplitX + 0.5f, 13.0f, Layout::SplitX + 0.5f, 447.0f, Theme::LineSoft);

    DrawText(L"Profile", Layout::LeftX, 20.0f, Layout::LeftRight, 48.0f,
        Theme::Text, text_);
    DrawText(L"Configuration manager", Layout::RightX, 20.0f,
        Layout::RightRight, 48.0f, Theme::SubText, small_, DWRITE_TEXT_ALIGNMENT_TRAILING);

    Divider(Layout::LeftX, Layout::LeftRight, 61.0f);
    Divider(Layout::RightX, Layout::RightRight, 61.0f);

    TextInput(L"CONFIG NAME", configName_, configNameFocused_,
        Layout::LeftX, Layout::LeftRight, 79.0f);

    if (ActionButton(L"Save config", Glyph::Save,
        Layout::LeftX, 160.0f, Layout::LeftRight, 199.0f, true))
    {
        configNameFocused_ = false;
        const std::wstring cleanName = SanitizeConfigName(configName_);
        if (cleanName.empty())
        {
            StatusMessage(L"Enter a valid config name.", false);
        }
        else if (SaveConfig(cleanName))
        {
            configName_ = cleanName;
            RefreshConfigs();
            const auto iterator = std::find(configs_.begin(), configs_.end(), cleanName);
            if (iterator != configs_.end())
                selectedConfig_ = static_cast<int>(std::distance(configs_.begin(), iterator));
            EnsureSelectedConfigVisible();
            StatusMessage(L"Config saved successfully.", true);
        }
        else
        {
            StatusMessage(L"Config could not be saved.", false);
        }
    }

    if (ActionButton(L"Load selected", Glyph::Load,
        Layout::LeftX, 212.0f, Layout::LeftRight, 251.0f))
    {
        configNameFocused_ = false;
        if (!HasSelectedConfig())
        {
            StatusMessage(L"Select a config first.", false);
        }
        else if (LoadConfig(SelectedConfig()))
        {
            configName_ = SelectedConfig();
            StatusMessage(L"Config loaded successfully.", true);
        }
        else
        {
            StatusMessage(L"Config could not be loaded.", false);
        }
    }

    if (ActionButton(L"Delete selected", Glyph::Delete,
        Layout::LeftX, 264.0f, Layout::LeftRight, 303.0f, false, true))
    {
        configNameFocused_ = false;
        if (!HasSelectedConfig())
        {
            StatusMessage(L"Select a config first.", false);
        }
        else
        {
            const std::wstring deletedName = SelectedConfig();
            if (DeleteConfig(deletedName))
            {
                configName_.clear();
                RefreshConfigs();
                StatusMessage(L"Config deleted.", true);
            }
            else
            {
                StatusMessage(L"Config could not be deleted.", false);
            }
        }
    }

    FillRound(Layout::LeftX, 328.0f, Layout::LeftRight, 388.0f,
        5.0f, WithAlpha(Theme::Panel, 0.82f));
    DrawRound(Layout::LeftX, 328.0f, Layout::LeftRight, 388.0f,
        5.0f, Theme::Line, 1.0f);
    DrawIcon(Glyph::Info, Layout::LeftX + 10.0f, 328.0f,
        Layout::LeftX + 42.0f, 388.0f, Theme::Muted);
    DrawText(L"Configs save every current menu option and both custom colors.",
        Layout::LeftX + 46.0f, 334.0f, Layout::LeftRight - 12.0f, 382.0f,
        Theme::SubText, tiny_);

    DrawText(L"SAVED CONFIGS", Layout::RightX, 76.0f,
        Layout::RightRight, 96.0f, Theme::Muted, tiny_);

    FillRound(Layout::RightX, 102.0f, Layout::RightRight, 382.0f,
        6.0f, WithAlpha(Theme::Panel, 0.88f));
    DrawRound(Layout::RightX, 102.0f, Layout::RightRight, 382.0f,
        6.0f, Theme::Line, 1.0f);

    if (configs_.empty())
    {
        DrawIcon(Glyph::Folder, Layout::RightX, 151.0f,
            Layout::RightRight, 198.0f, Theme::Muted);
        DrawText(L"No saved configs", Layout::RightX + 18.0f, 200.0f,
            Layout::RightRight - 18.0f, 226.0f, Theme::SubText, small_,
            DWRITE_TEXT_ALIGNMENT_CENTER);
        DrawText(L"Enter a name and save your first profile.",
            Layout::RightX + 28.0f, 229.0f, Layout::RightRight - 28.0f, 259.0f,
            Theme::Muted, tiny_, DWRITE_TEXT_ALIGNMENT_CENTER);
    }
    else
    {
        constexpr int visibleRows = 7;
        constexpr float listTop = 110.0f;
        constexpr float rowStep = 38.0f;
        constexpr float rowHeight = 35.0f;
        constexpr float listBottom = listTop + rowStep * visibleRows;

        const int totalItems = static_cast<int>(configs_.size());
        const int maximumScroll = std::max(0, totalItems - visibleRows);
        const bool showScrollbar = maximumScroll > 0;

        configScrollIndex_ = std::clamp(
            configScrollIndex_,
            0,
            maximumScroll);

        const float itemRight = showScrollbar
            ? Layout::RightRight - 20.0f
            : Layout::RightRight - 7.0f;

        const int visibleCount = std::min(
            visibleRows,
            totalItems - configScrollIndex_);

        for (int visibleIndex = 0; visibleIndex < visibleCount; ++visibleIndex)
        {
            const int configIndex = configScrollIndex_ + visibleIndex;
            const float rowTop = listTop + rowStep * visibleIndex;
            const float rowBottom = rowTop + rowHeight;
            const bool selected = configIndex == selectedConfig_;
            const bool hovered = Hover(
                Layout::RightX + 7.0f,
                rowTop,
                itemRight,
                rowBottom);

            if (selected || hovered)
            {
                FillRound(
                    Layout::RightX + 7.0f,
                    rowTop,
                    itemRight,
                    rowBottom,
                    4.0f,
                    selected
                    ? WithAlpha(settings_.accentColor, 0.14f)
                    : Theme::RowHover);
            }

            if (selected)
            {
                FillRound(
                    Layout::RightX + 8.0f,
                    rowTop + 8.0f,
                    Layout::RightX + 10.5f,
                    rowBottom - 8.0f,
                    1.0f,
                    settings_.accentColor);
            }

            DrawIcon(
                Glyph::Save,
                Layout::RightX + 16.0f,
                rowTop,
                Layout::RightX + 42.0f,
                rowBottom,
                selected
                ? settings_.accentColor
                : hovered
                ? Theme::SubText
                : Theme::Muted);

            DrawText(
                configs_[configIndex],
                Layout::RightX + 48.0f,
                rowTop,
                itemRight - 8.0f,
                rowBottom,
                selected
                ? Theme::Text
                : hovered
                ? Theme::Text
                : Theme::SubText,
                small_);

            if (!colorPickerOpen_ && hovered && clicked_)
            {
                selectedConfig_ = configIndex;
                configName_ = configs_[configIndex];
                configNameFocused_ = false;
                clickConsumed_ = true;
            }
        }

        if (showScrollbar)
        {
            const float trackLeft = Layout::RightRight - 12.0f;
            const float trackRight = Layout::RightRight - 8.0f;
            const float trackTop = listTop;
            const float trackBottom = listBottom - 3.0f;
            const float trackHeight = trackBottom - trackTop;

            const float thumbHeight = std::max(
                34.0f,
                trackHeight *
                static_cast<float>(visibleRows) /
                static_cast<float>(totalItems));

            const float thumbTravel = std::max(
                1.0f,
                trackHeight - thumbHeight);

            float thumbTop = trackTop +
                thumbTravel *
                static_cast<float>(configScrollIndex_) /
                static_cast<float>(maximumScroll);

            bool thumbHovered = Hover(
                trackLeft - 4.0f,
                thumbTop - 2.0f,
                trackRight + 4.0f,
                thumbTop + thumbHeight + 2.0f);

            const bool trackHovered = Hover(
                trackLeft - 5.0f,
                trackTop,
                trackRight + 5.0f,
                trackBottom);

            if (!colorPickerOpen_ && clicked_ && trackHovered)
            {
                if (thumbHovered)
                {
                    configScrollbarGrabOffset_ =
                        static_cast<float>(mouse_.y) - thumbTop;
                }
                else
                {
                    configScrollbarGrabOffset_ = thumbHeight * 0.5f;
                    const float ratio = Clamp(
                        (static_cast<float>(mouse_.y) -
                            trackTop -
                            configScrollbarGrabOffset_) /
                        thumbTravel,
                        0.0f,
                        1.0f);

                    configScrollIndex_ = static_cast<int>(std::round(
                        ratio * static_cast<float>(maximumScroll)));
                }

                configScrollbarDragging_ = true;
                clickConsumed_ = true;
            }

            if (mouseDown_ && configScrollbarDragging_)
            {
                const float ratio = Clamp(
                    (static_cast<float>(mouse_.y) -
                        trackTop -
                        configScrollbarGrabOffset_) /
                    thumbTravel,
                    0.0f,
                    1.0f);

                configScrollIndex_ = static_cast<int>(std::round(
                    ratio * static_cast<float>(maximumScroll)));

                configScrollIndex_ = std::clamp(
                    configScrollIndex_,
                    0,
                    maximumScroll);

                thumbTop = trackTop +
                    thumbTravel *
                    static_cast<float>(configScrollIndex_) /
                    static_cast<float>(maximumScroll);

                thumbHovered = true;
            }

            // Accent-derived track and thumb so the scrollbar follows the
            // currently selected custom accent color.
            FillRound(
                trackLeft,
                trackTop,
                trackRight,
                trackBottom,
                2.0f,
                WithAlpha(settings_.accentColor, 0.12f));

            FillRound(
                trackLeft,
                thumbTop,
                trackRight,
                thumbTop + thumbHeight,
                2.0f,
                WithAlpha(
                    settings_.accentColor,
                    thumbHovered || configScrollbarDragging_
                    ? 1.0f
                    : 0.72f));
        }
    }

    if (!statusText_.empty())
    {
        const D2D1_COLOR_F statusColor = statusSuccess_ ? Theme::Success : Theme::Danger;
        FillRound(Layout::LeftX, 405.0f, Layout::RightRight, 442.0f,
            5.0f, WithAlpha(statusColor, 0.10f));
        DrawRound(Layout::LeftX, 405.0f, Layout::RightRight, 442.0f,
            5.0f, WithAlpha(statusColor, 0.52f), 1.0f);
        DrawIcon(statusSuccess_ ? Glyph::Check : Glyph::Info,
            Layout::LeftX + 8.0f, 405.0f, Layout::LeftX + 38.0f, 442.0f,
            statusColor);
        DrawText(statusText_, Layout::LeftX + 43.0f, 405.0f,
            Layout::RightRight - 12.0f, 442.0f, statusColor, small_);
    }
}

void App::DrawSidebar(float height)
{
    FillRect(0.0f, 0.0f, Layout::Sidebar, height, Theme::Sidebar);
    DrawLine(Layout::Sidebar - 0.5f, 0.0f, Layout::Sidebar - 0.5f, height, Theme::LineSoft);

    SidebarButton(0, Glyph::Target, 18.0f);
    SidebarButton(1, Glyph::Eye, 72.0f);
    SidebarButton(2, Glyph::Settings, 126.0f);
    SidebarButton(3, Glyph::Profile, 180.0f);
    SidebarButton(4, Glyph::Cloud, 234.0f);
}

void App::DrawMainPage()
{
    // Main vertical separator.
    DrawLine(Layout::SplitX + 0.5f, 13.0f, Layout::SplitX + 0.5f, 447.0f, Theme::LineSoft);

    // LEFT COLUMN ------------------------------------------------------------
    ColorRow(L"Colorpicker", settings_.pickerColor,
        Layout::LeftX, Layout::LeftRight, 16.0f);

    ToggleRow(L"Aimbot", settings_.aimbot, Layout::LeftX, Layout::LeftRight, 58.0f);

    SliderControl(L"Aimbot FOV", settings_.aimbotFov, 0.0f, 100.0f,
        Layout::LeftX, Layout::LeftRight, 101.0f, ActiveSlider::AimbotFov, 1);

    Divider(Layout::LeftX, Layout::LeftRight, 158.0f);

    ComboControl(1, L"Hitboxes", { L"Body", L"Head", L"Chest", L"Closest" },
        settings_.hitbox, Layout::LeftX, Layout::LeftRight, 173.0f);

    ComboControl(2, L"Multipoints", { L"Body", L"Head", L"Chest", L"All" },
        settings_.multipoint, Layout::LeftX, Layout::LeftRight, 250.0f);

    Divider(Layout::LeftX, Layout::LeftRight, 335.0f);
    ToggleRow(L"Triggerbot", settings_.triggerbot, Layout::LeftX, Layout::LeftRight, 350.0f);

    // RIGHT COLUMN -----------------------------------------------------------
    SliderControl(L"Dpi scale", settings_.dpiScale, 80.0f, 200.0f,
        Layout::RightX, Layout::RightRight, 16.0f, ActiveSlider::DpiScale, 1);

    ColorRow(L"Accent color", settings_.accentColor,
        Layout::RightX, Layout::RightRight, 73.0f);

    Divider(Layout::RightX, Layout::RightRight, 116.0f);
    ToggleRow(L"Enable scroller", settings_.enableScroller,
        Layout::RightX, Layout::RightRight, 121.0f);

    Divider(Layout::RightX, Layout::RightRight, 166.0f);
    ToggleRow(L"Disable rain", settings_.disableRain,
        Layout::RightX, Layout::RightRight, 171.0f);
    ToggleRow(L"Bunnyhope", settings_.bunnyhop,
        Layout::RightX, Layout::RightRight, 212.0f);
    ToggleRow(L"Fast duck", settings_.fastDuck,
        Layout::RightX, Layout::RightRight, 253.0f);
    ToggleRow(L"Long jump", settings_.longJump,
        Layout::RightX, Layout::RightRight, 294.0f);
}

void App::DrawOtherPage(int page)
{
    if (page == 2)
    {
        DrawSettingsPage();
        return;
    }

    if (page == 3)
    {
        DrawProfilePage();
        return;
    }

    DrawLine(Layout::SplitX + 0.5f, 13.0f, Layout::SplitX + 0.5f, 447.0f, Theme::LineSoft);

    const wchar_t* title = page == 1 ? L"Visuals" : L"Cloud";
    const wchar_t* description = page == 1
        ? L"Visual configuration page"
        : L"Cloud services page";

    DrawText(title, Layout::LeftX, 22.0f, Layout::LeftRight, 48.0f,
        Theme::Text, text_);
    DrawText(description, Layout::RightX, 22.0f, Layout::RightRight, 48.0f,
        Theme::SubText, small_, DWRITE_TEXT_ALIGNMENT_TRAILING);

    Divider(Layout::LeftX, Layout::LeftRight, 60.0f);
    Divider(Layout::RightX, Layout::RightRight, 60.0f);

    FillRound(Layout::LeftX, 87.0f, Layout::RightRight, 205.0f,
        6.0f, WithAlpha(Theme::Panel, 0.84f));
    DrawRound(Layout::LeftX, 87.0f, Layout::RightRight, 205.0f,
        6.0f, Theme::Line, 1.0f);
    DrawIcon(Glyph::Info, Layout::LeftX + 20.0f, 112.0f,
        Layout::LeftX + 68.0f, 160.0f, Theme::Muted);
    DrawText(L"This page is ready for your own controls.",
        Layout::LeftX + 82.0f, 104.0f, Layout::RightRight - 20.0f, 137.0f,
        Theme::Text, small_);
    DrawText(L"The transparent black style and connected particle background remain active.",
        Layout::LeftX + 82.0f, 139.0f, Layout::RightRight - 20.0f, 181.0f,
        Theme::SubText, tiny_);
}

void App::DrawShell(float width, float height)
{
    FillRect(0.0f, 0.0f, width, height, Theme::Window);
    DrawConnectedParticles(width, height);

    // Extremely subtle inner edge, as in the dark reference.
    DrawRound(0.5f, 0.5f, width - 0.5f, height - 0.5f, 7.0f, WithAlpha(Theme::Line, 0.80f));

    DrawSidebar(height);

    if (selectedPage_ == 0)
        DrawMainPage();
    else
        DrawOtherPage(selectedPage_);

    // Popups are intentionally rendered last so they are never hidden.
    DrawComboPopup();
    DrawColorPicker();

    if (!colorPickerOpen_ && clicked_ && !clickConsumed_)
    {
        openCombo_ = 0;
        configNameFocused_ = false;
    }
}

void App::Render()
{
    if (FAILED(CreateDeviceResources()))
        return;

    RECT client{};
    GetClientRect(hwnd_, &client);
    const float width = static_cast<float>(client.right - client.left);
    const float height = static_cast<float>(client.bottom - client.top);

    comboPopup_ = ComboPopup{};
    clickConsumed_ = false;

    if (statusTimer_ > 0.0f)
    {
        statusTimer_ -= 0.016f;
        if (statusTimer_ <= 0.0f)
        {
            statusTimer_ = 0.0f;
            statusText_.clear();
        }
    }

    renderTarget_->BeginDraw();
    renderTarget_->SetTransform(D2D1::Matrix3x2F::Identity());
    renderTarget_->Clear(Theme::Window);

    DrawShell(width, height);

    const HRESULT result = renderTarget_->EndDraw();
    if (result == D2DERR_RECREATE_TARGET)
        DiscardDeviceResources();

    clicked_ = false;
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
        const LRESULT defaultResult = DefWindowProcW(hwnd_, message, wParam, lParam);
        if (defaultResult != HTCLIENT)
            return defaultResult;

        POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hwnd_, &point);
        if (point.y >= 0 && point.y < static_cast<LONG>(Layout::DragStrip))
            return HTCAPTION;
        return HTCLIENT;
    }

    case WM_MOUSEMOVE:
    {
        mouse_.x = GET_X_LPARAM(lParam);
        mouse_.y = GET_Y_LPARAM(lParam);

        TRACKMOUSEEVENT tracking{};
        tracking.cbSize = sizeof(tracking);
        tracking.dwFlags = TME_LEAVE;
        tracking.hwndTrack = hwnd_;
        TrackMouseEvent(&tracking);

        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    }

    case WM_MOUSELEAVE:
        mouse_ = { -1000, -1000 };
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDOWN:
        SetCapture(hwnd_);
        mouseDown_ = true;
        clicked_ = true;
        mouse_.x = GET_X_LPARAM(lParam);
        mouse_.y = GET_Y_LPARAM(lParam);
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;

    case WM_LBUTTONUP:
        mouseDown_ = false;
        activeSlider_ = ActiveSlider::None;
        activeColorArea_ = -1;
        configScrollbarDragging_ = false;
        ReleaseCapture();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;

    case WM_MOUSEWHEEL:
    {
        if (selectedPage_ == 3 && !colorPickerOpen_ && configs_.size() > 7)
        {
            POINT point{
                GET_X_LPARAM(lParam),
                GET_Y_LPARAM(lParam)
            };
            ScreenToClient(hwnd_, &point);
            mouse_ = point;

            const bool overConfigList = Hit(
                static_cast<float>(point.x),
                static_cast<float>(point.y),
                Layout::RightX,
                102.0f,
                Layout::RightRight,
                382.0f);

            if (overConfigList)
            {
                const int maximumScroll = std::max(
                    0,
                    static_cast<int>(configs_.size()) - 7);

                const int wheelSteps =
                    GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;

                configScrollIndex_ = std::clamp(
                    configScrollIndex_ - wheelSteps,
                    0,
                    maximumScroll);

                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }
        }

        break;
    }

    case WM_CHAR:
        if (configNameFocused_ && !colorPickerOpen_)
        {
            const wchar_t character = static_cast<wchar_t>(wParam);

            if (character == L'\b')
            {
                if (!configName_.empty())
                    configName_.pop_back();
            }
            else if (character == L'\r' || character == L'\n')
            {
                configNameFocused_ = false;
            }
            else if (character >= 32 && configName_.size() < 32 &&
                (std::iswalnum(character) || character == L' ' ||
                    character == L'-' || character == L'_'))
            {
                configName_.push_back(character);
            }

            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_RETURN && configNameFocused_)
        {
            configNameFocused_ = false;
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (wParam == VK_ESCAPE)
        {
            if (colorPickerOpen_ && pickerTarget_)
            {
                *pickerTarget_ = pickerOriginal_;
                colorPickerOpen_ = false;
                pickerTarget_ = nullptr;
                activeColorArea_ = -1;
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
            else if (openCombo_ != 0)
            {
                openCombo_ = 0;
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
            else if (configNameFocused_)
            {
                configNameFocused_ = false;
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
            else
            {
                PostMessageW(hwnd_, WM_CLOSE, 0, 0);
            }
        }
        return 0;

    case WM_SYSCOMMAND:
        // Keep normal Windows taskbar minimize/restore behavior for this
        // otherwise borderless popup window.
        if ((wParam & 0xFFF0) == SC_RESTORE)
        {
            ShowWindow(hwnd_, SW_RESTORE);
            SetForegroundWindow(hwnd_);
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }
        break;

    case WM_SIZE:
        if (renderTarget_)
            renderTarget_->Resize(D2D1::SizeU(LOWORD(lParam), HIWORD(lParam)));
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT paint{};
        BeginPaint(hwnd_, &paint);
        Render();
        EndPaint(hwnd_, &paint);
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
        MessageBoxW(nullptr, L"Failed to initialize Direct2D.", L"Error", MB_OK | MB_ICONERROR);
        app.Shutdown();
        return 1;
    }

    const int result = app.Run();
    app.Shutdown();
    return result;
}
