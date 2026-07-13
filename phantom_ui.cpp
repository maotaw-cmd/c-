#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <windowsx.h>

// Windows defines RGB as a preprocessor macro. This UI uses its own
// hexadecimal color helper, so remove the macro to avoid parser errors.
#ifdef RGB
#undef RGB
#endif

#include <d2d1.h>
#include <dwrite.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// ============================================================================
// Screenshot-style Direct2D menu UI.
//
// Build:
//   - Windows x64
//   - C++17 or newer
//   - Unicode
//   - Windows subsystem
//
// The controls are UI-only. They do not connect to or modify any game.
// ============================================================================

template <typename T>
void SafeRelease(T*& object)
{
    if (object)
    {
        object->Release();
        object = nullptr;
    }
}

float Clamp(float value, float minimum, float maximum)
{
    return std::max(minimum, std::min(value, maximum));
}

bool PointInside(float x, float y, float left, float top, float right, float bottom)
{
    return x >= left && x <= right && y >= top && y <= bottom;
}

D2D1_COLOR_F ColorFromHex(std::uint32_t rgb, float alpha = 1.0f)
{
    return D2D1::ColorF(
        static_cast<float>((rgb >> 16) & 0xFF) / 255.0f,
        static_cast<float>((rgb >> 8) & 0xFF) / 255.0f,
        static_cast<float>(rgb & 0xFF) / 255.0f,
        alpha);
}

namespace Layout
{
    constexpr int WindowWidth = 1020;
    constexpr int WindowHeight = 640;

    constexpr float SidebarWidth = 248.0f;
    constexpr float TopLineHeight = 2.0f;
    constexpr float ContentLeft = 276.0f;
    constexpr float ContentRight = 982.0f;
}

namespace Theme
{
    const D2D1_COLOR_F Background = ColorFromHex(0x07090A);
    const D2D1_COLOR_F ContentPanel = ColorFromHex(0x080A0B);
    const D2D1_COLOR_F SearchBackground = ColorFromHex(0x0B0D0E);
    const D2D1_COLOR_F InputBackground = ColorFromHex(0x0D0F10);
    const D2D1_COLOR_F InputHover = ColorFromHex(0x111415);
    const D2D1_COLOR_F SidebarBase = ColorFromHex(0x160A09);
    const D2D1_COLOR_F SidebarShade = ColorFromHex(0x0E0909, 0.95f);
    const D2D1_COLOR_F Divider = ColorFromHex(0x232728);
    const D2D1_COLOR_F SoftDivider = ColorFromHex(0x151819);
    const D2D1_COLOR_F Border = ColorFromHex(0x263132);
    const D2D1_COLOR_F TopAccent = ColorFromHex(0x4D7D73);

    const D2D1_COLOR_F Accent = ColorFromHex(0xD82937);
    const D2D1_COLOR_F AccentBright = ColorFromHex(0xEF3345);
    const D2D1_COLOR_F AccentDark = ColorFromHex(0x531016);
    const D2D1_COLOR_F AccentTrack = ColorFromHex(0x4C1318);

    const D2D1_COLOR_F White = ColorFromHex(0xF2F2F2);
    const D2D1_COLOR_F Text = ColorFromHex(0xC8CBCD);
    const D2D1_COLOR_F SubText = ColorFromHex(0x8A8E90);
    const D2D1_COLOR_F Muted = ColorFromHex(0x53585A);
    const D2D1_COLOR_F VeryMuted = ColorFromHex(0x343839);
    const D2D1_COLOR_F Off = ColorFromHex(0x252A2B);
    const D2D1_COLOR_F Hover = ColorFromHex(0x141718);
}

namespace Glyphs
{
    constexpr const wchar_t* Aim = L"\xF272";
    constexpr const wchar_t* Eye = L"\xE890";
    constexpr const wchar_t* More = L"\xE712";
    constexpr const wchar_t* Palette = L"\xE790";
    constexpr const wchar_t* Code = L"\xE943";
    constexpr const wchar_t* Settings = L"\xE713";
    constexpr const wchar_t* Link = L"\xE71B";
    constexpr const wchar_t* ChevronDown = L"\xE70D";
    constexpr const wchar_t* ChevronRight = L"\xE76C";
    constexpr const wchar_t* Search = L"\xE721";
    constexpr const wchar_t* Circle = L"\xEA3A";
    constexpr const wchar_t* User = L"\xE77B";
}

enum class Page
{
    Ragebot,
    AntiAim,
    Legitbot,
    Visuals,
    Misc,
    Skins,
    Scripts,
    Settings,
    Hotkeys
};

struct UiState
{
    Page page = Page::Ragebot;

    bool enableRagebot = false;
    bool autoScope = true;
    bool autoFire = true;
    bool backtrack = true;
    bool delayShot = false;

    bool multipoint = true;
    bool avoidLimbs = false;
    bool safePoints = false;

    float headScale = 82.0f;
    float bodyScale = 63.0f;
    float minimumDamage = 21.0f;

    int settingFor = 0;
    int hitboxes = 0;
    int priority = 0;
};

class Application
{
public:
    bool Initialize(HINSTANCE instance);
    int Run();
    void Shutdown();

private:
    HWND window_ = nullptr;

    ID2D1Factory* d2dFactory_ = nullptr;
    ID2D1HwndRenderTarget* renderTarget_ = nullptr;
    ID2D1SolidColorBrush* brush_ = nullptr;
    IDWriteFactory* writeFactory_ = nullptr;

    IDWriteTextFormat* fontBody_ = nullptr;
    IDWriteTextFormat* fontSmall_ = nullptr;
    IDWriteTextFormat* fontTiny_ = nullptr;
    IDWriteTextFormat* fontMedium_ = nullptr;
    IDWriteTextFormat* fontTitle_ = nullptr;
    IDWriteTextFormat* fontLogo_ = nullptr;
    IDWriteTextFormat* fontIcon_ = nullptr;

    POINT mouse_{ -10000, -10000 };
    bool mouseDown_ = false;
    bool mousePressed_ = false;
    std::wstring activeSlider_;
    std::wstring openCombo_;

    UiState state_;

private:
    static LRESULT CALLBACK StaticWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources();
    void DiscardDeviceResources();

    HRESULT CreateFont(
        const wchar_t* family,
        float size,
        DWRITE_FONT_WEIGHT weight,
        IDWriteTextFormat** output);

    void Render();

    void SetBrush(D2D1_COLOR_F color);
    void FillRect(float left, float top, float right, float bottom, D2D1_COLOR_F color);
    void DrawRect(float left, float top, float right, float bottom, D2D1_COLOR_F color, float width = 1.0f);
    void FillRoundedRect(float left, float top, float right, float bottom, float radius, D2D1_COLOR_F color);
    void DrawRoundedRect(
        float left,
        float top,
        float right,
        float bottom,
        float radius,
        D2D1_COLOR_F color,
        float width = 1.0f);
    void DrawLine(float x1, float y1, float x2, float y2, D2D1_COLOR_F color, float width = 1.0f);
    void FillEllipse(float centerX, float centerY, float radiusX, float radiusY, D2D1_COLOR_F color);
    void DrawEllipse(
        float centerX,
        float centerY,
        float radiusX,
        float radiusY,
        D2D1_COLOR_F color,
        float width = 1.0f);
    void DrawLabelText(
        const std::wstring& text,
        float x,
        float y,
        float width,
        float height,
        D2D1_COLOR_F color,
        IDWriteTextFormat* format = nullptr,
        DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING);
    void DrawIcon(
        const wchar_t* icon,
        float x,
        float y,
        float width,
        float height,
        D2D1_COLOR_F color,
        float fontScale = 1.0f);
    void FillPolygon(const std::vector<D2D1_POINT_2F>& points, D2D1_COLOR_F color);
    void DrawRadialGlow(
        float centerX,
        float centerY,
        float radiusX,
        float radiusY,
        D2D1_COLOR_F innerColor,
        D2D1_COLOR_F outerColor);

    bool Hover(float left, float top, float right, float bottom) const;

    void DrawBackground(float width, float height);
    void DrawSidebar(float height);
    void DrawLogo(float x, float y);
    void DrawNavigation(float height);
    void DrawBreadcrumbAndSearch();
    void DrawRagebotPage();
    void DrawPlaceholderPage(const std::wstring& title, const std::wstring& description);

    void DrawNavRow(
        Page page,
        const wchar_t* icon,
        const std::wstring& text,
        float y,
        bool hasArrow = false);
    void DrawSubNavRow(Page page, const std::wstring& text, float y);

    bool Toggle(
        const std::wstring& id,
        const std::wstring& label,
        bool& value,
        float x,
        float y,
        float width);
    bool Combo(
        const std::wstring& id,
        const std::wstring& label,
        const std::vector<std::wstring>& options,
        int& selected,
        float x,
        float y,
        float width);
    void DrawComboPopup(
        const std::wstring& id,
        const std::vector<std::wstring>& options,
        int& selected,
        float x,
        float y,
        float width);
    bool Slider(
        const std::wstring& id,
        const std::wstring& label,
        float& value,
        float minimum,
        float maximum,
        float x,
        float y,
        float width,
        int decimals = 1);
    bool DotOption(const std::wstring& label, bool& value, float x, float y, float width);

    void HandleSidebarClick(float x, float y);
};

HRESULT Application::CreateFont(
    const wchar_t* family,
    float size,
    DWRITE_FONT_WEIGHT weight,
    IDWriteTextFormat** output)
{
    HRESULT result = writeFactory_->CreateTextFormat(
        family,
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
        (*output)->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    return result;
}

HRESULT Application::CreateDeviceIndependentResources()
{
    HRESULT result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory_);
    if (FAILED(result))
    {
        return result;
    }

    result = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&writeFactory_));
    if (FAILED(result))
    {
        return result;
    }

    if (FAILED(result = CreateFont(L"Segoe UI", 13.0f, DWRITE_FONT_WEIGHT_NORMAL, &fontBody_)))
        return result;
    if (FAILED(result = CreateFont(L"Segoe UI", 11.5f, DWRITE_FONT_WEIGHT_NORMAL, &fontSmall_)))
        return result;
    if (FAILED(result = CreateFont(L"Segoe UI", 9.5f, DWRITE_FONT_WEIGHT_NORMAL, &fontTiny_)))
        return result;
    if (FAILED(result = CreateFont(L"Segoe UI", 13.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, &fontMedium_)))
        return result;
    if (FAILED(result = CreateFont(L"Segoe UI", 17.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, &fontTitle_)))
        return result;
    if (FAILED(result = CreateFont(L"Segoe UI", 15.5f, DWRITE_FONT_WEIGHT_BOLD, &fontLogo_)))
        return result;

    return CreateFont(L"Segoe MDL2 Assets", 15.0f, DWRITE_FONT_WEIGHT_NORMAL, &fontIcon_);
}

HRESULT Application::CreateDeviceResources()
{
    if (renderTarget_)
    {
        return S_OK;
    }

    RECT client{};
    GetClientRect(window_, &client);

    HRESULT result = d2dFactory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_HARDWARE,
            D2D1::PixelFormat(
                DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_IGNORE)),
        D2D1::HwndRenderTargetProperties(
            window_,
            D2D1::SizeU(client.right, client.bottom),
            D2D1_PRESENT_OPTIONS_IMMEDIATELY),
        &renderTarget_);

    if (FAILED(result))
    {
        return result;
    }

    return renderTarget_->CreateSolidColorBrush(Theme::White, &brush_);
}

void Application::DiscardDeviceResources()
{
    SafeRelease(brush_);
    SafeRelease(renderTarget_);
}

bool Application::Initialize(HINSTANCE instance)
{
    SetProcessDPIAware();

    if (FAILED(CreateDeviceIndependentResources()))
    {
        return false;
    }

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = StaticWindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = L"ScreenshotRagebotMenu";

    if (!RegisterClassExW(&windowClass))
    {
        return false;
    }

    const int x = (GetSystemMetrics(SM_CXSCREEN) - Layout::WindowWidth) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - Layout::WindowHeight) / 2;

    window_ = CreateWindowExW(
        WS_EX_APPWINDOW,
        windowClass.lpszClassName,
        L"Ragebot UI",
        WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX,
        x,
        y,
        Layout::WindowWidth,
        Layout::WindowHeight,
        nullptr,
        nullptr,
        instance,
        this);

    if (!window_)
    {
        return false;
    }

    ShowWindow(window_, SW_SHOW);
    UpdateWindow(window_);
    SetTimer(window_, 1, 16, nullptr);
    return true;
}

int Application::Run()
{
    MSG message{};

    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}

void Application::Shutdown()
{
    if (window_)
    {
        KillTimer(window_, 1);
    }

    DiscardDeviceResources();

    SafeRelease(fontIcon_);
    SafeRelease(fontLogo_);
    SafeRelease(fontTitle_);
    SafeRelease(fontMedium_);
    SafeRelease(fontTiny_);
    SafeRelease(fontSmall_);
    SafeRelease(fontBody_);

    SafeRelease(writeFactory_);
    SafeRelease(d2dFactory_);
}

void Application::SetBrush(D2D1_COLOR_F color)
{
    brush_->SetColor(color);
}

void Application::FillRect(float left, float top, float right, float bottom, D2D1_COLOR_F color)
{
    SetBrush(color);
    renderTarget_->FillRectangle(D2D1::RectF(left, top, right, bottom), brush_);
}

void Application::DrawRect(
    float left,
    float top,
    float right,
    float bottom,
    D2D1_COLOR_F color,
    float width)
{
    SetBrush(color);
    renderTarget_->DrawRectangle(D2D1::RectF(left, top, right, bottom), brush_, width);
}

void Application::FillRoundedRect(
    float left,
    float top,
    float right,
    float bottom,
    float radius,
    D2D1_COLOR_F color)
{
    SetBrush(color);
    renderTarget_->FillRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), radius, radius),
        brush_);
}

void Application::DrawRoundedRect(
    float left,
    float top,
    float right,
    float bottom,
    float radius,
    D2D1_COLOR_F color,
    float width)
{
    SetBrush(color);
    renderTarget_->DrawRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), radius, radius),
        brush_,
        width);
}

void Application::DrawLine(
    float x1,
    float y1,
    float x2,
    float y2,
    D2D1_COLOR_F color,
    float width)
{
    SetBrush(color);
    renderTarget_->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush_, width);
}

void Application::FillEllipse(
    float centerX,
    float centerY,
    float radiusX,
    float radiusY,
    D2D1_COLOR_F color)
{
    SetBrush(color);
    renderTarget_->FillEllipse(
        D2D1::Ellipse(D2D1::Point2F(centerX, centerY), radiusX, radiusY),
        brush_);
}

void Application::DrawEllipse(
    float centerX,
    float centerY,
    float radiusX,
    float radiusY,
    D2D1_COLOR_F color,
    float width)
{
    SetBrush(color);
    renderTarget_->DrawEllipse(
        D2D1::Ellipse(D2D1::Point2F(centerX, centerY), radiusX, radiusY),
        brush_,
        width);
}

void Application::DrawLabelText(
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
    {
        format = fontBody_;
    }

    format->SetTextAlignment(alignment);
    SetBrush(color);

    renderTarget_->DrawTextW(
        text.c_str(),
        static_cast<UINT32>(text.size()),
        format,
        D2D1::RectF(x, y, x + width, y + height),
        brush_,
        D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

void Application::DrawIcon(
    const wchar_t* icon,
    float x,
    float y,
    float width,
    float height,
    D2D1_COLOR_F color,
    float)
{
    DrawLabelText(icon, x, y, width, height, color, fontIcon_, DWRITE_TEXT_ALIGNMENT_CENTER);
}

void Application::FillPolygon(
    const std::vector<D2D1_POINT_2F>& points,
    D2D1_COLOR_F color)
{
    if (points.size() < 3)
    {
        return;
    }

    ID2D1PathGeometry* geometry = nullptr;
    ID2D1GeometrySink* sink = nullptr;

    if (FAILED(d2dFactory_->CreatePathGeometry(&geometry)))
    {
        return;
    }

    if (SUCCEEDED(geometry->Open(&sink)))
    {
        sink->BeginFigure(points.front(), D2D1_FIGURE_BEGIN_FILLED);
        sink->AddLines(points.data() + 1, static_cast<UINT32>(points.size() - 1));
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        sink->Close();

        SetBrush(color);
        renderTarget_->FillGeometry(geometry, brush_);
    }

    SafeRelease(sink);
    SafeRelease(geometry);
}

void Application::DrawRadialGlow(
    float centerX,
    float centerY,
    float radiusX,
    float radiusY,
    D2D1_COLOR_F innerColor,
    D2D1_COLOR_F outerColor)
{
    D2D1_GRADIENT_STOP stops[2] = {
        { 0.0f, innerColor },
        { 1.0f, outerColor }
    };

    ID2D1GradientStopCollection* collection = nullptr;
    ID2D1RadialGradientBrush* radialBrush = nullptr;

    if (SUCCEEDED(renderTarget_->CreateGradientStopCollection(stops, 2, &collection)))
    {
        renderTarget_->CreateRadialGradientBrush(
            D2D1::RadialGradientBrushProperties(
                D2D1::Point2F(centerX, centerY),
                D2D1::Point2F(0.0f, 0.0f),
                radiusX,
                radiusY),
            collection,
            &radialBrush);
    }

    if (radialBrush)
    {
        renderTarget_->FillEllipse(
            D2D1::Ellipse(D2D1::Point2F(centerX, centerY), radiusX, radiusY),
            radialBrush);
    }

    SafeRelease(radialBrush);
    SafeRelease(collection);
}

bool Application::Hover(float left, float top, float right, float bottom) const
{
    return PointInside(
        static_cast<float>(mouse_.x),
        static_cast<float>(mouse_.y),
        left,
        top,
        right,
        bottom);
}

void Application::DrawLogo(float x, float y)
{
    // Three interlocking angular marks, matching the white emblem in the reference.
    const D2D1_COLOR_F logoColor = ColorFromHex(0xE7E0D7);

    FillPolygon({
        D2D1::Point2F(x + 0.0f, y + 25.0f),
        D2D1::Point2F(x + 15.0f, y + 8.0f),
        D2D1::Point2F(x + 25.0f, y + 15.0f),
        D2D1::Point2F(x + 10.0f, y + 32.0f)
        }, logoColor);

    FillPolygon({
        D2D1::Point2F(x + 12.0f, y + 36.0f),
        D2D1::Point2F(x + 35.0f, y + 8.0f),
        D2D1::Point2F(x + 45.0f, y + 15.0f),
        D2D1::Point2F(x + 23.0f, y + 43.0f)
        }, logoColor);

    FillPolygon({
        D2D1::Point2F(x + 26.0f, y + 46.0f),
        D2D1::Point2F(x + 55.0f, y + 13.0f),
        D2D1::Point2F(x + 65.0f, y + 20.0f),
        D2D1::Point2F(x + 38.0f, y + 53.0f)
        }, logoColor);

    FillPolygon({
        D2D1::Point2F(x + 9.0f, y + 31.0f),
        D2D1::Point2F(x + 36.0f, y + 53.0f),
        D2D1::Point2F(x + 29.0f, y + 63.0f),
        D2D1::Point2F(x + 0.0f, y + 39.0f)
        }, logoColor);
}

void Application::DrawBackground(float width, float height)
{
    renderTarget_->Clear(Theme::Background);

    FillRect(0.0f, 0.0f, Layout::SidebarWidth, height, Theme::SidebarBase);
    FillRect(Layout::SidebarWidth, 0.0f, width, height, Theme::ContentPanel);

    // Red smoky sidebar lighting.
    DrawRadialGlow(
        48.0f,
        68.0f,
        188.0f,
        145.0f,
        ColorFromHex(0x8C191A, 0.52f),
        ColorFromHex(0x150909, 0.0f));

    DrawRadialGlow(
        196.0f,
        548.0f,
        150.0f,
        184.0f,
        ColorFromHex(0x8A1519, 0.40f),
        ColorFromHex(0x150909, 0.0f));

    DrawRadialGlow(
        91.0f,
        332.0f,
        117.0f,
        150.0f,
        ColorFromHex(0x4B1114, 0.30f),
        ColorFromHex(0x150909, 0.0f));

    // Abstract dark-red shapes behind the navigation.
    FillPolygon({
        D2D1::Point2F(0.0f, 165.0f),
        D2D1::Point2F(77.0f, 97.0f),
        D2D1::Point2F(115.0f, 121.0f),
        D2D1::Point2F(36.0f, 219.0f)
        }, ColorFromHex(0x3A1011, 0.35f));

    FillPolygon({
        D2D1::Point2F(131.0f, 252.0f),
        D2D1::Point2F(247.0f, 177.0f),
        D2D1::Point2F(247.0f, 299.0f),
        D2D1::Point2F(168.0f, 343.0f)
        }, ColorFromHex(0x5B1216, 0.22f));

    FillPolygon({
        D2D1::Point2F(0.0f, 506.0f),
        D2D1::Point2F(95.0f, 430.0f),
        D2D1::Point2F(143.0f, 493.0f),
        D2D1::Point2F(52.0f, 620.0f)
        }, ColorFromHex(0x2D0D10, 0.45f));

    // Dark overlay makes the red artwork subtle, like the reference image.
    FillRect(0.0f, 0.0f, Layout::SidebarWidth, height, Theme::SidebarShade);

    // Restore small red glows above the overlay.
    DrawRadialGlow(
        33.0f,
        55.0f,
        116.0f,
        102.0f,
        ColorFromHex(0xB32124, 0.27f),
        ColorFromHex(0x190909, 0.0f));

    DrawRadialGlow(
        213.0f,
        588.0f,
        86.0f,
        110.0f,
        ColorFromHex(0xA11B21, 0.23f),
        ColorFromHex(0x190909, 0.0f));

    FillRect(0.0f, 0.0f, width, Layout::TopLineHeight, Theme::TopAccent);
    DrawLine(Layout::SidebarWidth, 0.0f, Layout::SidebarWidth, height, ColorFromHex(0x292D2E), 1.0f);
}

void Application::DrawNavRow(
    Page page,
    const wchar_t* icon,
    const std::wstring& text,
    float y,
    bool hasArrow)
{
    const bool selected = state_.page == page;
    const bool hovered = Hover(24.0f, y, 224.0f, y + 34.0f);

    if (hovered && !selected)
    {
        FillRoundedRect(23.0f, y, 225.0f, y + 34.0f, 2.0f, ColorFromHex(0xFFFFFF, 0.025f));
    }

    DrawIcon(
        icon,
        28.0f,
        y,
        34.0f,
        34.0f,
        selected ? Theme::White : hovered ? Theme::Text : Theme::SubText);

    DrawLabelText(
        text,
        67.0f,
        y,
        128.0f,
        34.0f,
        selected ? Theme::White : hovered ? Theme::Text : Theme::SubText,
        fontSmall_);

    if (hasArrow)
    {
        DrawIcon(
            Glyphs::ChevronDown,
            199.0f,
            y,
            20.0f,
            34.0f,
            selected ? Theme::White : Theme::Muted);
    }
}

void Application::DrawSubNavRow(Page page, const std::wstring& text, float y)
{
    const bool selected = state_.page == page;
    const bool hovered = Hover(55.0f, y, 219.0f, y + 31.0f);

    if (selected)
    {
        FillRoundedRect(58.0f, y + 2.0f, 216.0f, y + 29.0f, 2.0f, ColorFromHex(0xFFFFFF, 0.018f));
    }
    else if (hovered)
    {
        FillRoundedRect(58.0f, y + 2.0f, 216.0f, y + 29.0f, 2.0f, ColorFromHex(0xFFFFFF, 0.012f));
    }

    DrawLabelText(
        text,
        68.0f,
        y,
        130.0f,
        31.0f,
        selected ? Theme::White : hovered ? Theme::Text : Theme::Muted,
        selected ? fontMedium_ : fontSmall_);
}

void Application::DrawNavigation(float height)
{
    DrawNavRow(Page::Ragebot, Glyphs::Aim, L"AimBot", 128.0f, true);
    DrawSubNavRow(Page::Ragebot, L"Ragebot", 164.0f);
    DrawSubNavRow(Page::AntiAim, L"Anti aim", 196.0f);
    DrawSubNavRow(Page::Legitbot, L"Legitbot", 228.0f);

    DrawNavRow(Page::Visuals, Glyphs::Eye, L"Visuals", 274.0f, true);
    DrawNavRow(Page::Misc, Glyphs::More, L"Misc", 314.0f, false);
    DrawNavRow(Page::Skins, Glyphs::Palette, L"Skins", 354.0f, false);
    DrawNavRow(Page::Scripts, Glyphs::Code, L"Scripts", 394.0f, false);
    DrawNavRow(Page::Settings, Glyphs::Settings, L"Settings", 434.0f, false);
    DrawNavRow(Page::Hotkeys, Glyphs::Link, L"Hotkeys", 474.0f, false);

    DrawLine(31.0f, height - 96.0f, 217.0f, height - 96.0f, ColorFromHex(0x8B3A36, 0.40f), 1.0f);

    // Profile block.
    DrawEllipse(52.0f, height - 51.0f, 24.0f, 24.0f, ColorFromHex(0xBCA899, 0.48f), 2.0f);
    FillEllipse(52.0f, height - 55.0f, 7.5f, 7.5f, ColorFromHex(0xBCA899, 0.50f));
    FillRoundedRect(39.0f, height - 47.0f, 65.0f, height - 33.0f, 7.0f, ColorFromHex(0xBCA899, 0.42f));

    DrawLabelText(
        L"Emanuelstarter",
        82.0f,
        height - 70.0f,
        130.0f,
        26.0f,
        Theme::White,
        fontMedium_);

    DrawLabelText(
        L"premium account",
        82.0f,
        height - 48.0f,
        130.0f,
        20.0f,
        ColorFromHex(0xA77D75),
        fontTiny_);
}

void Application::DrawSidebar(float height)
{
    DrawLogo(91.0f, 18.0f);
    DrawLine(23.0f, 95.0f, 219.0f, 95.0f, ColorFromHex(0x6E3431, 0.42f), 1.0f);
    DrawNavigation(height);
}

void Application::DrawBreadcrumbAndSearch()
{
    DrawIcon(Glyphs::Aim, 279.0f, 14.0f, 26.0f, 30.0f, Theme::SubText);
    DrawLabelText(L"AimBot", 311.0f, 14.0f, 58.0f, 30.0f, Theme::SubText, fontSmall_);
    DrawIcon(Glyphs::ChevronRight, 368.0f, 14.0f, 18.0f, 30.0f, Theme::Muted);
    DrawLabelText(L"Ragebot", 392.0f, 14.0f, 76.0f, 30.0f, Theme::SubText, fontSmall_);

    FillRoundedRect(
        Layout::ContentLeft,
        54.0f,
        Layout::ContentRight,
        94.0f,
        2.0f,
        Theme::SearchBackground);

    DrawIcon(Glyphs::Search, 291.0f, 54.0f, 30.0f, 40.0f, Theme::VeryMuted);
}

bool Application::Toggle(
    const std::wstring& id,
    const std::wstring& label,
    bool& value,
    float x,
    float y,
    float width)
{
    const bool hovered = Hover(x, y, x + width, y + 30.0f);

    if (hovered && mousePressed_)
    {
        value = !value;
    }

    const float switchLeft = x;
    const float switchTop = y + 7.0f;
    const float switchWidth = 37.0f;
    const float switchHeight = 18.0f;

    FillRoundedRect(
        switchLeft,
        switchTop,
        switchLeft + switchWidth,
        switchTop + switchHeight,
        9.0f,
        value ? Theme::AccentDark : Theme::Off);

    if (value)
    {
        DrawRadialGlow(
            switchLeft + 24.0f,
            switchTop + 9.0f,
            24.0f,
            18.0f,
            ColorFromHex(0xE52B3B, 0.32f),
            ColorFromHex(0xE52B3B, 0.0f));

        FillRoundedRect(
            switchLeft,
            switchTop,
            switchLeft + switchWidth,
            switchTop + switchHeight,
            9.0f,
            Theme::AccentDark);
    }

    const float knobX = value ? switchLeft + 25.0f : switchLeft + 5.0f;
    FillEllipse(
        knobX,
        switchTop + 9.0f,
        6.0f,
        6.0f,
        value ? Theme::White : Theme::Muted);

    DrawLabelText(
        label,
        x + 54.0f,
        y,
        width - 54.0f,
        30.0f,
        hovered ? Theme::White : Theme::Text,
        fontSmall_);

    return hovered;
}

bool Application::Combo(
    const std::wstring& id,
    const std::wstring& label,
    const std::vector<std::wstring>& options,
    int& selected,
    float x,
    float y,
    float width)
{
    selected = std::clamp(selected, 0, static_cast<int>(options.size()) - 1);

    const bool hovered = Hover(x, y, x + width, y + 37.0f);
    const bool opened = openCombo_ == id;

    if (hovered && mousePressed_)
    {
        openCombo_ = opened ? L"" : id;
    }

    FillRoundedRect(
        x,
        y,
        x + width,
        y + 37.0f,
        2.0f,
        hovered || opened ? Theme::InputHover : Theme::InputBackground);

    DrawLabelText(
        label + L" " + options[selected],
        x + 13.0f,
        y,
        width - 48.0f,
        37.0f,
        Theme::Text,
        fontSmall_);

    DrawIcon(
        Glyphs::ChevronDown,
        x + width - 34.0f,
        y,
        24.0f,
        37.0f,
        opened ? Theme::Text : Theme::VeryMuted);

    return hovered;
}

void Application::DrawComboPopup(
    const std::wstring& id,
    const std::vector<std::wstring>& options,
    int& selected,
    float x,
    float y,
    float width)
{
    if (openCombo_ != id)
    {
        return;
    }

    const float itemHeight = 31.0f;
    const float top = y + 40.0f;
    const float bottom = top + itemHeight * static_cast<float>(options.size());

    FillRoundedRect(x, top, x + width, bottom, 2.0f, ColorFromHex(0x0D0F10));
    DrawRect(x, top, x + width, bottom, Theme::Divider, 1.0f);

    for (std::size_t index = 0; index < options.size(); ++index)
    {
        const float itemTop = top + itemHeight * static_cast<float>(index);
        const bool itemHovered = Hover(x, itemTop, x + width, itemTop + itemHeight);

        if (itemHovered)
        {
            FillRect(x + 1.0f, itemTop, x + width - 1.0f, itemTop + itemHeight, Theme::Hover);
        }

        DrawLabelText(
            options[index],
            x + 13.0f,
            itemTop,
            width - 26.0f,
            itemHeight,
            static_cast<int>(index) == selected ? Theme::White : Theme::SubText,
            fontSmall_);

        if (itemHovered && mousePressed_)
        {
            selected = static_cast<int>(index);
            openCombo_.clear();
        }
    }
}

bool Application::Slider(
    const std::wstring& id,
    const std::wstring& label,
    float& value,
    float minimum,
    float maximum,
    float x,
    float y,
    float width,
    int decimals)
{
    const float trackY = y + 28.0f;
    const bool hovered = Hover(x, trackY - 9.0f, x + width, trackY + 9.0f);

    if (hovered && mousePressed_)
    {
        activeSlider_ = id;
    }

    if (activeSlider_ == id && mouseDown_)
    {
        const float normalized = Clamp((static_cast<float>(mouse_.x) - x) / width, 0.0f, 1.0f);
        value = minimum + normalized * (maximum - minimum);
    }

    std::wstring valueText;
    if (decimals == 0)
    {
        valueText = std::to_wstring(static_cast<int>(std::round(value)));
    }
    else
    {
        const int whole = static_cast<int>(value / 100.0f);
        const int tenths = static_cast<int>(std::round(std::fmod(value, 100.0f) / 10.0f));
        valueText = std::to_wstring(whole) + L"." + std::to_wstring(std::min(tenths, 9));
    }

    DrawLabelText(label, x, y, width - 70.0f, 20.0f, Theme::Text, fontSmall_);
    DrawLabelText(
        valueText,
        x + width - 70.0f,
        y,
        70.0f,
        20.0f,
        Theme::VeryMuted,
        fontTiny_,
        DWRITE_TEXT_ALIGNMENT_TRAILING);

    FillRoundedRect(x, trackY - 2.0f, x + width, trackY + 2.0f, 2.0f, Theme::Off);

    const float normalized = (value - minimum) / (maximum - minimum);
    const float knobX = x + width * normalized;

    DrawRadialGlow(
        knobX,
        trackY,
        20.0f,
        15.0f,
        ColorFromHex(0xD82937, 0.25f),
        ColorFromHex(0xD82937, 0.0f));

    FillRoundedRect(x, trackY - 2.0f, knobX, trackY + 2.0f, 2.0f, Theme::AccentTrack);
    FillEllipse(knobX, trackY, 5.0f, 5.0f, Theme::White);

    return hovered;
}

bool Application::DotOption(
    const std::wstring& label,
    bool& value,
    float x,
    float y,
    float width)
{
    const bool hovered = Hover(x, y, x + width, y + 28.0f);

    if (hovered && mousePressed_)
    {
        value = !value;
    }

    FillEllipse(x + 7.0f, y + 14.0f, 6.0f, 6.0f, value ? Theme::AccentDark : Theme::Off);
    if (value)
    {
        FillEllipse(x + 7.0f, y + 14.0f, 2.5f, 2.5f, Theme::White);
    }

    DrawLabelText(
        label,
        x + 30.0f,
        y,
        width - 30.0f,
        28.0f,
        hovered ? Theme::White : Theme::SubText,
        fontSmall_);

    return hovered;
}

void Application::DrawRagebotPage()
{
    DrawLabelText(
        L"Global Settings",
        Layout::ContentLeft,
        112.0f,
        300.0f,
        32.0f,
        Theme::Text,
        fontTitle_);

    // Left column.
    const float leftX = 281.0f;
    float leftY = 150.0f;

    Toggle(L"enable_ragebot", L"Enable ragebot", state_.enableRagebot, leftX, leftY, 255.0f);
    leftY += 42.0f;
    Toggle(L"auto_scope", L"Auto scope", state_.autoScope, leftX, leftY, 255.0f);
    leftY += 42.0f;
    Toggle(L"auto_fire", L"Auto fire", state_.autoFire, leftX, leftY, 255.0f);
    leftY += 42.0f;
    Toggle(L"backtrack", L"Backtrack", state_.backtrack, leftX, leftY, 255.0f);
    leftY += 42.0f;
    Toggle(L"delay_shot", L"Delay shot", state_.delayShot, leftX, leftY, 255.0f);

    // Small central gear shown in the reference.
    DrawIcon(Glyphs::Settings, 578.0f, 264.0f, 34.0f, 34.0f, Theme::VeryMuted);

    // Right column.
    const float rightX = 650.0f;
    const float rightWidth = 302.0f;
    float rightY = 125.0f;

    Combo(
        L"setting_for",
        L"Setting for:",
        { L"General", L"Pistols", L"Rifles", L"Snipers" },
        state_.settingFor,
        rightX,
        rightY,
        rightWidth);

    rightY += 64.0f;

    Combo(
        L"hitboxes",
        L"Hitboxes:",
        { L"Head, Chest, Stomach", L"Head only", L"Chest only", L"Body only" },
        state_.hitboxes,
        rightX,
        rightY,
        rightWidth);

    rightY += 47.0f;
    Toggle(L"multipoint", L"Multipoint", state_.multipoint, rightX, rightY, rightWidth);

    rightY += 43.0f;
    Slider(
        L"head_scale",
        L"Multipoint scale head",
        state_.headScale,
        0.0f,
        100.0f,
        rightX,
        rightY,
        rightWidth,
        1);

    rightY += 58.0f;
    Slider(
        L"body_scale",
        L"Multipoint scale body",
        state_.bodyScale,
        0.0f,
        100.0f,
        rightX,
        rightY,
        rightWidth,
        1);

    rightY += 58.0f;

    Combo(
        L"priority",
        L"Target Priority:",
        { L"Damage", L"Distance", L"Health", L"FOV" },
        state_.priority,
        rightX,
        rightY,
        rightWidth);

    rightY += 50.0f;
    DotOption(L"Avoid limbs", state_.avoidLimbs, rightX, rightY, rightWidth);
    rightY += 42.0f;
    DotOption(L"Safe points", state_.safePoints, rightX, rightY, rightWidth);

    rightY += 48.0f;
    Slider(
        L"minimum_damage",
        L"Minimum damage",
        state_.minimumDamage,
        0.0f,
        100.0f,
        rightX,
        rightY,
        rightWidth,
        0);

    // Combo popups are drawn last so they always appear above every control.
    DrawComboPopup(
        L"setting_for",
        { L"General", L"Pistols", L"Rifles", L"Snipers" },
        state_.settingFor,
        rightX,
        125.0f,
        rightWidth);

    DrawComboPopup(
        L"hitboxes",
        { L"Head, Chest, Stomach", L"Head only", L"Chest only", L"Body only" },
        state_.hitboxes,
        rightX,
        189.0f,
        rightWidth);

    DrawComboPopup(
        L"priority",
        { L"Damage", L"Distance", L"Health", L"FOV" },
        state_.priority,
        rightX,
        395.0f,
        rightWidth);
}

void Application::DrawPlaceholderPage(
    const std::wstring& title,
    const std::wstring& description)
{
    DrawLabelText(title, Layout::ContentLeft, 112.0f, 320.0f, 32.0f, Theme::Text, fontTitle_);
    DrawLabelText(
        description,
        Layout::ContentLeft,
        154.0f,
        560.0f,
        30.0f,
        Theme::SubText,
        fontSmall_);
}

void Application::Render()
{
    if (FAILED(CreateDeviceResources()))
    {
        return;
    }

    RECT client{};
    GetClientRect(window_, &client);

    const float width = static_cast<float>(client.right);
    const float height = static_cast<float>(client.bottom);

    renderTarget_->BeginDraw();
    renderTarget_->SetTransform(D2D1::Matrix3x2F::Identity());

    DrawBackground(width, height);
    DrawSidebar(height);
    DrawBreadcrumbAndSearch();

    switch (state_.page)
    {
    case Page::Ragebot:
        DrawRagebotPage();
        break;
    case Page::AntiAim:
        DrawPlaceholderPage(L"Anti aim", L"This page keeps the same visual language and is ready for your controls.");
        break;
    case Page::Legitbot:
        DrawPlaceholderPage(L"Legitbot", L"This page keeps the same visual language and is ready for your controls.");
        break;
    case Page::Visuals:
        DrawPlaceholderPage(L"Visuals", L"This page keeps the same visual language and is ready for your controls.");
        break;
    case Page::Misc:
        DrawPlaceholderPage(L"Misc", L"This page keeps the same visual language and is ready for your controls.");
        break;
    case Page::Skins:
        DrawPlaceholderPage(L"Skins", L"This page keeps the same visual language and is ready for your controls.");
        break;
    case Page::Scripts:
        DrawPlaceholderPage(L"Scripts", L"This page keeps the same visual language and is ready for your controls.");
        break;
    case Page::Settings:
        DrawPlaceholderPage(L"Settings", L"This page keeps the same visual language and is ready for your controls.");
        break;
    case Page::Hotkeys:
        DrawPlaceholderPage(L"Hotkeys", L"This page keeps the same visual language and is ready for your controls.");
        break;
    }

    DrawRect(0.5f, 0.5f, width - 0.5f, height - 0.5f, Theme::Border, 1.0f);

    const HRESULT result = renderTarget_->EndDraw();
    if (result == D2DERR_RECREATE_TARGET)
    {
        DiscardDeviceResources();
    }

    mousePressed_ = false;
}

void Application::HandleSidebarClick(float x, float y)
{
    if (PointInside(x, y, 24.0f, 128.0f, 224.0f, 162.0f) ||
        PointInside(x, y, 55.0f, 164.0f, 219.0f, 195.0f))
    {
        state_.page = Page::Ragebot;
    }
    else if (PointInside(x, y, 55.0f, 196.0f, 219.0f, 227.0f))
    {
        state_.page = Page::AntiAim;
    }
    else if (PointInside(x, y, 55.0f, 228.0f, 219.0f, 259.0f))
    {
        state_.page = Page::Legitbot;
    }
    else if (PointInside(x, y, 24.0f, 274.0f, 224.0f, 308.0f))
    {
        state_.page = Page::Visuals;
    }
    else if (PointInside(x, y, 24.0f, 314.0f, 224.0f, 348.0f))
    {
        state_.page = Page::Misc;
    }
    else if (PointInside(x, y, 24.0f, 354.0f, 224.0f, 388.0f))
    {
        state_.page = Page::Skins;
    }
    else if (PointInside(x, y, 24.0f, 394.0f, 224.0f, 428.0f))
    {
        state_.page = Page::Scripts;
    }
    else if (PointInside(x, y, 24.0f, 434.0f, 224.0f, 468.0f))
    {
        state_.page = Page::Settings;
    }
    else if (PointInside(x, y, 24.0f, 474.0f, 224.0f, 508.0f))
    {
        state_.page = Page::Hotkeys;
    }

    activeSlider_.clear();
    openCombo_.clear();
}

LRESULT CALLBACK Application::StaticWindowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    Application* application = nullptr;

    if (message == WM_NCCREATE)
    {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        application = static_cast<Application*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(application));
        application->window_ = window;
    }
    else
    {
        application = reinterpret_cast<Application*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }

    if (application)
    {
        return application->WindowProc(window, message, wParam, lParam);
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT Application::WindowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_TIMER:
        InvalidateRect(window, nullptr, FALSE);
        return 0;

    case WM_MOUSEMOVE:
    {
        mouse_.x = GET_X_LPARAM(lParam);
        mouse_.y = GET_Y_LPARAM(lParam);

        TRACKMOUSEEVENT track{};
        track.cbSize = sizeof(track);
        track.dwFlags = TME_LEAVE;
        track.hwndTrack = window;
        TrackMouseEvent(&track);

        InvalidateRect(window, nullptr, FALSE);
        return 0;
    }

    case WM_MOUSELEAVE:
        mouse_ = { -10000, -10000 };
        InvalidateRect(window, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDOWN:
    {
        SetCapture(window);
        mouseDown_ = true;
        mousePressed_ = true;
        mouse_.x = GET_X_LPARAM(lParam);
        mouse_.y = GET_Y_LPARAM(lParam);

        if (static_cast<float>(mouse_.x) < Layout::SidebarWidth)
        {
            HandleSidebarClick(
                static_cast<float>(mouse_.x),
                static_cast<float>(mouse_.y));
        }

        InvalidateRect(window, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONUP:
        mouseDown_ = false;
        activeSlider_.clear();
        ReleaseCapture();
        return 0;

    case WM_NCHITTEST:
    {
        POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(window, &point);

        // The empty breadcrumb strip acts as the drag area.
        if (point.y >= 2 && point.y <= 47)
        {
            return HTCAPTION;
        }

        return HTCLIENT;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            if (!openCombo_.empty())
            {
                openCombo_.clear();
                InvalidateRect(window, nullptr, FALSE);
            }
            else
            {
                PostMessageW(window, WM_CLOSE, 0, 0);
            }
        }
        return 0;

    case WM_SIZE:
        if (renderTarget_)
        {
            renderTarget_->Resize(D2D1::SizeU(LOWORD(lParam), HIWORD(lParam)));
        }
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT paint{};
        BeginPaint(window, &paint);
        Render();
        EndPaint(window, &paint);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_SETCURSOR:
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
        return TRUE;

    case WM_DESTROY:
        KillTimer(window, 1);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    Application application;

    if (!application.Initialize(instance))
    {
        MessageBoxW(
            nullptr,
            L"Failed to initialize the Direct2D interface.",
            L"Error",
            MB_OK | MB_ICONERROR);

        application.Shutdown();
        return 1;
    }

    const int result = application.Run();
    application.Shutdown();
    return result;
}
