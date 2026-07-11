// ============================================================================
// MenuSense-style Direct2D UI - single main.cpp
//
// HOW TO ADD A NEW VALUE
//   1. Add the variable to AppSettings below.
//   2. Draw it in one of the page functions using Check, Toggle, Slider or
//   Combo.
//   3. Add it to Config::Save and Config::Load if it should be stored.
//
// SETTINGS PAGE
//   Settings contains only window appearance controls and config management.
//   The animated rainbow top bar and all color sliders work immediately.
//
// EXAMPLES INSIDE A PAGE
//
//   Check(L"My checkbox", g_settings.myCheckbox, x, y, columnWidth);
//   y += 28.0f;
//
//   Toggle(L"My toggle", g_settings.myToggle, x, y, columnWidth);
//   y += 40.0f;
//
//   Slider(L"My value", g_settings.myValue, 0.0f, 100.0f,
//          x, y, columnWidth, L"%");
//   y += 53.0f;
//
//   Combo(L"My dropdown", { L"Option 1", L"Option 2", L"Option 3" },
//         g_settings.myDropdown, x, y, columnWidth);
//   y += 38.0f;
//
// Build: Windows x64, C++17 or newer, Unicode, subsystem Windows.
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
#include <d2d1.h>
#include <dwrite.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <windows.h>
#include <windowsx.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// COM and math helpers

template <class T> void Release(T*& p)
{
    if (p)
    {
        p->Release();
        p = nullptr;
    }
}
float Clamp(float v, float a, float b)
{
    return std::max(a, std::min(v, b));
}
bool Hit(float x, float y, float l, float t, float r, float b)
{
    return x >= l && x <= r && y >= t && y <= b;
}
D2D1_COLOR_F C(uint32_t rgb, float a = 1)
{
    return D2D1::ColorF(((rgb >> 16) & 255) / 255.f, ((rgb >> 8) & 255) / 255.f, (rgb & 255) / 255.f, a);
}

D2D1_COLOR_F Hsv(float hue, float saturation, float value, float alpha = 1.0f)
{
    hue = std::fmod(hue, 360.0f);
    if (hue < 0.0f)
        hue += 360.0f;

    saturation = Clamp(saturation, 0.0f, 1.0f);
    value = Clamp(value, 0.0f, 1.0f);

    const float chroma = value * saturation;
    const float section = hue / 60.0f;
    const float x = chroma * (1.0f - std::fabs(std::fmod(section, 2.0f) - 1.0f));
    const float match = value - chroma;

    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;

    if (section < 1.0f)
    {
        red = chroma;
        green = x;
    }
    else if (section < 2.0f)
    {
        red = x;
        green = chroma;
    }
    else if (section < 3.0f)
    {
        green = chroma;
        blue = x;
    }
    else if (section < 4.0f)
    {
        green = x;
        blue = chroma;
    }
    else if (section < 5.0f)
    {
        red = x;
        blue = chroma;
    }
    else
    {
        red = chroma;
        blue = x;
    }

    return D2D1::ColorF(red + match, green + match, blue + match, alpha);
}
// Layout and visual theme

namespace Layout
{
    constexpr int W = 1020, H = 640;
    constexpr float Accent = 3, Title = 42, Side = 184, NavH = 39, NavGap = 3, PadX = 42, PadY = 30;
} // namespace Layout

namespace Theme
{
    auto Bg = C(0x0B0E10);
    auto Title = C(0x0D1012);
    auto Side = C(0x101315);
    auto Panel = C(0x101315);
    auto Ctrl = C(0x15191C);
    auto Hover = C(0x1B2024);
    auto Sel = C(0x171B1E);
    auto Border = C(0x252B2F);
    auto Soft = C(0x1B2023);
    auto Accent = C(0xFF295D);
    auto Accent2 = C(0xFF4773);
    auto Text = C(0xD9DDE0);
    auto Sub = C(0x7B8389);
    auto Muted = C(0x51585D);
    auto White = C(0xFFFFFF);
    auto Success = C(0x39D98A);
    auto Danger = C(0xF04464);
} // namespace Theme
namespace Icons
{
    constexpr const wchar_t* Target = L"\xF272";
    constexpr const wchar_t* Bolt = L"\xE945";
    constexpr const wchar_t* Eye = L"\xE890";
    constexpr const wchar_t* Paint = L"\xE790";
    constexpr const wchar_t* People = L"\xE716";
    constexpr const wchar_t* Globe = L"\xE774";
    constexpr const wchar_t* More = L"\xE712";
    constexpr const wchar_t* Settings = L"\xE713";
    constexpr const wchar_t* Check = L"\xE73E";
    constexpr const wchar_t* Down = L"\xE70D";
    constexpr const wchar_t* Right = L"\xE76C";
    constexpr const wchar_t* Minus = L"\xE738";
    constexpr const wchar_t* Close = L"\xE711";
    constexpr const wchar_t* Save = L"\xE74E";
    constexpr const wchar_t* Load = L"\xE72C";
    constexpr const wchar_t* Add = L"\xE710";
    constexpr const wchar_t* Delete = L"\xE74D";
    constexpr const wchar_t* File = L"\xE8A5";
    constexpr const wchar_t* Info = L"\xE946";
} // namespace Icons

// Sidebar pages

enum class Page
{
    AimAssist,
    Combat,
    Visuals,
    Appearance,
    Players,
    World,
    Miscellaneous,
    Settings
};
struct NavigationItem
{
    Page p;
    const wchar_t* icon;
    const wchar_t* text;
};
const std::vector<NavigationItem> NavigationItems = { {Page::AimAssist, Icons::Target, L"Aim Assist"},
                                                     {Page::Combat, Icons::Bolt, L"Combat"},
                                                     {Page::Visuals, Icons::Eye, L"Visuals"},
                                                     {Page::Appearance, Icons::Paint, L"Appearance"},
                                                     {Page::Players, Icons::People, L"Players"},
                                                     {Page::World, Icons::Globe, L"World"},
                                                     {Page::Miscellaneous, Icons::More, L"Miscellaneous"},
                                                     {Page::Settings, Icons::Settings, L"Settings"} };
// All editable values live here. Add new settings here first.

struct AppSettings
{
    // Combat page
    bool enabled = true;
    bool team = false;
    bool prediction = true;
    bool autoAction = true;
    bool rapid = true;

    bool head = true;
    bool neck = false;
    bool chest = true;
    bool stomach = false;
    bool arms = false;
    bool legs = false;

    bool safe = false;
    bool ignoreLimbs = false;
    bool closest = true;
    bool visible = true;
    bool ignoreProtected = false;

    // Visuals page
    bool boxes = true;
    bool names = true;
    bool distance = false;
    bool health = true;

    // Settings page: window appearance
    bool rainbowTopBar = true;
    float rainbowSpeed = 55.0f;
    float accentHue = 345.0f;
    float backgroundBrightness = 4.5f;
    float surfaceBrightness = 7.5f;

    // Sliders and dropdown selections
    float accuracy = 78.0f;
    float minimum = 55.0f;
    float smoothing = 42.0f;
    float fov = 60.0f;

    int mode = 0;
    int area = 0;
    int priority = 0;
};

AppSettings g_settings;

void ApplyWindowTheme()
{
    const float background = Clamp(g_settings.backgroundBrightness / 100.0f, 0.02f, 0.22f);
    const float surface = Clamp(g_settings.surfaceBrightness / 100.0f, 0.035f, 0.28f);

    Theme::Bg = D2D1::ColorF(background * 0.88f, background * 0.98f, background * 1.05f, 1.0f);
    Theme::Title = D2D1::ColorF(background * 0.94f, background * 1.02f, background * 1.08f, 1.0f);
    Theme::Side = D2D1::ColorF(surface * 0.87f, surface * 0.94f, surface, 1.0f);
    Theme::Panel = D2D1::ColorF(surface * 0.88f, surface * 0.95f, surface, 1.0f);

    Theme::Ctrl = D2D1::ColorF(surface * 1.20f, surface * 1.28f, surface * 1.34f, 1.0f);
    Theme::Hover = D2D1::ColorF(surface * 1.42f, surface * 1.50f, surface * 1.58f, 1.0f);
    Theme::Sel = D2D1::ColorF(surface * 1.28f, surface * 1.36f, surface * 1.43f, 1.0f);
    Theme::Border = D2D1::ColorF(surface * 1.72f, surface * 1.80f, surface * 1.88f, 1.0f);
    Theme::Soft = D2D1::ColorF(surface * 1.30f, surface * 1.38f, surface * 1.45f, 1.0f);

    Theme::Accent = Hsv(g_settings.accentHue, 0.84f, 1.0f);
    Theme::Accent2 = Hsv(g_settings.accentHue + 12.0f, 0.68f, 1.0f);
}

// Simple readable .cfg storage in %APPDATA%\MenuSense\Configs

namespace Config
{
    std::filesystem::path Dir()
    {
        wchar_t a[MAX_PATH]{};
        DWORD n = GetEnvironmentVariableW(L"APPDATA", a, MAX_PATH);
        auto p = n ? std::filesystem::path(a) / L"MenuSense" / L"Configs"
            : std::filesystem::current_path() / L"Configs";
        std::error_code e;
        std::filesystem::create_directories(p, e);
        return p;
    }
    std::wstring Clean(const std::wstring& s)
    {
        std::wstring r;
        for (wchar_t c : s)
            if (std::iswalnum(c) || c == L' ' || c == L'-' || c == L'_')
                r += c;
        while (!r.empty() && r.front() == L' ')
            r.erase(r.begin());
        while (!r.empty() && r.back() == L' ')
            r.pop_back();
        if (r.size() > 40)
            r.resize(40);
        return r;
    }
    std::filesystem::path Path(const std::wstring& n)
    {
        return Dir() / (Clean(n) + L".cfg");
    }
    void WB(std::wofstream& f, const wchar_t* k, bool v)
    {
        f << k << L"=" << (v ? 1 : 0) << L"\n";
    }
    void WF(std::wofstream& f, const wchar_t* k, float v)
    {
        f << k << L"=" << v << L"\n";
    }
    void WI(std::wofstream& f, const wchar_t* k, int v)
    {
        f << k << L"=" << v << L"\n";
    }
    bool Save(const std::wstring& n, const AppSettings& s)
    {
        auto c = Clean(n);
        if (c.empty())
            return false;
        std::wofstream f(Path(c));
        if (!f)
            return false;
        f << L"[MenuSense]\n";
        WB(f, L"enabled", s.enabled);
        WB(f, L"team", s.team);
        WB(f, L"prediction", s.prediction);
        WB(f, L"autoAction", s.autoAction);
        WB(f, L"rapid", s.rapid);
        WB(f, L"head", s.head);
        WB(f, L"neck", s.neck);
        WB(f, L"chest", s.chest);
        WB(f, L"stomach", s.stomach);
        WB(f, L"arms", s.arms);
        WB(f, L"legs", s.legs);
        WB(f, L"safe", s.safe);
        WB(f, L"ignoreLimbs", s.ignoreLimbs);
        WB(f, L"closest", s.closest);
        WB(f, L"visible", s.visible);
        WB(f, L"ignoreProtected", s.ignoreProtected);
        WB(f, L"boxes", s.boxes);
        WB(f, L"names", s.names);
        WB(f, L"distance", s.distance);
        WB(f, L"health", s.health);
        WB(f, L"rainbowTopBar", s.rainbowTopBar);
        WF(f, L"rainbowSpeed", s.rainbowSpeed);
        WF(f, L"accentHue", s.accentHue);
        WF(f, L"backgroundBrightness", s.backgroundBrightness);
        WF(f, L"surfaceBrightness", s.surfaceBrightness);
        WF(f, L"accuracy", s.accuracy);
        WF(f, L"minimum", s.minimum);
        WF(f, L"smoothing", s.smoothing);
        WF(f, L"fov", s.fov);
        WI(f, L"mode", s.mode);
        WI(f, L"area", s.area);
        WI(f, L"priority", s.priority);
        return f.good();
    }
    bool B(const std::wstring& v)
    {
        return v == L"1" || v == L"true" || v == L"True";
    }
    bool Load(const std::wstring& n, AppSettings& s)
    {
        std::wifstream f(Path(n));
        if (!f)
            return false;
        AppSettings x = s;
        std::wstring line;
        while (std::getline(f, line))
        {
            if (line.empty() || line[0] == L'[')
                continue;
            auto p = line.find(L'=');
            if (p == std::wstring::npos)
                continue;
            auto k = line.substr(0, p), v = line.substr(p + 1);
            try
            {
                if (k == L"enabled")
                    x.enabled = B(v);
                else if (k == L"team")
                    x.team = B(v);
                else if (k == L"prediction")
                    x.prediction = B(v);
                else if (k == L"autoAction")
                    x.autoAction = B(v);
                else if (k == L"rapid")
                    x.rapid = B(v);
                else if (k == L"head")
                    x.head = B(v);
                else if (k == L"neck")
                    x.neck = B(v);
                else if (k == L"chest")
                    x.chest = B(v);
                else if (k == L"stomach")
                    x.stomach = B(v);
                else if (k == L"arms")
                    x.arms = B(v);
                else if (k == L"legs")
                    x.legs = B(v);
                else if (k == L"safe")
                    x.safe = B(v);
                else if (k == L"ignoreLimbs")
                    x.ignoreLimbs = B(v);
                else if (k == L"closest")
                    x.closest = B(v);
                else if (k == L"visible")
                    x.visible = B(v);
                else if (k == L"ignoreProtected")
                    x.ignoreProtected = B(v);
                else if (k == L"boxes")
                    x.boxes = B(v);
                else if (k == L"names")
                    x.names = B(v);
                else if (k == L"distance")
                    x.distance = B(v);
                else if (k == L"health")
                    x.health = B(v);
                else if (k == L"rainbowTopBar")
                    x.rainbowTopBar = B(v);
                else if (k == L"rainbowSpeed")
                    x.rainbowSpeed = std::stof(v);
                else if (k == L"accentHue")
                    x.accentHue = std::stof(v);
                else if (k == L"backgroundBrightness")
                    x.backgroundBrightness = std::stof(v);
                else if (k == L"surfaceBrightness")
                    x.surfaceBrightness = std::stof(v);
                else if (k == L"accuracy")
                    x.accuracy = std::stof(v);
                else if (k == L"minimum")
                    x.minimum = std::stof(v);
                else if (k == L"smoothing")
                    x.smoothing = std::stof(v);
                else if (k == L"fov")
                    x.fov = std::stof(v);
                else if (k == L"mode")
                    x.mode = std::stoi(v);
                else if (k == L"area")
                    x.area = std::stoi(v);
                else if (k == L"priority")
                    x.priority = std::stoi(v);
            }
            catch (...)
            {
            }
        }
        x.accuracy = Clamp(x.accuracy, 0, 100);
        x.minimum = Clamp(x.minimum, 0, 100);
        x.smoothing = Clamp(x.smoothing, 0, 100);
        x.fov = Clamp(x.fov, 0, 100);
        x.rainbowSpeed = Clamp(x.rainbowSpeed, 5.0f, 180.0f);
        x.accentHue = Clamp(x.accentHue, 0.0f, 359.0f);
        x.backgroundBrightness = Clamp(x.backgroundBrightness, 2.0f, 18.0f);
        x.surfaceBrightness = Clamp(x.surfaceBrightness, 4.0f, 24.0f);
        x.mode = std::clamp(x.mode, 0, 2);
        x.area = std::clamp(x.area, 0, 2);
        x.priority = std::clamp(x.priority, 0, 2);
        s = x;
        return true;
    }
    bool Delete(const std::wstring& n)
    {
        std::error_code e;
        return std::filesystem::remove(Path(n), e);
    }
    std::vector<std::wstring> All()
    {
        std::vector<std::wstring> r;
        std::error_code e;
        for (auto& x : std::filesystem::directory_iterator(Dir(), e))
        {
            if (e)
                break;
            if (x.is_regular_file() && x.path().extension() == L".cfg")
                r.push_back(x.path().stem().wstring());
        }
        std::sort(r.begin(), r.end());
        return r;
    }
} // namespace Config

// Main Direct2D application and reusable UI controls

class App
{
    HWND h{};
    ID2D1Factory* d2{};
    ID2D1HwndRenderTarget* rt{};
    ID2D1SolidColorBrush* br{};
    IDWriteFactory* dw{};
    IDWriteTextFormat* f{}, * fs{}, * ft{}, * fm{}, * fl{}, * logo{}, * icons{};
    POINT mouse{ -9999, -9999 };
    bool down = false, pressed = false;
    Page page = Page::Combat;
    std::wstring active, openCombo, name, status;
    bool nameFocus = false, statusOk = true;
    float statusTime = 0;
    std::vector<std::wstring> configs;
    int selected = -1;

public:
    bool Init(HINSTANCE);
    int Run();
    void Shut();
    static LRESULT CALLBACK SProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT Proc(HWND, UINT, WPARAM, LPARAM);

private:
    HRESULT InitG();
    HRESULT Dev();
    void Drop();
    HRESULT Font(const wchar_t*, float, DWRITE_FONT_WEIGHT, IDWriteTextFormat**);
    void Render();
    void Set(D2D1_COLOR_F);
    void FR(float, float, float, float, D2D1_COLOR_F);
    void DR(float, float, float, float, D2D1_COLOR_F, float = 1);
    void RR(float, float, float, float, float, D2D1_COLOR_F);
    void OR(float, float, float, float, float, D2D1_COLOR_F, float = 1);
    void Line(float, float, float, float, D2D1_COLOR_F, float = 1);
    void Text(const std::wstring&, float, float, float, float, D2D1_COLOR_F, IDWriteTextFormat* = nullptr,
        DWRITE_TEXT_ALIGNMENT = DWRITE_TEXT_ALIGNMENT_LEADING);
    void Icon(const wchar_t*, float, float, float, float, D2D1_COLOR_F);
    bool Hover(float, float, float, float);
    void Top(float);
    void Title(float);
    void Sidebar(float);
    void Content(float, float);
    void Header(const wchar_t*, const std::wstring&, const std::wstring&, float, float, float);
    void Section(const std::wstring&, float, float, float);
    bool Check(const std::wstring&, bool&, float, float, float);
    bool Toggle(const std::wstring&, bool&, float, float, float);
    bool Slider(const std::wstring&, float&, float, float, float, float, float, const std::wstring & = L"");
    bool Combo(const std::wstring&, const std::vector<std::wstring>&, int&, float, float, float);
    bool Button(const std::wstring&, const wchar_t*, float, float, float, float = 38, bool = false,
        bool = false);
    void Input(std::wstring&, bool&, float, float, float);
    void Combat(float, float, float);
    void Visuals(float, float, float);
    void SettingsPage(float, float, float);
    void Placeholder(const wchar_t*, const std::wstring&, const std::wstring&, float, float, float);
    void Refresh();
    bool Has() const
    {
        return selected >= 0 && selected < (int)configs.size();
    }
    std::wstring Sel() const
    {
        return Has() ? configs[selected] : L"";
    }
    void Msg(const std::wstring& s, bool ok)
    {
        status = s;
        statusOk = ok;
        statusTime = 3;
    }
};

HRESULT App::Font(const wchar_t* family, float size, DWRITE_FONT_WEIGHT w, IDWriteTextFormat** out)
{
    auto hr = dw->CreateTextFormat(family, nullptr, w, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        size, L"en-US", out);
    if (SUCCEEDED(hr))
    {
        (*out)->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        (*out)->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
    return hr;
}
HRESULT App::InitG()
{
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2);
    if (FAILED(hr))
        return hr;
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&dw));
    if (FAILED(hr))
        return hr;
    if (FAILED(hr = Font(L"Segoe UI", 12, DWRITE_FONT_WEIGHT_NORMAL, &f)))
        return hr;
    if (FAILED(hr = Font(L"Segoe UI", 10.5f, DWRITE_FONT_WEIGHT_NORMAL, &fs)))
        return hr;
    if (FAILED(hr = Font(L"Segoe UI", 9, DWRITE_FONT_WEIGHT_NORMAL, &ft)))
        return hr;
    if (FAILED(hr = Font(L"Segoe UI", 13, DWRITE_FONT_WEIGHT_SEMI_BOLD, &fm)))
        return hr;
    if (FAILED(hr = Font(L"Segoe UI", 20, DWRITE_FONT_WEIGHT_SEMI_BOLD, &fl)))
        return hr;
    if (FAILED(hr = Font(L"Segoe UI", 16, DWRITE_FONT_WEIGHT_BOLD, &logo)))
        return hr;
    return Font(L"Segoe MDL2 Assets", 15, DWRITE_FONT_WEIGHT_NORMAL, &icons);
}
HRESULT App::Dev()
{
    if (rt)
        return S_OK;
    RECT r{};
    GetClientRect(h, &r);
    auto hr = d2->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_HARDWARE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
        D2D1::HwndRenderTargetProperties(h, D2D1::SizeU(r.right, r.bottom), D2D1_PRESENT_OPTIONS_IMMEDIATELY),
        &rt);
    if (FAILED(hr))
        return hr;
    return rt->CreateSolidColorBrush(Theme::White, &br);
}
void App::Drop()
{
    Release(br);
    Release(rt);
}
bool App::Init(HINSTANCE ins)
{
    SetProcessDPIAware();
    if (FAILED(InitG()))
        return false;
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = SProc;
    wc.hInstance = ins;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"MenuSenseFixed";
    if (!RegisterClassExW(&wc))
        return false;
    int x = (GetSystemMetrics(SM_CXSCREEN) - Layout::W) / 2,
        y = (GetSystemMetrics(SM_CYSCREEN) - Layout::H) / 2;
    h = CreateWindowExW(0, wc.lpszClassName, L"MenuSense", WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU, x, y,
        Layout::W, Layout::H, nullptr, nullptr, ins, this);
    if (!h)
        return false;
    Refresh();
    ShowWindow(h, SW_SHOW);
    UpdateWindow(h);
    SetTimer(h, 1, 16, nullptr);
    return true;
}
int App::Run()
{
    MSG m{};
    while (GetMessageW(&m, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return (int)m.wParam;
}
void App::Shut()
{
    if (h)
        KillTimer(h, 1);
    Drop();
    Release(icons);
    Release(logo);
    Release(fl);
    Release(fm);
    Release(ft);
    Release(fs);
    Release(f);
    Release(dw);
    Release(d2);
}
void App::Set(D2D1_COLOR_F c)
{
    br->SetColor(c);
}
void App::FR(float l, float t, float r, float b, D2D1_COLOR_F c)
{
    Set(c);
    rt->FillRectangle(D2D1::RectF(l, t, r, b), br);
}
void App::DR(float l, float t, float r, float b, D2D1_COLOR_F c, float w)
{
    Set(c);
    rt->DrawRectangle(D2D1::RectF(l, t, r, b), br, w);
}
void App::RR(float l, float t, float r, float b, float rad, D2D1_COLOR_F c)
{
    Set(c);
    rt->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(l, t, r, b), rad, rad), br);
}
void App::OR(float l, float t, float r, float b, float rad, D2D1_COLOR_F c, float w)
{
    Set(c);
    rt->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(l, t, r, b), rad, rad), br, w);
}
void App::Line(float x1, float y1, float x2, float y2, D2D1_COLOR_F c, float w)
{
    Set(c);
    rt->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), br, w);
}
void App::Text(const std::wstring& s, float x, float y, float w, float hh, D2D1_COLOR_F c,
    IDWriteTextFormat* ff, DWRITE_TEXT_ALIGNMENT a)
{
    if (!ff)
        ff = f;
    ff->SetTextAlignment(a);
    Set(c);
    rt->DrawTextW(s.c_str(), (UINT32)s.size(), ff, D2D1::RectF(x, y, x + w, y + hh), br,
        D2D1_DRAW_TEXT_OPTIONS_CLIP);
}
void App::Icon(const wchar_t* i, float x, float y, float w, float hh, D2D1_COLOR_F c)
{
    Text(i, x, y, w, hh, c, icons, DWRITE_TEXT_ALIGNMENT_CENTER);
}
bool App::Hover(float l, float t, float r, float b)
{
    return Hit((float)mouse.x, (float)mouse.y, l, t, r, b);
}
void App::Top(float w)
{
    D2D1_GRADIENT_STOP stops[7]{};

    const float elapsedSeconds = static_cast<float>(GetTickCount64()) / 1000.0f;
    const float animatedHue = std::fmod(
        elapsedSeconds * g_settings.rainbowSpeed,
        360.0f);

    for (int index = 0; index < 7; ++index)
    {
        stops[index].position = static_cast<float>(index) / 6.0f;

        if (g_settings.rainbowTopBar)
        {
            stops[index].color = Hsv(
                animatedHue + static_cast<float>(index) * 60.0f,
                0.86f,
                1.0f);
        }
        else
        {
            stops[index].color = Hsv(
                g_settings.accentHue + static_cast<float>(index) * 14.0f,
                0.82f,
                1.0f);
        }
    }

    ID2D1GradientStopCollection* stopCollection = nullptr;
    ID2D1LinearGradientBrush* gradientBrush = nullptr;

    if (SUCCEEDED(rt->CreateGradientStopCollection(
        stops,
        ARRAYSIZE(stops),
        &stopCollection)))
    {
        rt->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(0.0f, 0.0f),
                D2D1::Point2F(w, 0.0f)),
            stopCollection,
            &gradientBrush);
    }

    if (gradientBrush)
    {
        rt->FillRectangle(
            D2D1::RectF(0.0f, 0.0f, w, Layout::Accent),
            gradientBrush);
    }

    Release(gradientBrush);
    Release(stopCollection);
}
void App::Title(float w)
{
    float top = Layout::Accent, b = top + Layout::Title;
    FR(0, top, w, b, Theme::Title);
    Text(L"Menu", 15, top, 44, Layout::Title, Theme::Accent, logo);
    Text(L"Sense", 57, top, 65, Layout::Title, Theme::Text, logo);
    Text(L"Direct2D", 127, top, 100, Layout::Title, Theme::Muted, ft);
    bool mh = Hover(w - 82, top, w - 41, b), ch = Hover(w - 41, top, w, b);
    if (mh)
        FR(w - 82, top, w - 41, b, Theme::Hover);
    if (ch)
        FR(w - 41, top, w, b, Theme::Danger);
    Icon(Icons::Minus, w - 82, top, 41, Layout::Title, mh ? Theme::White : Theme::Sub);
    Icon(Icons::Close, w - 41, top, 41, Layout::Title, ch ? Theme::White : Theme::Sub);
}
void App::Sidebar(float hh)
{
    float top = Layout::Accent + Layout::Title;
    FR(0, top, Layout::Side, hh, Theme::Side);
    Line(Layout::Side, top, Layout::Side, hh, Theme::Soft);
    float y = top + 14;
    for (auto& n : NavigationItems)
    {
        float l = 8, r = Layout::Side - 8, b = y + Layout::NavH;
        bool hov = Hover(l, y, r, b), sel = n.p == page;
        if (sel)
        {
            RR(l, y, r, b, 5, Theme::Sel);
            RR(l, y + 9, l + 3, b - 9, 1.5f, Theme::Accent);
        }
        else if (hov)
            RR(l, y, r, b, 5, Theme::Hover);
        Icon(n.icon, l + 9, y, 27, Layout::NavH, sel ? Theme::Accent : hov ? Theme::White : Theme::Sub);
        Text(n.text, l + 40, y, r - l - 50, Layout::NavH,
            sel ? Theme::Accent2
            : hov ? Theme::White
            : Theme::Sub,
            fs);
        if (sel)
            Icon(Icons::Right, r - 25, y, 18, Layout::NavH, Theme::Accent);
        y += Layout::NavH + Layout::NavGap;
    }
    Text(L"VERSION 1.0", 15, hh - 29, Layout::Side - 30, 18, Theme::Muted, ft);
}
void App::Header(const wchar_t* i, const std::wstring& t, const std::wstring& d, float x, float y, float w)
{
    RR(x, y, x + 38, y + 38, 7, Theme::Ctrl);
    Icon(i, x, y, 38, 38, Theme::Accent);
    Text(t, x + 52, y - 1, w - 52, 23, Theme::Text, fl);
    Text(d, x + 52, y + 21, w - 52, 18, Theme::Sub, fs);
}
void App::Section(const std::wstring& t, float x, float y, float w)
{
    Text(t, x, y, 130, 18, Theme::Sub, fs);
    Line(x + 88, y + 9, x + w, y + 9, Theme::Soft);
}
bool App::Check(const std::wstring& lab, bool& v, float x, float y, float w)
{
    bool hov = Hover(x, y, x + w, y + 24);
    if (hov && pressed)
        v = !v;
    RR(x, y + 4, x + 16, y + 20, 3, v ? Theme::Accent : hov ? Theme::Hover : Theme::Ctrl);
    OR(x, y + 4, x + 16, y + 20, 3, v ? Theme::Accent2 : Theme::Border);
    if (v)
        Icon(Icons::Check, x, y + 4, 16, 16, Theme::White);
    Text(lab, x + 25, y, w - 25, 24, hov ? Theme::White : Theme::Text, fs);
    return hov;
}
bool App::Toggle(const std::wstring& lab, bool& v, float x, float y, float w)
{
    bool hov = Hover(x, y, x + w, y + 30);
    if (hov && pressed)
        v = !v;
    Text(lab, x, y, w - 52, 30, hov ? Theme::White : Theme::Text, fs);
    float l = x + w - 42, t = y + 6;
    RR(l, t, l + 38, t + 18, 9, v ? Theme::Accent : Theme::Ctrl);
    OR(l, t, l + 38, t + 18, 9, v ? Theme::Accent2 : Theme::Border);
    float k = v ? l + 22 : l + 3;
    RR(k, t + 3, k + 12, t + 15, 6, Theme::White);
    return hov;
}
bool App::Slider(const std::wstring& lab, float& v, float mn, float mx, float x, float y, float w,
    const std::wstring& suf)
{
    float ty = y + 31;
    bool hov = Hover(x, ty - 8, x + w, ty + 8);
    if (hov && pressed)
        active = lab;
    if (active == lab && down)
        v = mn + Clamp(((float)mouse.x - x) / w, 0, 1) * (mx - mn);
    Text(lab, x, y, w - 65, 18, Theme::Sub, fs);
    Text(std::to_wstring((int)std::round(v)) + suf, x + w - 65, y, 65, 18,
        active == lab ? Theme::Accent2 : Theme::Muted, fs, DWRITE_TEXT_ALIGNMENT_TRAILING);
    RR(x, ty - 2, x + w, ty + 2, 2, Theme::Ctrl);
    float cx = x + w * (v - mn) / (mx - mn);
    RR(x, ty - 2, cx, ty + 2, 2, Theme::Accent);
    RR(cx - 5, ty - 5, cx + 5, ty + 5, 5, hov || active == lab ? Theme::White : Theme::Accent2);
    return hov;
}
bool App::Combo(const std::wstring& lab, const std::vector<std::wstring>& o, int& sel, float x, float y,
    float w)
{
    sel = std::clamp(sel, 0, (int)o.size() - 1);
    float lw = 102, h = 29, bl = x + lw, bw = w - lw;
    bool hov = Hover(bl, y, bl + bw, y + h), opened = openCombo == lab;
    if (hov && pressed)
        openCombo = opened ? L"" : lab;
    Text(lab, x, y, lw - 8, h, Theme::Sub, fs);
    RR(bl, y, bl + bw, y + h, 4, hov || opened ? Theme::Hover : Theme::Ctrl);
    OR(bl, y, bl + bw, y + h, 4, opened ? Theme::Accent : Theme::Border);
    Text(o[sel], bl + 9, y, bw - 38, h, Theme::Text, fs);
    Icon(Icons::Down, bl + bw - 28, y, 20, h, opened ? Theme::Accent : Theme::Sub);
    if (opened)
    {
        float lt = y + h + 4, ih = 28;
        RR(bl, lt, bl + bw, lt + ih * o.size(), 5, Theme::Title);
        OR(bl, lt, bl + bw, lt + ih * o.size(), 5, Theme::Border);
        for (size_t i = 0; i < o.size(); ++i)
        {
            float it = lt + ih * i;
            bool hh = Hover(bl, it, bl + bw, it + ih);
            if (hh)
                RR(bl + 3, it + 2, bl + bw - 3, it + ih - 2, 3, Theme::Hover);
            if ((int)i == sel)
                FR(bl + 4, it + 7, bl + 6, it + ih - 7, Theme::Accent);
            Text(o[i], bl + 11, it, bw - 22, ih,
                (int)i == sel ? Theme::Accent2
                : hh ? Theme::White
                : Theme::Text,
                fs);
            if (hh && pressed)
            {
                sel = (int)i;
                openCombo.clear();
            }
        }
    }
    return hov;
}
bool App::Button(const std::wstring& lab, const wchar_t* i, float x, float y, float w, float hh, bool accent,
    bool danger)
{
    bool hov = Hover(x, y, x + w, y + hh);
    D2D1_COLOR_F bg = Theme::Ctrl, b = Theme::Border;
    if (accent)
    {
        bg = hov ? Theme::Accent2 : Theme::Accent;
        b = Theme::Accent2;
    }
    else if (danger)
    {
        bg = hov ? Theme::Danger : C(0x38151D);
        b = Theme::Danger;
    }
    else if (hov)
        bg = Theme::Hover;
    RR(x, y, x + w, y + hh, 5, bg);
    OR(x, y, x + w, y + hh, 5, b);
    if (i)
        Icon(i, x + 10, y, 24, hh, accent || danger ? Theme::White : hov ? Theme::Accent2 : Theme::Sub);
    Text(lab, x + (i ? 39 : 10), y, w - (i ? 49 : 20), hh,
        accent || danger ? Theme::White
        : hov ? Theme::White
        : Theme::Text,
        fs);
    return hov && pressed;
}
void App::Input(std::wstring& v, bool& focus, float x, float y, float w)
{
    float lw = 86, hh = 34, il = x + lw;
    bool hov = Hover(il, y, x + w, y + hh);
    if (pressed)
        focus = hov;
    Text(L"Name", x, y, lw - 10, hh, Theme::Sub, fs);
    RR(il, y, x + w, y + hh, 4, hov || focus ? Theme::Hover : Theme::Ctrl);
    OR(il, y, x + w, y + hh, 4, focus ? Theme::Accent : Theme::Border);
    Text(v.empty() ? L"Enter config name" : v, il + 10, y, w - lw - 20, hh,
        v.empty() ? Theme::Muted : Theme::Text, fs);
    if (focus && (GetTickCount64() / 500) % 2 == 0)
    {
        float a = std::min((float)v.size() * 6.2f, w - lw - 28);
        Line(il + 10 + a, y + 9, il + 10 + a, y + 25, Theme::Accent2);
    }
}
void App::Combat(float l, float t, float w)
{
    Header(Icons::Bolt, L"Combat", L"Configure general, selection and accuracy options.", l, t, w);
    float gap = 70, cw = (w - gap) / 2, r = l + cw + gap, y = t + 69;
    Section(L"General", l, y, cw);
    y += 28;
    Check(L"Enable", g_settings.enabled, l, y, cw);
    y += 28;
    Check(L"Team check", g_settings.team, l, y, cw);
    y += 28;
    Check(L"Prediction", g_settings.prediction, l, y, cw);
    y += 28;
    Check(L"Automatic action", g_settings.autoAction, l, y, cw);
    y += 28;
    Check(L"Rapid mode", g_settings.rapid, l, y, cw);
    y += 38;
    Combo(L"Mode", { L"Left", L"Right", L"Automatic" }, g_settings.mode, l, y, cw);
    y += 38;
    Combo(L"Target area", { L"Head", L"Chest", L"Closest" }, g_settings.area, l, y, cw);
    y += 54;
    Section(L"Accuracy", l, y, cw);
    y += 27;
    Slider(L"Accuracy", g_settings.accuracy, 0, 100, l, y, cw, L"%");
    y += 53;
    Slider(L"Minimum value", g_settings.minimum, 0, 100, l, y, cw);
    y += 55;
    Check(L"Prefer safe points", g_settings.safe, l, y, cw);
    y += 28;
    Check(L"Ignore limbs", g_settings.ignoreLimbs, l, y, cw);
    y = t + 69;
    Section(L"Target areas", r, y, cw);
    y += 28;
    Check(L"Head", g_settings.head, r, y, cw);
    y += 28;
    Check(L"Neck", g_settings.neck, r, y, cw);
    y += 28;
    Check(L"Chest", g_settings.chest, r, y, cw);
    y += 28;
    Check(L"Stomach", g_settings.stomach, r, y, cw);
    y += 28;
    Check(L"Arms", g_settings.arms, r, y, cw);
    y += 28;
    Check(L"Legs", g_settings.legs, r, y, cw);
    y += 50;
    Section(L"Selection", r, y, cw);
    y += 27;
    Combo(L"Priority", { L"Default", L"Distance", L"Health" }, g_settings.priority, r, y, cw);
    y += 42;
    Check(L"Closest target", g_settings.closest, r, y, cw);
    y += 28;
    Check(L"Visible targets", g_settings.visible, r, y, cw);
    y += 28;
    Check(L"Ignore protected targets", g_settings.ignoreProtected, r, y, cw);
}
void App::Visuals(float l, float t, float w)
{
    Header(
        Icons::Eye,
        L"Visuals",
        L"Configure displayed information and visual behavior.",
        l,
        t,
        w);

    const float gap = 70.0f;
    const float columnWidth = (w - gap) / 2.0f;
    const float rightX = l + columnWidth + gap;

    float y = t + 72.0f;

    Section(L"Display", l, y, columnWidth);
    y += 29.0f;

    Toggle(L"Show boxes", g_settings.boxes, l, y, columnWidth);
    y += 38.0f;

    Toggle(L"Show names", g_settings.names, l, y, columnWidth);
    y += 38.0f;

    Toggle(L"Show distance", g_settings.distance, l, y, columnWidth);
    y += 38.0f;

    Toggle(L"Show health", g_settings.health, l, y, columnWidth);

    y = t + 72.0f;

    Section(L"Fine tuning", rightX, y, columnWidth);
    y += 31.0f;

    Slider(
        L"Field of view",
        g_settings.fov,
        0.0f,
        100.0f,
        rightX,
        y,
        columnWidth,
        L"%");

    y += 60.0f;

    Slider(
        L"Smoothing",
        g_settings.smoothing,
        0.0f,
        100.0f,
        rightX,
        y,
        columnWidth,
        L"%");
}
void App::Refresh()
{
    auto old = Sel();
    configs = Config::All();
    selected = -1;
    if (!old.empty())
    {
        auto it = std::find(configs.begin(), configs.end(), old);
        if (it != configs.end())
            selected = (int)std::distance(configs.begin(), it);
    }
    if (selected < 0 && !configs.empty())
        selected = 0;
}
void App::SettingsPage(float l, float t, float w)
{
    Header(
        Icons::Settings,
        L"Settings",
        L"Window colors, animated top bar and configuration profiles.",
        l,
        t,
        w);

    const float gap = 38.0f;
    const float appearanceWidth = 300.0f;
    const float configX = l + appearanceWidth + gap;
    const float configWidth = w - appearanceWidth - gap;

    // --------------------------------------------------------
    // Left column: window appearance only
    // --------------------------------------------------------

    float y = t + 72.0f;

    Section(L"Window appearance", l, y, appearanceWidth);
    y += 30.0f;

    Toggle(
        L"Animated rainbow top bar",
        g_settings.rainbowTopBar,
        l,
        y,
        appearanceWidth);

    y += 44.0f;

    Slider(
        L"Rainbow speed",
        g_settings.rainbowSpeed,
        5.0f,
        180.0f,
        l,
        y,
        appearanceWidth,
        L"°/s");

    y += 58.0f;

    Slider(
        L"Accent hue",
        g_settings.accentHue,
        0.0f,
        359.0f,
        l,
        y,
        appearanceWidth,
        L"°");

    y += 58.0f;

    Slider(
        L"Background brightness",
        g_settings.backgroundBrightness,
        2.0f,
        18.0f,
        l,
        y,
        appearanceWidth,
        L"%");

    y += 58.0f;

    Slider(
        L"Panel brightness",
        g_settings.surfaceBrightness,
        4.0f,
        24.0f,
        l,
        y,
        appearanceWidth,
        L"%");

    y += 62.0f;

    if (Button(
        L"Reset window colors",
        Icons::Load,
        l,
        y,
        appearanceWidth))
    {
        g_settings.rainbowTopBar = true;
        g_settings.rainbowSpeed = 55.0f;
        g_settings.accentHue = 345.0f;
        g_settings.backgroundBrightness = 4.5f;
        g_settings.surfaceBrightness = 7.5f;
        Msg(L"Window colors reset.", true);
    }

    y += 51.0f;

    Text(
        L"The rainbow animation is rendered by Direct2D and updates every frame.",
        l,
        y,
        appearanceWidth,
        42.0f,
        Theme::Muted,
        ft);

    // --------------------------------------------------------
    // Right column: configuration manager only
    // --------------------------------------------------------

    float configY = t + 72.0f;

    Section(L"Configuration profiles", configX, configY, configWidth);
    configY += 31.0f;

    Input(name, nameFocus, configX, configY, configWidth);
    configY += 52.0f;

    if (Button(
        L"Create new config",
        Icons::Add,
        configX,
        configY,
        configWidth,
        39.0f,
        true))
    {
        const std::wstring cleanName = Config::Clean(name);

        if (cleanName.empty())
        {
            Msg(L"Enter a valid configuration name.", false);
        }
        else if (std::filesystem::exists(Config::Path(cleanName)))
        {
            Msg(L"A configuration with this name already exists.", false);
        }
        else if (Config::Save(cleanName, g_settings))
        {
            name = cleanName;
            Refresh();

            const auto iterator = std::find(
                configs.begin(),
                configs.end(),
                cleanName);

            if (iterator != configs.end())
            {
                selected = static_cast<int>(
                    std::distance(configs.begin(), iterator));
            }

            Msg(L"Configuration created.", true);
        }
        else
        {
            Msg(L"Could not create the configuration.", false);
        }
    }

    configY += 49.0f;

    RR(
        configX,
        configY,
        configX + configWidth,
        configY + 168.0f,
        7.0f,
        Theme::Panel);

    OR(
        configX,
        configY,
        configX + configWidth,
        configY + 168.0f,
        7.0f,
        Theme::Soft);

    if (configs.empty())
    {
        Icon(
            Icons::File,
            configX,
            configY + 35.0f,
            configWidth,
            35.0f,
            Theme::Muted);

        Text(
            L"No configurations",
            configX + 15.0f,
            configY + 74.0f,
            configWidth - 30.0f,
            22.0f,
            Theme::Sub,
            fm,
            DWRITE_TEXT_ALIGNMENT_CENTER);

        Text(
            L"Create one using the name field above.",
            configX + 15.0f,
            configY + 98.0f,
            configWidth - 30.0f,
            20.0f,
            Theme::Muted,
            fs,
            DWRITE_TEXT_ALIGNMENT_CENTER);
    }
    else
    {
        float itemY = configY + 7.0f;

        for (std::size_t index = 0;
            index < configs.size() && index < 4;
            ++index)
        {
            const bool isSelected =
                static_cast<int>(index) == selected;

            const bool isHovered = Hover(
                configX + 7.0f,
                itemY,
                configX + configWidth - 7.0f,
                itemY + 35.0f);

            if (isSelected || isHovered)
            {
                RR(
                    configX + 7.0f,
                    itemY,
                    configX + configWidth - 7.0f,
                    itemY + 35.0f,
                    4.0f,
                    isSelected ? Theme::Sel : Theme::Hover);
            }

            if (isSelected)
            {
                RR(
                    configX + 7.0f,
                    itemY + 8.0f,
                    configX + 10.0f,
                    itemY + 27.0f,
                    1.5f,
                    Theme::Accent);
            }

            Icon(
                Icons::File,
                configX + 16.0f,
                itemY,
                24.0f,
                35.0f,
                isSelected ? Theme::Accent : Theme::Sub);

            Text(
                configs[index],
                configX + 46.0f,
                itemY,
                configWidth - 60.0f,
                35.0f,
                isSelected
                ? Theme::Accent2
                : isHovered
                ? Theme::White
                : Theme::Text,
                fs);

            if (isHovered && pressed)
            {
                selected = static_cast<int>(index);
                name = configs[index];
                nameFocus = false;
            }

            itemY += 39.0f;
        }
    }

    configY += 181.0f;

    const float halfWidth = (configWidth - 10.0f) / 2.0f;

    if (Button(
        L"Save selected",
        Icons::Save,
        configX,
        configY,
        halfWidth))
    {
        if (!Has())
        {
            Msg(L"Select a configuration first.", false);
        }
        else if (Config::Save(Sel(), g_settings))
        {
            Msg(L"Configuration saved.", true);
        }
        else
        {
            Msg(L"Could not save the configuration.", false);
        }
    }

    if (Button(
        L"Load selected",
        Icons::Load,
        configX + halfWidth + 10.0f,
        configY,
        halfWidth))
    {
        if (!Has())
        {
            Msg(L"Select a configuration first.", false);
        }
        else if (Config::Load(Sel(), g_settings))
        {
            name = Sel();
            openCombo.clear();
            ApplyWindowTheme();
            Msg(L"Configuration loaded.", true);
        }
        else
        {
            Msg(L"Could not load the configuration.", false);
        }
    }

    configY += 49.0f;

    if (Button(
        L"Delete selected config",
        Icons::Delete,
        configX,
        configY,
        configWidth,
        39.0f,
        false,
        true))
    {
        if (!Has())
        {
            Msg(L"Select a configuration first.", false);
        }
        else if (Config::Delete(Sel()))
        {
            name.clear();
            Refresh();
            Msg(L"Configuration deleted.", true);
        }
        else
        {
            Msg(L"Could not delete the configuration.", false);
        }
    }

    configY += 52.0f;

    if (!status.empty())
    {
        const D2D1_COLOR_F statusColor =
            statusOk ? Theme::Success : Theme::Danger;

        RR(
            configX,
            configY,
            configX + configWidth,
            configY + 41.0f,
            5.0f,
            D2D1::ColorF(
                statusColor.r,
                statusColor.g,
                statusColor.b,
                0.12f));

        OR(
            configX,
            configY,
            configX + configWidth,
            configY + 41.0f,
            5.0f,
            statusColor);

        Icon(
            statusOk ? Icons::Check : Icons::Info,
            configX + 9.0f,
            configY,
            27.0f,
            41.0f,
            statusColor);

        Text(
            status,
            configX + 41.0f,
            configY,
            configWidth - 51.0f,
            41.0f,
            statusColor,
            fs);
    }
}
void App::Placeholder(const wchar_t* i, const std::wstring& title, const std::wstring& desc, float l, float t,
    float w)
{
    Header(i, title, desc, l, t, w);
    RR(l, t + 82, l + w, t + 193, 7, Theme::Panel);
    OR(l, t + 82, l + w, t + 193, 7, Theme::Soft);
    Icon(Icons::Info, l + 21, t + 105, 42, 42, Theme::Accent);
    Text(L"This page is ready for custom controls.", l + 77, t + 98, w - 98, 30, Theme::Text, fm);
    Text(L"Use Check, Slider, Combo, Toggle and Button to add your own settings.", l + 77, t + 130, w - 98,
        38, Theme::Sub, fs);
}
void App::Content(float w, float hh)
{
    float ct = Layout::Accent + Layout::Title;
    FR(Layout::Side, ct, w, hh, Theme::Bg);
    float l = Layout::Side + Layout::PadX, t = ct + Layout::PadY, cw = w - Layout::Side - Layout::PadX * 2;
    switch (page)
    {
    case Page::Combat:
        Combat(l, t, cw);
        break;
    case Page::Visuals:
        Visuals(l, t, cw);
        break;
    case Page::Settings:
        SettingsPage(l, t, cw);
        break;
    case Page::AimAssist:
        Placeholder(Icons::Target, L"Aim Assist", L"Configure assistance and smoothing options.", l, t, cw);
        break;
    case Page::Appearance:
        Placeholder(Icons::Paint, L"Appearance", L"Customize colors and interface styling.", l, t, cw);
        break;
    case Page::Players:
        Placeholder(Icons::People, L"Players", L"Manage player-specific options.", l, t, cw);
        break;
    case Page::World:
        Placeholder(Icons::Globe, L"World", L"Configure world and environment options.", l, t, cw);
        break;
    case Page::Miscellaneous:
        Placeholder(Icons::More, L"Miscellaneous", L"Add additional application tools.", l, t, cw);
        break;
    }
}
void App::Render()
{
    if (FAILED(Dev()))
        return;
    RECT r{};
    GetClientRect(h, &r);
    float w = (float)r.right, hh = (float)r.bottom;
    if (statusTime > 0)
    {
        statusTime -= .016f;
        if (statusTime <= 0)
        {
            statusTime = 0;
            status.clear();
        }
    }
    ApplyWindowTheme();

    rt->BeginDraw();
    rt->SetTransform(D2D1::Matrix3x2F::Identity());
    rt->Clear(Theme::Bg);
    Top(w);
    Title(w);
    Sidebar(hh);
    Content(w, hh);
    DR(.5f, .5f, w - .5f, hh - .5f, Theme::Border);
    auto hr = rt->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
        Drop();
    pressed = false;
}
LRESULT CALLBACK App::SProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    App* a{};
    if (m == WM_NCCREATE)
    {
        auto* c = (CREATESTRUCTW*)l;
        a = (App*)c->lpCreateParams;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)a);
        a->h = h;
    }
    else
        a = (App*)GetWindowLongPtrW(h, GWLP_USERDATA);
    return a ? a->Proc(h, m, w, l) : DefWindowProcW(h, m, w, l);
}
LRESULT App::Proc(HWND hwnd, UINT m, WPARAM w, LPARAM l)
{
    switch (m)
    {
    case WM_TIMER:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_MOUSEMOVE:
        mouse.x = GET_X_LPARAM(l);
        mouse.y = GET_Y_LPARAM(l);
        {
            TRACKMOUSEEVENT e{ sizeof(e), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&e);
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_MOUSELEAVE:
        mouse = { -9999, -9999 };
        return 0;
    case WM_LBUTTONDOWN:
    {
        SetCapture(hwnd);
        down = true;
        pressed = true;
        mouse.x = GET_X_LPARAM(l);
        mouse.y = GET_Y_LPARAM(l);
        float x = (float)mouse.x, y = (float)mouse.y, bt = Layout::Accent + Layout::Title;
        RECT r{};
        GetClientRect(hwnd, &r);
        if (y <= bt)
        {
            if (x >= r.right - 41)
                PostMessageW(hwnd, WM_CLOSE, 0, 0);
            else if (x >= r.right - 82)
                ShowWindow(hwnd, SW_MINIMIZE);
            else
            {
                ReleaseCapture();
                down = pressed = false;
                SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            }
            return 0;
        }
        if (x <= Layout::Side)
        {
            float iy = bt + 14;
            for (auto& n : NavigationItems)
            {
                if (Hit(x, y, 8, iy, Layout::Side - 8, iy + Layout::NavH))
                {
                    page = n.p;
                    active.clear();
                    openCombo.clear();
                    nameFocus = false;
                    break;
                }
                iy += Layout::NavH + Layout::NavGap;
            }
            return 0;
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_LBUTTONUP:
        down = false;
        active.clear();
        ReleaseCapture();
        return 0;
    case WM_CHAR:
        if (nameFocus)
        {
            wchar_t c = (wchar_t)w;
            if (c == L'\b')
            {
                if (!name.empty())
                    name.pop_back();
            }
            else if (c >= 32 && name.size() < 40 && (std::iswalnum(c) || c == L' ' || c == L'-' || c == L'_'))
                name += c;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_KEYDOWN:
        if (nameFocus)
        {
            if (w == VK_ESCAPE || w == VK_RETURN)
                nameFocus = false;
            return 0;
        }
        if (w == VK_ESCAPE)
        {
            if (!openCombo.empty())
                openCombo.clear();
            else
                PostMessageW(hwnd, WM_CLOSE, 0, 0);
        }
        return 0;
    case WM_SIZE:
        if (rt)
            rt->Resize(D2D1::SizeU(LOWORD(l), HIWORD(l)));
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT p{};
        BeginPaint(hwnd, &p);
        Render();
        EndPaint(hwnd, &p);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_SETCURSOR:
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
        return TRUE;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, m, w, l);
}
int WINAPI wWinMain(HINSTANCE i, HINSTANCE, PWSTR, int)
{
    App a;
    if (!a.Init(i))
    {
        MessageBoxW(nullptr, L"Failed to initialize Direct2D.", L"Error", MB_OK | MB_ICONERROR);
        a.Shut();
        return 1;
    }
    int r = a.Run();
    a.Shut();
    return r;
}
