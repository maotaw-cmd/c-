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
#include <d2d1.h>
#include <dwrite.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <cstdlib>
#include <cwctype>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// Reference-style Direct2D UI — V19 flat dropdown build 8C71 (2026-07-14).
// UI only. It does not connect to or modify any game.

template <typename T>
void SafeRelease(T*& p) {
    if (p) {
        p->Release();
        p = nullptr;
    }
}

static D2D1_COLOR_F Hex(std::uint32_t rgb, float a = 1.0f) {
    return D2D1::ColorF(
        ((rgb >> 16) & 0xFF) / 255.0f,
        ((rgb >> 8) & 0xFF) / 255.0f,
        (rgb & 0xFF) / 255.0f,
        a);
}

static D2D1_COLOR_F FromColorRef(COLORREF color, float a = 1.0f) {
    return D2D1::ColorF(
        GetRValue(color) / 255.0f,
        GetGValue(color) / 255.0f,
        GetBValue(color) / 255.0f,
        a);
}

static COLORREF HsvToColorRef(float h, float s, float v) {
    h = h - std::floor(h);
    s = std::max(0.0f, std::min(1.0f, s));
    v = std::max(0.0f, std::min(1.0f, v));

    const float c = v * s;
    const float hp = h * 6.0f;
    const float x = c * (1.0f - std::fabs(std::fmod(hp, 2.0f) - 1.0f));
    float r = 0.0f, g = 0.0f, b = 0.0f;

    if (hp < 1.0f) { r = c; g = x; }
    else if (hp < 2.0f) { r = x; g = c; }
    else if (hp < 3.0f) { g = c; b = x; }
    else if (hp < 4.0f) { g = x; b = c; }
    else if (hp < 5.0f) { r = x; b = c; }
    else { r = c; b = x; }

    const float m = v - c;
    return RGB(
        static_cast<BYTE>(std::round((r + m) * 255.0f)),
        static_cast<BYTE>(std::round((g + m) * 255.0f)),
        static_cast<BYTE>(std::round((b + m) * 255.0f)));
}

static void ColorRefToHsv(COLORREF color, float& h, float& s, float& v) {
    const float r = GetRValue(color) / 255.0f;
    const float g = GetGValue(color) / 255.0f;
    const float b = GetBValue(color) / 255.0f;
    const float maxv = std::max(r, std::max(g, b));
    const float minv = std::min(r, std::min(g, b));
    const float d = maxv - minv;

    v = maxv;
    s = maxv <= 0.0001f ? 0.0f : d / maxv;

    if (d <= 0.0001f) h = 0.0f;
    else if (maxv == r) h = std::fmod((g - b) / d, 6.0f) / 6.0f;
    else if (maxv == g) h = (((b - r) / d) + 2.0f) / 6.0f;
    else h = (((r - g) / d) + 4.0f) / 6.0f;

    if (h < 0.0f) h += 1.0f;
}

static bool Inside(float x, float y, float l, float t, float r, float b) {
    return x >= l && x <= r && y >= t && y <= b;
}

static float Clamp01(float v) {
    return std::max(0.0f, std::min(1.0f, v));
}

namespace Ui {
    constexpr int Width = 900;
    constexpr int Height = 600;
    constexpr float HeaderH = 91.0f;
    constexpr float Margin = 28.0f;
    constexpr float Gap = 22.0f;
    constexpr float ColumnW = 411.0f;
}

namespace C {
    const D2D1_COLOR_F BgTop = Hex(0x163873);
    const D2D1_COLOR_F BgMid = Hex(0x0B2152);
    const D2D1_COLOR_F BgBottom = Hex(0x160A3D);
    const D2D1_COLOR_F Panel = Hex(0x091943, 0.82f);
    const D2D1_COLOR_F PanelHover = Hex(0x102657, 0.90f);
    const D2D1_COLOR_F PanelEdge = Hex(0x33599B, 0.24f);
    const D2D1_COLOR_F Accent = Hex(0xA867FF);
    const D2D1_COLOR_F Accent2 = Hex(0x756DFF);
    const D2D1_COLOR_F Accent3 = Hex(0xF52CCB);
    const D2D1_COLOR_F Text = Hex(0xC7D1FF);
    const D2D1_COLOR_F Bright = Hex(0xF0EBFF);
    const D2D1_COLOR_F Muted = Hex(0x7182C1);
    const D2D1_COLOR_F Muted2 = Hex(0x4C5B93);
    const D2D1_COLOR_F Track = Hex(0x26366D);
    const D2D1_COLOR_F Dark = Hex(0x050B23);
    const D2D1_COLOR_F Knock = Hex(0xE7D9FF);
}

struct State {
    int page = 0;

    bool enabled = true;
    bool visibility = true;
    bool prediction = false;
    bool smoothing = true;
    bool recoil = false;
    bool radar = true;
    bool boxes = true;
    bool names = true;
    bool health = true;
    bool glow = false;
    bool streamProof = true;
    bool notifications = true;
    bool autoSave = true;
    bool trigger = false;
    bool crosshair = true;
    bool skeleton = false;
    bool hitSound = false;
    bool fpsLimit = true;
    bool animateBg = true;
    bool watermark = true;
    bool keyHints = true;
    bool compactAlerts = false;
    bool controllerNavigation = false;
    bool performanceMode = false;
    bool autoUpdate = true;
    bool lowLatencyMode = true;
    bool telemetry = false;

    float fov = 72.0f;
    float smooth = 0.42f;
    float distance = 180.0f;
    float thickness = 1.8f;
    float uiScale = 100.0f;
    float opacity = 88.0f;
    float brightness = 74.0f;

    int target = 0;
    int bone = 0;
    int theme = 0;
    int config = 0;

    COLORREF boxColor = RGB(184, 78, 255);
    COLORREF nameColor = RGB(209, 155, 255);
    COLORREF healthColor = RGB(117, 255, 177);
    COLORREF skeletonColor = RGB(127, 107, 255);
    COLORREF glowColor = RGB(255, 79, 216);
    COLORREF crosshairColor = RGB(0, 209, 255);
    COLORREF radarColor = RGB(184, 78, 255);

    bool dropdownTarget = false;
    bool dropdownBone = false;
    bool dropdownTheme = false;
    bool dropdownConfig = false;
};

class App {
public:
    bool Init(HINSTANCE instance);
    int Run();
    void Shutdown();

private:
    HWND hwnd_ = nullptr;
    ID2D1Factory* factory_ = nullptr;
    ID2D1HwndRenderTarget* target_ = nullptr;
    ID2D1SolidColorBrush* brush_ = nullptr;
    IDWriteFactory* write_ = nullptr;
    IDWriteTextFormat* font_ = nullptr;
    IDWriteTextFormat* small_ = nullptr;
    IDWriteTextFormat* tiny_ = nullptr;
    IDWriteTextFormat* heading_ = nullptr;
    IDWriteTextFormat* logo_ = nullptr;
    IDWriteTextFormat* icon_ = nullptr;

    POINT mouse_{ -10000, -10000 };
    bool pressed_ = false;
    bool down_ = false;
    std::wstring dragSlider_;
    State state_;
    std::unordered_map<std::wstring, float> toggleAnim_;
    float anim_ = 0.0f;
    float previewHealth_ = 0.82f;
    float appliedUiScale_ = 100.0f;
    bool draggingConfigScrollbar_ = false;
    float configScrollGrab_ = 0.0f;
    int configScroll_ = 0;

    bool colorPickerOpen_ = false;
    bool draggingPickerSquare_ = false;
    bool draggingPickerHue_ = false;
    COLORREF* activeColor_ = nullptr;
    float pickerHue_ = 0.78f;
    float pickerSat_ = 0.70f;
    float pickerVal_ = 1.0f;

    std::wstring configName_ = L"My Config";
    bool configNameFocused_ = false;
    std::vector<std::wstring> savedConfigs_;
    int selectedConfig_ = -1;
    std::wstring configStatus_ = L"Enter a name and save your configuration.";

    struct DeferredDropdown {
        std::wstring id;
        std::vector<std::wstring> options;
        int* selected = nullptr;
        bool* open = nullptr;
        float x = 0.0f;
        float boxY = 0.0f;
        float width = 0.0f;
        bool opensUp = false;
    };

    std::vector<DeferredDropdown> dropdownOverlays_;
    std::wstring dropdownOpenedThisFrame_;

    static LRESULT CALLBACK StaticProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT Proc(HWND, UINT, WPARAM, LPARAM);

    HRESULT CreateIndependent();
    HRESULT CreateDevice();
    HRESULT MakeFont(const wchar_t*, float, DWRITE_FONT_WEIGHT, IDWriteTextFormat**);
    void DiscardDevice();

    void Render();
    void Set(D2D1_COLOR_F c);
    void Fill(float l, float t, float r, float b, D2D1_COLOR_F c);
    void Round(float l, float t, float r, float b, float radius, D2D1_COLOR_F c);
    void RoundStroke(float l, float t, float r, float b, float radius, D2D1_COLOR_F c, float w = 1.0f);
    void Line(float x1, float y1, float x2, float y2, D2D1_COLOR_F c, float w = 1.0f);
    void Ellipse(float x, float y, float rx, float ry, D2D1_COLOR_F c);
    void EllipseStroke(float x, float y, float rx, float ry, D2D1_COLOR_F c, float width = 1.0f);
    void Bone(float x1, float y1, float x2, float y2, D2D1_COLOR_F c, float width);
    void Joint(float x, float y, D2D1_COLOR_F c, float radius = 2.5f);
    void Text(const std::wstring&, float x, float y, float w, float h, D2D1_COLOR_F,
        IDWriteTextFormat* = nullptr,
        DWRITE_TEXT_ALIGNMENT = DWRITE_TEXT_ALIGNMENT_LEADING);
    void Glow(float x, float y, float rx, float ry, D2D1_COLOR_F inner);
    bool Hover(float l, float t, float r, float b) const;
    D2D1_COLOR_F ActiveAccent() const;
    void OpenColorPicker(COLORREF* color);
    void UpdatePickerFromMouse();
    void DrawColorPicker();
    std::wstring ConfigFolder() const;
    std::wstring ConfigPath(const std::wstring& name) const;
    std::wstring SanitizeConfigName(const std::wstring& name) const;
    void RefreshConfigs();
    void SaveConfig(const std::wstring& name);
    void LoadConfig(const std::wstring& name);
    void DeleteConfig(const std::wstring& name);
    void ResetDefaults();
    void ApplyUiScale();

    void DrawBackground();
    void DrawAnimatedPurple();
    void DrawHeader();
    void DrawNavIcon(int index, float cx, float cy, D2D1_COLOR_F color);
    void DrawWindowButtons();
    void DrawPanel(float x, float y, float w, float h, const std::wstring& title, const std::wstring& iconText = L"");
    void DrawToggle(const std::wstring& label, bool& value, float x, float y, float width, bool enabled = true);
    void DrawColorToggle(const std::wstring& label, bool& value, COLORREF color, float x, float y, float width);
    void DrawSlider(const std::wstring& id, const std::wstring& label, float& value,
        float min, float max, float x, float y, float width,
        const std::wstring& suffix, int decimals);
    void DrawDropdown(const std::wstring& id, const std::wstring& label,
        const std::vector<std::wstring>& options, int& selected,
        bool& open, float x, float y, float width);
    void DrawDropdownOverlays();
    void DrawActionButton(const std::wstring& text, float x, float y, float width);
    void DrawInfoRow(const std::wstring& left, const std::wstring& right, float x, float y, float width);
    void DrawMetricCard(float x, float y, float w, const std::wstring& title, const std::wstring& value);

    void DrawAimbotPage();
    void DrawVisualsPage();
    void DrawMiscPage();
    void DrawSettingsPage();
};

HRESULT App::MakeFont(const wchar_t* family, float size, DWRITE_FONT_WEIGHT weight,
    IDWriteTextFormat** out) {
    HRESULT hr = write_->CreateTextFormat(
        family, nullptr, weight, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, size, L"en-US", out);
    if (SUCCEEDED(hr)) {
        (*out)->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        (*out)->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
    return hr;
}

HRESULT App::CreateIndependent() {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory_);
    if (FAILED(hr)) return hr;

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&write_));
    if (FAILED(hr)) return hr;

    if (FAILED(hr = MakeFont(L"Segoe UI", 15.0f, DWRITE_FONT_WEIGHT_NORMAL, &font_))) return hr;
    if (FAILED(hr = MakeFont(L"Segoe UI", 13.0f, DWRITE_FONT_WEIGHT_NORMAL, &small_))) return hr;
    if (FAILED(hr = MakeFont(L"Segoe UI", 11.0f, DWRITE_FONT_WEIGHT_NORMAL, &tiny_))) return hr;
    if (FAILED(hr = MakeFont(L"Segoe UI", 13.0f, DWRITE_FONT_WEIGHT_BOLD, &heading_))) return hr;
    if (FAILED(hr = MakeFont(L"Segoe UI", 21.0f, DWRITE_FONT_WEIGHT_BOLD, &logo_))) return hr;
    return MakeFont(L"Segoe MDL2 Assets", 18.0f, DWRITE_FONT_WEIGHT_NORMAL, &icon_);
}

HRESULT App::CreateDevice() {
    if (target_) return S_OK;

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    HRESULT hr = factory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_HARDWARE),
        D2D1::HwndRenderTargetProperties(
            hwnd_, D2D1::SizeU(rc.right, rc.bottom), D2D1_PRESENT_OPTIONS_IMMEDIATELY),
        &target_);
    if (FAILED(hr)) return hr;

    return target_->CreateSolidColorBrush(C::Bright, &brush_);
}

void App::DiscardDevice() {
    SafeRelease(brush_);
    SafeRelease(target_);
}

bool App::Init(HINSTANCE instance) {
    SetProcessDPIAware();
    if (FAILED(CreateIndependent())) return false;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = StaticProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"ReferenceUIV19FlatDropdown_8C71";
    if (!RegisterClassExW(&wc)) return false;

    const int x = (GetSystemMetrics(SM_CXSCREEN) - Ui::Width) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - Ui::Height) / 2;

    hwnd_ = CreateWindowExW(
        WS_EX_APPWINDOW,
        wc.lpszClassName,
        L"Reference UI V19 8C71",
        WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, Ui::Width, Ui::Height,
        nullptr, nullptr, instance, this);
    if (!hwnd_) return false;

    RefreshConfigs();
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    SetTimer(hwnd_, 1, 16, nullptr);
    return true;
}

int App::Run() {
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

void App::Shutdown() {
    if (hwnd_) KillTimer(hwnd_, 1);
    DiscardDevice();
    SafeRelease(icon_);
    SafeRelease(logo_);
    SafeRelease(heading_);
    SafeRelease(tiny_);
    SafeRelease(small_);
    SafeRelease(font_);
    SafeRelease(write_);
    SafeRelease(factory_);
}

void App::Set(D2D1_COLOR_F c) { brush_->SetColor(c); }

void App::Fill(float l, float t, float r, float b, D2D1_COLOR_F c) {
    Set(c);
    target_->FillRectangle(D2D1::RectF(l, t, r, b), brush_);
}

void App::Round(float l, float t, float r, float b, float radius, D2D1_COLOR_F c) {
    Set(c);
    target_->FillRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(l, t, r, b), radius, radius), brush_);
}

void App::RoundStroke(float l, float t, float r, float b, float radius, D2D1_COLOR_F c, float w) {
    Set(c);
    target_->DrawRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(l, t, r, b), radius, radius), brush_, w);
}

void App::Line(float x1, float y1, float x2, float y2, D2D1_COLOR_F c, float w) {
    Set(c);
    target_->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush_, w);
}

void App::Ellipse(float x, float y, float rx, float ry, D2D1_COLOR_F c) {
    Set(c);
    target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), rx, ry), brush_);
}

void App::EllipseStroke(float x, float y, float rx, float ry,
    D2D1_COLOR_F c, float width) {
    Set(c);
    target_->DrawEllipse(
        D2D1::Ellipse(D2D1::Point2F(x, y), rx, ry), brush_, width);
}

void App::Bone(float x1, float y1, float x2, float y2, D2D1_COLOR_F c, float width) {
    // Dark outer pass plus colored inner pass gives the bone rig the crisp ESP look.
    Line(x1, y1, x2, y2, Hex(0x030615, 0.96f), width + 2.6f);
    Line(x1, y1, x2, y2, c, width);
}

void App::Joint(float x, float y, D2D1_COLOR_F c, float radius) {
    Ellipse(x, y, radius + 1.6f, radius + 1.6f, Hex(0x030615, 0.96f));
    Ellipse(x, y, radius, radius, c);
}

void App::Text(const std::wstring& s, float x, float y, float w, float h,
    D2D1_COLOR_F c, IDWriteTextFormat* f, DWRITE_TEXT_ALIGNMENT align) {
    if (!f) f = font_;
    f->SetTextAlignment(align);
    Set(c);
    target_->DrawTextW(
        s.c_str(), static_cast<UINT32>(s.size()), f,
        D2D1::RectF(x, y, x + w, y + h), brush_, D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

void App::Glow(float x, float y, float rx, float ry, D2D1_COLOR_F inner) {
    D2D1_GRADIENT_STOP stops[] = {
        {0.0f, inner},
        {1.0f, Hex(0x000000, 0.0f)}
    };

    ID2D1GradientStopCollection* collection = nullptr;
    ID2D1RadialGradientBrush* gradient = nullptr;

    if (SUCCEEDED(target_->CreateGradientStopCollection(stops, 2, &collection))) {
        target_->CreateRadialGradientBrush(
            D2D1::RadialGradientBrushProperties(
                D2D1::Point2F(x, y), D2D1::Point2F(0, 0), rx, ry),
            collection, &gradient);
    }

    if (gradient) {
        target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), rx, ry), gradient);
    }

    SafeRelease(gradient);
    SafeRelease(collection);
}

bool App::Hover(float l, float t, float r, float b) const {
    return Inside(static_cast<float>(mouse_.x), static_cast<float>(mouse_.y), l, t, r, b);
}

D2D1_COLOR_F App::ActiveAccent() const {
    switch (state_.theme) {
    case 1: return Hex(0x756DFF); // Deep violet
    case 2: return Hex(0xF52CCB); // Magenta night
    default: return Hex(0xA867FF); // Purple neon
    }
}

void App::ApplyUiScale() {
    const float clamped = std::max(80.0f, std::min(120.0f, state_.uiScale));
    if (std::fabs(clamped - appliedUiScale_) < 0.20f || !hwnd_) return;

    RECT rc{};
    GetWindowRect(hwnd_, &rc);
    const float scale = clamped / 100.0f;
    const int width = static_cast<int>(std::round(Ui::Width * scale));
    const int height = static_cast<int>(std::round(Ui::Height * scale));
    SetWindowPos(hwnd_, nullptr, rc.left, rc.top, width, height,
        SWP_NOZORDER | SWP_NOACTIVATE);
    appliedUiScale_ = clamped;
}

void App::OpenColorPicker(COLORREF* color) {
    if (!color) return;
    activeColor_ = color;
    ColorRefToHsv(*color, pickerHue_, pickerSat_, pickerVal_);
    colorPickerOpen_ = true;
    draggingPickerSquare_ = false;
    draggingPickerHue_ = false;
}

void App::UpdatePickerFromMouse() {
    if (!activeColor_) return;

    const float px = 230.0f;
    const float py = 105.0f;
    const float squareX = px + 24.0f;
    const float squareY = py + 62.0f;
    const float squareW = 392.0f;
    const float squareH = 176.0f;
    const float hueX = squareX;
    const float hueY = squareY + squareH + 18.0f;
    const float hueW = squareW;
    const float hueH = 18.0f;

    if (draggingPickerSquare_) {
        pickerSat_ = Clamp01((static_cast<float>(mouse_.x) - squareX) / squareW);
        pickerVal_ = 1.0f - Clamp01((static_cast<float>(mouse_.y) - squareY) / squareH);
    }
    if (draggingPickerHue_) {
        pickerHue_ = Clamp01((static_cast<float>(mouse_.x) - hueX) / hueW);
        if (pickerHue_ >= 1.0f) pickerHue_ = 0.9999f;
    }

    *activeColor_ = HsvToColorRef(pickerHue_, pickerSat_, pickerVal_);
}

std::wstring App::ConfigFolder() const {
    wchar_t modulePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    std::wstring folder(modulePath);
    const size_t slash = folder.find_last_of(L"\\/");
    if (slash != std::wstring::npos) folder.resize(slash + 1);
    else folder.clear();
    folder += L"phantom_configs\\";
    CreateDirectoryW(folder.c_str(), nullptr);
    return folder;
}

std::wstring App::SanitizeConfigName(const std::wstring& name) const {
    std::wstring clean;
    for (wchar_t ch : name) {
        if (std::iswalnum(ch) || ch == L' ' || ch == L'-' || ch == L'_') clean += ch;
        else clean += L'_';
    }
    while (!clean.empty() && clean.front() == L' ') clean.erase(clean.begin());
    while (!clean.empty() && clean.back() == L' ') clean.pop_back();
    if (clean.empty()) clean = L"Config";
    return clean;
}

std::wstring App::ConfigPath(const std::wstring& name) const {
    return ConfigFolder() + SanitizeConfigName(name) + L".ini";
}

void App::RefreshConfigs() {
    savedConfigs_.clear();
    const std::wstring folder = ConfigFolder();
    WIN32_FIND_DATAW data{};
    HANDLE search = FindFirstFileW((folder + L"*.ini").c_str(), &data);
    if (search != INVALID_HANDLE_VALUE) {
        do {
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) continue;
            const std::wstring path = folder + data.cFileName;
            wchar_t displayName[128]{};
            GetPrivateProfileStringW(L"Meta", L"Name", L"", displayName, 128, path.c_str());
            std::wstring name = displayName;
            if (name.empty()) {
                name = data.cFileName;
                const size_t dot = name.find_last_of(L'.');
                if (dot != std::wstring::npos) name.resize(dot);
            }
            savedConfigs_.push_back(name);
        } while (FindNextFileW(search, &data));
        FindClose(search);
    }
    std::sort(savedConfigs_.begin(), savedConfigs_.end());
    if (selectedConfig_ >= static_cast<int>(savedConfigs_.size())) selectedConfig_ = -1;
    const int maxScroll = std::max(0, static_cast<int>(savedConfigs_.size()) - 3);
    configScroll_ = std::max(0, std::min(configScroll_, maxScroll));
}

void App::SaveConfig(const std::wstring& name) {
    const std::wstring cleanName = SanitizeConfigName(name);
    const std::wstring path = ConfigPath(cleanName);
    WritePrivateProfileStringW(L"Meta", L"Name", cleanName.c_str(), path.c_str());

    auto writeBool = [&](const wchar_t* key, bool value) {
        WritePrivateProfileStringW(L"Options", key, value ? L"1" : L"0", path.c_str());
        };
    auto writeFloat = [&](const wchar_t* key, float value) {
        const std::wstring text = std::to_wstring(value);
        WritePrivateProfileStringW(L"Values", key, text.c_str(), path.c_str());
        };
    auto writeInt = [&](const wchar_t* section, const wchar_t* key, unsigned long value) {
        const std::wstring text = std::to_wstring(value);
        WritePrivateProfileStringW(section, key, text.c_str(), path.c_str());
        };

    writeBool(L"Enabled", state_.enabled);
    writeBool(L"Visibility", state_.visibility);
    writeBool(L"Prediction", state_.prediction);
    writeBool(L"Smoothing", state_.smoothing);
    writeBool(L"Recoil", state_.recoil);
    writeBool(L"Radar", state_.radar);
    writeBool(L"Boxes", state_.boxes);
    writeBool(L"Names", state_.names);
    writeBool(L"Health", state_.health);
    writeBool(L"Glow", state_.glow);
    writeBool(L"Crosshair", state_.crosshair);
    writeBool(L"Skeleton", state_.skeleton);
    writeBool(L"StreamProof", state_.streamProof);
    writeBool(L"Notifications", state_.notifications);
    writeBool(L"AutoSave", state_.autoSave);
    writeBool(L"Trigger", state_.trigger);
    writeBool(L"HitSound", state_.hitSound);
    writeBool(L"FpsLimit", state_.fpsLimit);
    writeBool(L"AnimateBackground", state_.animateBg);
    writeBool(L"Watermark", state_.watermark);
    writeBool(L"KeyHints", state_.keyHints);
    writeBool(L"CompactAlerts", state_.compactAlerts);
    writeBool(L"ControllerNavigation", state_.controllerNavigation);
    writeBool(L"PerformanceMode", state_.performanceMode);
    writeBool(L"AutoUpdate", state_.autoUpdate);
    writeBool(L"LowLatencyMode", state_.lowLatencyMode);
    writeBool(L"Telemetry", state_.telemetry);

    writeFloat(L"Fov", state_.fov);
    writeFloat(L"Smooth", state_.smooth);
    writeFloat(L"Distance", state_.distance);
    writeFloat(L"Thickness", state_.thickness);
    writeFloat(L"UiScale", state_.uiScale);
    writeFloat(L"Opacity", state_.opacity);
    writeFloat(L"Brightness", state_.brightness);

    writeInt(L"Indexes", L"Target", state_.target);
    writeInt(L"Indexes", L"Bone", state_.bone);
    writeInt(L"Indexes", L"Theme", state_.theme);

    writeInt(L"Colors", L"Box", state_.boxColor);
    writeInt(L"Colors", L"Name", state_.nameColor);
    writeInt(L"Colors", L"Health", state_.healthColor);
    writeInt(L"Colors", L"Skeleton", state_.skeletonColor);
    writeInt(L"Colors", L"Glow", state_.glowColor);
    writeInt(L"Colors", L"Crosshair", state_.crosshairColor);
    writeInt(L"Colors", L"Radar", state_.radarColor);

    configName_ = cleanName;
    configStatus_ = L"Saved " + cleanName;
    RefreshConfigs();
    for (int i = 0; i < static_cast<int>(savedConfigs_.size()); ++i) {
        if (savedConfigs_[i] == cleanName) selectedConfig_ = i;
    }
    if (selectedConfig_ >= 0) {
        if (selectedConfig_ < configScroll_) configScroll_ = selectedConfig_;
        if (selectedConfig_ > configScroll_ + 2) configScroll_ = selectedConfig_ - 2;
    }
}

void App::LoadConfig(const std::wstring& name) {
    const std::wstring path = ConfigPath(name);
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        configStatus_ = L"Config file was not found.";
        return;
    }

    auto readBool = [&](const wchar_t* key, bool fallback) {
        return GetPrivateProfileIntW(L"Options", key, fallback ? 1 : 0, path.c_str()) != 0;
        };
    auto readFloat = [&](const wchar_t* key, float fallback) {
        wchar_t buffer[64]{};
        const std::wstring fallbackText = std::to_wstring(fallback);
        GetPrivateProfileStringW(L"Values", key, fallbackText.c_str(), buffer, 64, path.c_str());
        return std::wcstof(buffer, nullptr);
        };
    auto readInt = [&](const wchar_t* section, const wchar_t* key, unsigned long fallback) {
        wchar_t buffer[64]{};
        const std::wstring fallbackText = std::to_wstring(fallback);
        GetPrivateProfileStringW(section, key, fallbackText.c_str(), buffer, 64, path.c_str());
        return std::wcstoul(buffer, nullptr, 10);
        };

    state_.enabled = readBool(L"Enabled", state_.enabled);
    state_.visibility = readBool(L"Visibility", state_.visibility);
    state_.prediction = readBool(L"Prediction", state_.prediction);
    state_.smoothing = readBool(L"Smoothing", state_.smoothing);
    state_.recoil = readBool(L"Recoil", state_.recoil);
    state_.radar = readBool(L"Radar", state_.radar);
    state_.boxes = readBool(L"Boxes", state_.boxes);
    state_.names = readBool(L"Names", state_.names);
    state_.health = readBool(L"Health", state_.health);
    state_.glow = readBool(L"Glow", state_.glow);
    state_.crosshair = readBool(L"Crosshair", state_.crosshair);
    state_.skeleton = readBool(L"Skeleton", state_.skeleton);
    state_.streamProof = readBool(L"StreamProof", state_.streamProof);
    state_.notifications = readBool(L"Notifications", state_.notifications);
    state_.autoSave = readBool(L"AutoSave", state_.autoSave);
    state_.trigger = readBool(L"Trigger", state_.trigger);
    state_.hitSound = readBool(L"HitSound", state_.hitSound);
    state_.fpsLimit = readBool(L"FpsLimit", state_.fpsLimit);
    state_.animateBg = readBool(L"AnimateBackground", state_.animateBg);
    state_.watermark = readBool(L"Watermark", state_.watermark);
    state_.keyHints = readBool(L"KeyHints", state_.keyHints);
    state_.compactAlerts = readBool(L"CompactAlerts", state_.compactAlerts);
    state_.controllerNavigation = readBool(L"ControllerNavigation", state_.controllerNavigation);
    state_.performanceMode = readBool(L"PerformanceMode", state_.performanceMode);
    state_.autoUpdate = readBool(L"AutoUpdate", state_.autoUpdate);
    state_.lowLatencyMode = readBool(L"LowLatencyMode", state_.lowLatencyMode);
    state_.telemetry = readBool(L"Telemetry", state_.telemetry);

    state_.fov = readFloat(L"Fov", state_.fov);
    state_.smooth = readFloat(L"Smooth", state_.smooth);
    state_.distance = readFloat(L"Distance", state_.distance);
    state_.thickness = readFloat(L"Thickness", state_.thickness);
    state_.uiScale = std::max(80.0f, std::min(120.0f,
        readFloat(L"UiScale", state_.uiScale)));
    state_.opacity = std::max(45.0f, std::min(100.0f,
        readFloat(L"Opacity", state_.opacity)));
    state_.brightness = std::max(40.0f, std::min(100.0f,
        readFloat(L"Brightness", state_.brightness)));

    state_.target = static_cast<int>(readInt(L"Indexes", L"Target", state_.target));
    state_.bone = static_cast<int>(readInt(L"Indexes", L"Bone", state_.bone));
    state_.theme = std::max(0, std::min(2,
        static_cast<int>(readInt(L"Indexes", L"Theme", state_.theme))));

    state_.boxColor = static_cast<COLORREF>(readInt(L"Colors", L"Box", state_.boxColor));
    state_.nameColor = static_cast<COLORREF>(readInt(L"Colors", L"Name", state_.nameColor));
    state_.healthColor = static_cast<COLORREF>(readInt(L"Colors", L"Health", state_.healthColor));
    state_.skeletonColor = static_cast<COLORREF>(readInt(L"Colors", L"Skeleton", state_.skeletonColor));
    state_.glowColor = static_cast<COLORREF>(readInt(L"Colors", L"Glow", state_.glowColor));
    state_.crosshairColor = static_cast<COLORREF>(readInt(L"Colors", L"Crosshair", state_.crosshairColor));
    state_.radarColor = static_cast<COLORREF>(readInt(L"Colors", L"Radar", state_.radarColor));

    appliedUiScale_ = -1.0f;
    ApplyUiScale();
    configName_ = name;
    configStatus_ = L"Loaded " + name;
    toggleAnim_.clear();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::DeleteConfig(const std::wstring& name) {
    if (name.empty()) return;
    const std::wstring path = ConfigPath(name);
    if (DeleteFileW(path.c_str())) {
        configStatus_ = L"Deleted " + name;
        selectedConfig_ = -1;
        RefreshConfigs();
    }
    else {
        configStatus_ = L"Could not delete that config.";
    }
}

void App::ResetDefaults() {
    const int page = state_.page;
    state_ = State{};
    state_.page = page;
    toggleAnim_.clear();
    appliedUiScale_ = -1.0f;
    ApplyUiScale();
    configStatus_ = L"Defaults restored. Save with any name.";
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::DrawBackground() {
    D2D1_GRADIENT_STOP stops[] = {
        {0.0f, C::BgTop},
        {0.48f, C::BgMid},
        {1.0f, C::BgBottom}
    };

    ID2D1GradientStopCollection* collection = nullptr;
    ID2D1LinearGradientBrush* gradient = nullptr;
    if (SUCCEEDED(target_->CreateGradientStopCollection(stops, 3, &collection))) {
        target_->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(0, 0), D2D1::Point2F(0, Ui::Height)),
            collection, &gradient);
    }
    if (gradient) {
        target_->FillRectangle(D2D1::RectF(0, 0, Ui::Width, Ui::Height), gradient);
    }
    SafeRelease(gradient);
    SafeRelease(collection);

    Glow(430.0f, -20.0f, 560.0f, 170.0f, Hex(0x4E79FF, 0.13f));
    Glow(115.0f, 285.0f, 150.0f, 210.0f, Hex(0x6B3CFF, 0.055f));
    Glow(795.0f, 300.0f, 150.0f, 220.0f, Hex(0x4A5BFF, 0.050f));

    const float dim = std::max(0.0f, std::min(0.34f,
        (100.0f - state_.brightness) / 100.0f * 0.34f));
    if (dim > 0.001f) Fill(0, 0, Ui::Width, Ui::Height, Hex(0x000000, dim));

    DrawAnimatedPurple();
}

void App::DrawAnimatedPurple() {
    const float speed = state_.animateBg ? anim_ : 0.0f;
    const D2D1_COLOR_F accent = ActiveAccent();

    // Reference-style magenta light remains behind all cards.
    Glow(180.0f + std::sin(speed * 0.62f) * 105.0f, 602.0f,
        300.0f, 105.0f, Hex(0xB521FF, 0.27f));
    Glow(470.0f + std::cos(speed * 0.48f) * 130.0f, 607.0f,
        360.0f, 118.0f, Hex(0xFA22C8, 0.33f));
    Glow(790.0f + std::sin(speed * 0.41f + 1.7f) * 75.0f, 600.0f,
        225.0f, 100.0f, D2D1::ColorF(accent.r, accent.g, accent.b, 0.22f));

    for (int ribbon = 0; ribbon < 2; ++ribbon) {
        const float baseY = 570.0f + ribbon * 13.0f;
        const float phase = speed * (0.66f + ribbon * 0.12f) + ribbon * 1.3f;
        const D2D1_COLOR_F color = ribbon == 0
            ? Hex(0xB936FF, 0.22f)
            : Hex(0xFF2BCB, 0.20f);
        float px = -8.0f;
        float py = baseY;
        for (int i = 1; i <= 92; ++i) {
            const float x = i * 10.0f;
            const float y = baseY + std::sin(i * 0.16f + phase) * (7.0f + ribbon * 2.0f);
            Line(px, py, x, y, color, ribbon == 0 ? 1.5f : 1.1f);
            px = x;
            py = y;
        }
    }

    for (int i = 0; i < 16; ++i) {
        const float phase = speed * (0.45f + (i % 4) * 0.06f) + i * 1.11f;
        const float x = 35.0f + i * 57.0f + std::sin(phase) * 10.0f;
        const float y = 568.0f - std::fmod(speed * (5.5f + (i % 3)) + i * 13.0f, 64.0f);
        Ellipse(x, y, 1.2f + (i % 2) * 0.4f, 1.2f + (i % 2) * 0.4f,
            i % 2 == 0 ? Hex(0xE743FF, 0.48f) : Hex(0x7D63FF, 0.40f));
    }
}

void App::DrawWindowButtons() {
    const float minX = 814.0f;
    const float closeX = 852.0f;
    const bool minHot = Hover(minX, 17, minX + 28, 45);
    const bool closeHot = Hover(closeX, 17, closeX + 28, 45);

    Line(minX + 8, 32, minX + 20, 32,
        minHot ? C::Bright : Hex(0x8290C8), 1.55f);
    Line(closeX + 8, 25, closeX + 20, 37,
        closeHot ? Hex(0xE9C6FF) : Hex(0x8290C8), 1.55f);
    Line(closeX + 20, 25, closeX + 8, 37,
        closeHot ? Hex(0xE9C6FF) : Hex(0x8290C8), 1.55f);
}

void App::DrawNavIcon(int index, float cx, float cy, D2D1_COLOR_F color) {
    // V16: custom vector navigation icons.
    // No Segoe MDL2 glyphs are used here.
    switch (index) {
    case 0: { // AIM: angular pointer arrow + target dot
        const float x = cx - 9.5f;
        const float y = cy - 11.0f;
        Line(x, y, x + 19.0f, y + 10.5f, color, 1.9f);
        Line(x + 19.0f, y + 10.5f, x + 10.3f, y + 12.8f, color, 1.9f);
        Line(x + 10.3f, y + 12.8f, x + 14.8f, y + 21.0f, color, 1.9f);
        Line(x + 14.8f, y + 21.0f, x + 10.7f, y + 22.8f, color, 1.9f);
        Line(x + 10.7f, y + 22.8f, x + 6.5f, y + 14.4f, color, 1.9f);
        Line(x + 6.5f, y + 14.4f, x, y + 20.0f, color, 1.9f);
        Line(x, y + 20.0f, x, y, color, 1.9f);
        Ellipse(cx + 10.5f, cy - 8.5f, 2.1f, 2.1f, color);
        break;
    }
    case 1: { // VISUAL: monitor with an eye inside
        RoundStroke(cx - 12.0f, cy - 9.0f, cx + 12.0f, cy + 7.0f, 3.0f, color, 1.75f);
        Line(cx - 4.0f, cy + 10.0f, cx + 4.0f, cy + 10.0f, color, 1.65f);
        Line(cx, cy + 7.0f, cx, cy + 10.0f, color, 1.65f);
        Line(cx - 7.2f, cy - 1.0f, cx - 3.5f, cy - 4.0f, color, 1.35f);
        Line(cx - 3.5f, cy - 4.0f, cx, cy - 5.0f, color, 1.35f);
        Line(cx, cy - 5.0f, cx + 3.5f, cy - 4.0f, color, 1.35f);
        Line(cx + 3.5f, cy - 4.0f, cx + 7.2f, cy - 1.0f, color, 1.35f);
        Line(cx - 7.2f, cy - 1.0f, cx - 3.5f, cy + 2.0f, color, 1.35f);
        Line(cx - 3.5f, cy + 2.0f, cx, cy + 3.0f, color, 1.35f);
        Line(cx, cy + 3.0f, cx + 3.5f, cy + 2.0f, color, 1.35f);
        Line(cx + 3.5f, cy + 2.0f, cx + 7.2f, cy - 1.0f, color, 1.35f);
        Ellipse(cx, cy - 1.0f, 1.8f, 1.8f, color);
        break;
    }
    case 2: { // MISC: four solid rounded tiles
        Round(cx - 10.5f, cy - 10.5f, cx - 2.0f, cy - 2.0f, 2.2f, color);
        Round(cx + 2.0f, cy - 10.5f, cx + 10.5f, cy - 2.0f, 2.2f, color);
        Round(cx - 10.5f, cy + 2.0f, cx - 2.0f, cy + 10.5f, 2.2f, color);
        Round(cx + 2.0f, cy + 2.0f, cx + 10.5f, cy + 10.5f, 2.2f, color);
        break;
    }
    default: { // SETTINGS: three vertical tuning sliders
        Line(cx - 8.0f, cy - 11.0f, cx - 8.0f, cy + 11.0f, color, 1.65f);
        Line(cx, cy - 11.0f, cx, cy + 11.0f, color, 1.65f);
        Line(cx + 8.0f, cy - 11.0f, cx + 8.0f, cy + 11.0f, color, 1.65f);
        Round(cx - 11.5f, cy - 6.8f, cx - 4.5f, cy - 1.2f, 2.4f, color);
        Round(cx - 3.5f, cy + 1.2f, cx + 3.5f, cy + 6.8f, 2.4f, color);
        Round(cx + 4.5f, cy - 1.8f, cx + 11.5f, cy + 3.8f, 2.4f, color);
        break;
    }
    }
}

void App::DrawHeader() {
    Fill(0, 0, Ui::Width, Ui::HeaderH, Hex(0x102E66, 0.40f));
    Line(18, Ui::HeaderH, Ui::Width - 18, Ui::HeaderH,
        Hex(0x5474B7, 0.24f), 1.0f);

    const wchar_t* icons[] = {
        L"\xE7FC", // controller
        L"\xE890", // eye
        L"\xE8B0", // tool / wand
        L"\xE713"  // settings
    };
    const float centers[] = { 50.0f, 108.0f, 166.0f, 224.0f };

    for (int i = 0; i < 4; ++i) {
        const float cx = centers[i];
        const bool selected = state_.page == i;
        const bool hot = Hover(cx - 19.0f, 14.0f, cx + 19.0f, 54.0f);
        const D2D1_COLOR_F color = selected
            ? Hex(0xE7D9FF)
            : (hot ? Hex(0xC9D0FF) : Hex(0x7789C8));

        if (selected) {
            Glow(cx, 32.0f, 23.0f, 19.0f, Hex(0xA45CFF, 0.15f));
        }
        Text(icons[i], cx - 13.0f, 20.0f, 26.0f, 25.0f,
            color, icon_, DWRITE_TEXT_ALIGNMENT_CENTER);

        if (hot && pressed_) {
            state_.page = i;
            state_.dropdownTarget = false;
            state_.dropdownBone = false;
            state_.dropdownTheme = false;
            state_.dropdownConfig = false;
            colorPickerOpen_ = false;
            activeColor_ = nullptr;
        }
    }

    DrawWindowButtons();
}

void App::DrawPanel(float x, float y, float w, float h,
    const std::wstring& title, const std::wstring& iconText) {
    const float panelAlpha = std::max(0.55f, std::min(0.92f, state_.opacity / 100.0f));
    Round(x, y, x + w, y + h, 5.0f,
        D2D1::ColorF(C::Panel.r, C::Panel.g, C::Panel.b, panelAlpha));
    RoundStroke(x, y, x + w, y + h, 5.0f, C::PanelEdge, 1.0f);

    Text(title, x + 16.0f, y + 10.0f, w - 32.0f, 22.0f,
        Hex(0xD8D1FF), heading_);
    Line(x + 14.0f, y + 38.0f, x + w - 14.0f, y + 38.0f,
        Hex(0x2C4F8C, 0.28f), 1.0f);

    // Thin purple rail seen in the reference card layout.
    Round(x + w - 4.0f, y + 52.0f, x + w - 2.0f, y + h - 18.0f,
        1.0f, Hex(0xB43BFF, 0.72f));
    (void)iconText;
}

void App::DrawToggle(const std::wstring& label, bool& value,
    float x, float y, float width, bool enabled) {
    const float sw = 42.0f;
    const float sh = 18.0f;
    const float sx = x + width - sw;
    const bool hot = enabled && Hover(x, y, x + width, y + 26.0f);

    Text(label, x, y, width - sw - 12.0f, 23.0f,
        enabled ? (value ? C::Text : C::Muted) : C::Muted2, small_);

    float& t = toggleAnim_[label];
    const float targetValue = value ? 1.0f : 0.0f;
    t += (targetValue - t) * 0.23f;

    Round(sx, y + 3.0f, sx + sw, y + 3.0f + sh, sh * 0.5f,
        value ? Hex(0xC9A0FF, 0.96f) : Hex(0x28366B, 0.90f));
    const float knobX = sx + 9.0f + (sw - 18.0f) * t;
    Ellipse(knobX, y + 12.0f, 6.7f, 6.7f,
        value ? Hex(0xF5EFFF) : Hex(0x6876AE));
    if (value) {
        Ellipse(knobX, y + 12.0f, 2.2f, 2.2f, Hex(0xA55AFF));
    }

    if (hot && enabled && pressed_) value = !value;
}

void App::DrawColorToggle(const std::wstring& label, bool& value, COLORREF color,
    float x, float y, float width) {
    const D2D1_COLOR_F swatch = FromColorRef(color);
    const bool hot = Hover(x, y + 3, x + 20, y + 23);

    Round(x, y + 3, x + 20, y + 23, 4, swatch);
    RoundStroke(x, y + 3, x + 20, y + 23, 4,
        hot ? Hex(0xF0D7FF, 0.86f) : Hex(0xE5D4FF, 0.24f),
        hot ? 1.4f : 1.0f);

    DrawToggle(label, value, x + 31, y, width - 31);
}

void App::DrawSlider(const std::wstring& id, const std::wstring& label, float& value,
    float min, float max, float x, float y, float width,
    const std::wstring& suffix, int decimals) {
    wchar_t buffer[64]{};
    if (decimals == 0) swprintf_s(buffer, L"%.0f%s", value, suffix.c_str());
    else if (decimals == 2) swprintf_s(buffer, L"%.2f%s", value, suffix.c_str());
    else swprintf_s(buffer, L"%.1f%s", value, suffix.c_str());

    Text(label, x, y, width - 70.0f, 20.0f, C::Muted, tiny_);
    Text(buffer, x + width - 68.0f, y, 68.0f, 20.0f,
        Hex(0x7588C8), tiny_, DWRITE_TEXT_ALIGNMENT_TRAILING);

    const float barY = y + 27.0f;
    const float barH = 4.0f;
    Round(x, barY, x + width, barY + barH, 2.0f, C::Track);
    const float t = Clamp01((value - min) / (max - min));
    Round(x, barY, x + width * t, barY + barH, 2.0f, Hex(0xAFA0F2));
    const float knobX = x + width * t;
    Ellipse(knobX, barY + 2.0f, 6.0f, 6.0f, Hex(0xF2EEFF));

    const bool hot = Hover(x - 6.0f, barY - 9.0f, x + width + 6.0f, barY + 13.0f);
    if (hot && down_) dragSlider_ = id;
    if (!down_ && dragSlider_ == id) dragSlider_.clear();
    if (dragSlider_ == id) {
        const float nt = Clamp01((static_cast<float>(mouse_.x) - x) / width);
        value = min + nt * (max - min);
    }
}

void App::DrawDropdown(const std::wstring& id, const std::wstring& label,
    const std::vector<std::wstring>& options, int& selected,
    bool& open, float x, float y, float width) {
    Text(label, x, y, width, 18.0f, C::Muted, tiny_);

    // Match the action button dimensions exactly: same width and 36px height.
    const float boxY = y + 21.0f;
    const float boxH = 36.0f;
    const bool hot = Hover(x, boxY, x + width, boxY + boxH);
    const D2D1_COLOR_F fill = hot || open
        ? Hex(0x102556, 0.96f)
        : Hex(0x0C1D48, 0.94f);
    const D2D1_COLOR_F edge = open
        ? Hex(0x9D5FFF, 0.46f)
        : Hex(0x355994, 0.20f);

    // Flat rectangular dropdown: no radius and no width expansion.
    Fill(x, boxY, x + width, boxY + boxH, fill);
    Line(x, boxY, x + width, boxY, edge, 1.0f);
    Line(x, boxY + boxH, x + width, boxY + boxH, edge, 1.0f);
    Line(x, boxY, x, boxY + boxH, edge, 1.0f);
    Line(x + width, boxY, x + width, boxY + boxH, edge, 1.0f);

    Text(options[selected], x + 12.0f, boxY + 4.0f, width - 44.0f, 27.0f,
        C::Muted, small_);
    Text(open ? L"\xE70E" : L"\xE70D", x + width - 31.0f, boxY + 4.0f,
        18.0f, 27.0f, Hex(0x7D8DCA), icon_, DWRITE_TEXT_ALIGNMENT_CENTER);

    if (hot && pressed_) {
        const bool shouldOpen = !open;
        state_.dropdownTarget = false;
        state_.dropdownBone = false;
        state_.dropdownTheme = false;
        state_.dropdownConfig = false;
        open = shouldOpen;
        if (shouldOpen) dropdownOpenedThisFrame_ = id;
    }

    if (open) {
        const float requiredBottom = boxY + boxH + 5.0f
            + static_cast<float>(options.size()) * 34.0f;
        const bool opensUp = requiredBottom > Ui::Height - 10.0f;
        dropdownOverlays_.push_back({ id, options, &selected, &open, x, boxY, width, opensUp });
    }
}

void App::DrawDropdownOverlays() {
    if (dropdownOverlays_.empty()) return;

    const D2D1_COLOR_F accent = ActiveAccent();
    bool clickedInsideAny = false;

    // Rendered after all cards, so the list is always the top-most UI layer.
    for (auto& dropdown : dropdownOverlays_) {
        if (!dropdown.open || !*dropdown.open || !dropdown.selected) continue;

        const int count = static_cast<int>(dropdown.options.size());
        const float rowHeight = 34.0f;
        const float listHeight = static_cast<float>(count) * rowHeight;
        const float margin = 9.0f;
        const float gap = 5.0f;

        float listTop = dropdown.opensUp
            ? dropdown.boxY - gap - listHeight
            : dropdown.boxY + 36.0f + gap;

        const float maxTop = Ui::Height - margin - listHeight;
        listTop = std::max(margin, std::min(listTop, maxTop));
        const float listBottom = listTop + listHeight;

        // Popup uses exactly the same x/width as its base field.
        Glow(dropdown.x + dropdown.width * 0.5f,
            listTop + listHeight * 0.5f,
            dropdown.width * 0.56f,
            listHeight * 0.70f,
            Hex(0x00020A, 0.42f));

        Fill(dropdown.x, listTop,
            dropdown.x + dropdown.width, listBottom,
            Hex(0x050A1E, 1.0f));
        const D2D1_COLOR_F popupEdge =
            D2D1::ColorF(accent.r, accent.g, accent.b, 0.62f);
        Line(dropdown.x, listTop, dropdown.x + dropdown.width, listTop, popupEdge, 1.0f);
        Line(dropdown.x, listBottom, dropdown.x + dropdown.width, listBottom, popupEdge, 1.0f);
        Line(dropdown.x, listTop, dropdown.x, listBottom, popupEdge, 1.0f);
        Line(dropdown.x + dropdown.width, listTop,
            dropdown.x + dropdown.width, listBottom, popupEdge, 1.0f);

        for (int i = 0; i < count; ++i) {
            const float itemY = listTop + static_cast<float>(i) * rowHeight;
            const bool itemHot = Hover(dropdown.x, itemY,
                dropdown.x + dropdown.width, itemY + rowHeight);
            const bool itemSelected = *dropdown.selected == i;

            if (itemSelected) {
                Fill(dropdown.x, itemY,
                    dropdown.x + dropdown.width, itemY + rowHeight,
                    Hex(0x34205E, 1.0f));
            }
            else if (itemHot) {
                Fill(dropdown.x, itemY,
                    dropdown.x + dropdown.width, itemY + rowHeight,
                    Hex(0x1D1740, 1.0f));
            }

            if (i > 0) {
                Line(dropdown.x + 10.0f, itemY,
                    dropdown.x + dropdown.width - 10.0f, itemY,
                    Hex(0x314777, 0.26f), 1.0f);
            }

            Text(dropdown.options[i], dropdown.x + 12.0f, itemY + 3.0f,
                dropdown.width - 40.0f, 28.0f,
                itemSelected ? C::Bright : (itemHot ? C::Text : C::Muted), small_);

            if (itemSelected) {
                Ellipse(dropdown.x + dropdown.width - 17.0f,
                    itemY + rowHeight * 0.5f,
                    2.6f, 2.6f, accent);
            }

            if (itemHot) clickedInsideAny = true;
            if (itemHot && pressed_) {
                *dropdown.selected = i;
                *dropdown.open = false;
                dropdownOpenedThisFrame_.clear();
            }
        }

        const bool baseHot = Hover(dropdown.x, dropdown.boxY,
            dropdown.x + dropdown.width,
            dropdown.boxY + 36.0f);
        if (baseHot) clickedInsideAny = true;
        if (baseHot && pressed_ && dropdownOpenedThisFrame_ != dropdown.id) {
            *dropdown.open = false;
        }
    }

    if (pressed_ && dropdownOpenedThisFrame_.empty() && !clickedInsideAny) {
        state_.dropdownTarget = false;
        state_.dropdownBone = false;
        state_.dropdownTheme = false;
        state_.dropdownConfig = false;
    }
}

void App::DrawActionButton(const std::wstring& text, float x, float y, float width) {
    const bool hot = Hover(x, y, x + width, y + 36.0f);
    Round(x, y, x + width, y + 36.0f, 4.0f,
        hot ? Hex(0x173064, 0.96f) : Hex(0x0D204E, 0.94f));
    RoundStroke(x, y, x + width, y + 36.0f, 4.0f,
        hot ? Hex(0x8A62D8, 0.34f) : Hex(0x34568D, 0.18f));
    Text(text, x, y + 3.0f, width, 29.0f,
        hot ? C::Text : C::Muted, small_, DWRITE_TEXT_ALIGNMENT_CENTER);
}

void App::DrawInfoRow(const std::wstring& left, const std::wstring& right,
    float x, float y, float width) {
    Text(left, x, y, width * 0.55f, 24, C::Muted, small_);
    Text(right, x + width * 0.55f, y, width * 0.45f, 24,
        Hex(0xB48CFF), small_, DWRITE_TEXT_ALIGNMENT_TRAILING);
    Line(x, y + 28, x + width, y + 28, Hex(0x31447B, 0.28f));
}

void App::DrawMetricCard(float x, float y, float w, const std::wstring& title, const std::wstring& value) {
    Round(x, y, x + w, y + 55, 7, Hex(0x0F183D, 0.92f));
    RoundStroke(x, y, x + w, y + 55, 7, Hex(0x4D56A2, 0.28f));
    Text(title, x + 14, y + 6, w - 28, 17, C::Muted, tiny_);
    Text(value, x + 14, y + 22, w - 28, 24, Hex(0xD09BFF), heading_);
}

void App::DrawAimbotPage() {
    const float lx = Ui::Margin;
    const float rx = Ui::Margin + Ui::ColumnW + Ui::Gap;
    const float top = 112.0f;

    DrawPanel(lx, top, Ui::ColumnW, 205.0f, L"GENERAL", L"");
    DrawToggle(L"Checkbox active", state_.enabled,
        lx + 20.0f, top + 57.0f, Ui::ColumnW - 40.0f);
    DrawToggle(L"Checkbox inactive", state_.visibility,
        lx + 20.0f, top + 96.0f, Ui::ColumnW - 40.0f);
    DrawToggle(L"Prediction", state_.prediction,
        lx + 20.0f, top + 135.0f, Ui::ColumnW - 40.0f);

    DrawPanel(lx, 344.0f, Ui::ColumnW, 202.0f, L"MISCELLANEOUS", L"");
    DrawDropdown(L"target", L"COMBOBOX",
        { L"Selection", L"Closest", L"Lowest health" },
        state_.target, state_.dropdownTarget,
        lx + 20.0f, 394.0f, Ui::ColumnW - 40.0f);
    DrawDropdown(L"bone", L"TARGET BONE",
        { L"Head", L"Neck", L"Chest", L"Pelvis" },
        state_.bone, state_.dropdownBone,
        lx + 20.0f, 468.0f, Ui::ColumnW - 40.0f);

    DrawPanel(rx, top, Ui::ColumnW, 434.0f, L"AIMBOT", L"");
    DrawSlider(L"fov", L"Slider Integer", state_.fov, 0.0f, 180.0f,
        rx + 20.0f, top + 58.0f, Ui::ColumnW - 40.0f, L"%", 0);
    DrawSlider(L"smooth", L"Slider Float", state_.smooth, 0.0f, 1.0f,
        rx + 20.0f, top + 116.0f, Ui::ColumnW - 40.0f, L"", 2);
    DrawActionButton(L"Button inactive", rx + 20.0f, top + 177.0f,
        Ui::ColumnW - 40.0f);
    DrawDropdown(L"target-mode", L"TRACKLIST",
        { L"Closest target", L"Visible target", L"Lowest health" },
        state_.target, state_.dropdownConfig,
        rx + 20.0f, top + 232.0f, Ui::ColumnW - 40.0f);
    DrawToggle(L"Humanized movement", state_.smoothing,
        rx + 20.0f, top + 318.0f, Ui::ColumnW - 40.0f);
    DrawToggle(L"Recoil control", state_.recoil,
        rx + 20.0f, top + 357.0f, Ui::ColumnW - 40.0f);
}

void App::DrawVisualsPage() {
    const float lx = Ui::Margin;
    const float rx = Ui::Margin + Ui::ColumnW + Ui::Gap;
    const float top = 116.0f;

    const D2D1_COLOR_F boxColor = FromColorRef(state_.boxColor);
    const D2D1_COLOR_F nameColor = FromColorRef(state_.nameColor);
    const D2D1_COLOR_F healthColor = FromColorRef(state_.healthColor);
    const D2D1_COLOR_F skeletonColor = FromColorRef(state_.skeletonColor);
    const D2D1_COLOR_F glowColor = FromColorRef(state_.glowColor);
    const D2D1_COLOR_F crosshairColor = FromColorRef(state_.crosshairColor);

    DrawPanel(lx, top, Ui::ColumnW, 261, L"PLAYER ESP", L"\xE7C3");
    DrawColorToggle(L"Bounding boxes", state_.boxes, state_.boxColor, lx + 20, top + 58, Ui::ColumnW - 40);
    DrawColorToggle(L"Player names", state_.names, state_.nameColor, lx + 20, top + 95, Ui::ColumnW - 40);
    DrawColorToggle(L"Health bars", state_.health, state_.healthColor, lx + 20, top + 132, Ui::ColumnW - 40);
    DrawColorToggle(L"Skeleton", state_.skeleton, state_.skeletonColor, lx + 20, top + 169, Ui::ColumnW - 40);
    DrawSlider(L"thickness", L"Line thickness", state_.thickness, 1.0f, 5.0f,
        lx + 20, top + 207, Ui::ColumnW - 40, L"px", 1);

    DrawPanel(lx, 397, Ui::ColumnW, 166, L"WORLD", L"\xE7F4");
    DrawColorToggle(L"Radar", state_.radar, state_.radarColor, lx + 20, 447, Ui::ColumnW - 40);
    DrawColorToggle(L"Glow highlights", state_.glow, state_.glowColor, lx + 20, 484, Ui::ColumnW - 40);
    DrawColorToggle(L"Crosshair", state_.crosshair, state_.crosshairColor, lx + 20, 521, Ui::ColumnW - 40);

    DrawPanel(rx, top, Ui::ColumnW, 447, L"LIVE PREVIEW", L"\xE91B");

    const float pvL = rx + 44.0f;
    const float pvT = top + 58.0f;
    const float pvR = rx + 367.0f;
    const float pvB = top + 405.0f;
    Round(pvL, pvT, pvR, pvB, 10, Hex(0x07112B, 0.90f));
    RoundStroke(pvL, pvT, pvR, pvB, 10, Hex(0x6854A8, 0.24f));

    const float centerX = (pvL + pvR) * 0.5f;
    const float headY = pvT + 76.0f;
    const float bodyTop = pvT + 108.0f;
    const float bodyBot = pvT + 220.0f;
    const float boxL = centerX - 66.0f;
    const float boxR = centerX + 66.0f;
    const float boxT = pvT + 48.0f;
    const float boxB = pvT + 250.0f;

    if (state_.glow) {
        Glow(centerX, pvT + 150.0f, 104.0f, 126.0f,
            D2D1::ColorF(glowColor.r, glowColor.g, glowColor.b, 0.16f));
    }

    if (state_.boxes) {
        const float c = 22.0f;
        const float t = state_.thickness;
        Line(boxL, boxT, boxL + c, boxT, boxColor, t);
        Line(boxL, boxT, boxL, boxT + c, boxColor, t);
        Line(boxR - c, boxT, boxR, boxT, boxColor, t);
        Line(boxR, boxT, boxR, boxT + c, boxColor, t);
        Line(boxL, boxB, boxL + c, boxB, boxColor, t);
        Line(boxL, boxB - c, boxL, boxB, boxColor, t);
        Line(boxR - c, boxB, boxR, boxB, boxColor, t);
        Line(boxR, boxB - c, boxR, boxB, boxColor, t);
    }

    // No filled human silhouette: preview stays clean and focuses on ESP elements.

    if (state_.skeleton) {
        // Realistic ESP bone rig: outlined bones, articulated joints,
        // pelvis triangle, feet and a slightly natural asymmetric pose.
        const float boneWidth = std::max(1.35f, state_.thickness * 0.78f);

        const D2D1_POINT_2F head{ centerX, headY };
        const D2D1_POINT_2F neck{ centerX, pvT + 100.0f };
        const D2D1_POINT_2F upperSpine{ centerX, pvT + 126.0f };
        const D2D1_POINT_2F lowerSpine{ centerX, pvT + 157.0f };
        const D2D1_POINT_2F pelvis{ centerX, pvT + 181.0f };

        const D2D1_POINT_2F shoulderL{ centerX - 29.0f, pvT + 113.0f };
        const D2D1_POINT_2F shoulderR{ centerX + 29.0f, pvT + 113.0f };
        const D2D1_POINT_2F elbowL{ centerX - 43.0f, pvT + 148.0f };
        const D2D1_POINT_2F elbowR{ centerX + 46.0f, pvT + 145.0f };
        const D2D1_POINT_2F wristL{ centerX - 38.0f, pvT + 184.0f };
        const D2D1_POINT_2F wristR{ centerX + 51.0f, pvT + 181.0f };
        const D2D1_POINT_2F handL{ centerX - 35.0f, pvT + 194.0f };
        const D2D1_POINT_2F handR{ centerX + 53.0f, pvT + 191.0f };

        const D2D1_POINT_2F hipL{ centerX - 16.0f, pvT + 185.0f };
        const D2D1_POINT_2F hipR{ centerX + 16.0f, pvT + 185.0f };
        const D2D1_POINT_2F kneeL{ centerX - 21.0f, pvT + 218.0f };
        const D2D1_POINT_2F kneeR{ centerX + 20.0f, pvT + 217.0f };
        const D2D1_POINT_2F ankleL{ centerX - 25.0f, pvT + 244.0f };
        const D2D1_POINT_2F ankleR{ centerX + 24.0f, pvT + 244.0f };
        const D2D1_POINT_2F footL{ centerX - 33.0f, pvT + 249.0f };
        const D2D1_POINT_2F footR{ centerX + 33.0f, pvT + 249.0f };

        // Head ring with dark outline.
        Set(Hex(0x030615, 0.96f));
        target_->DrawEllipse(D2D1::Ellipse(head, 15.8f, 18.0f), brush_, boneWidth + 2.6f);
        Set(skeletonColor);
        target_->DrawEllipse(D2D1::Ellipse(head, 15.8f, 18.0f), brush_, boneWidth);

        Bone(centerX, headY + 18.0f, neck.x, neck.y, skeletonColor, boneWidth);
        Bone(neck.x, neck.y, upperSpine.x, upperSpine.y, skeletonColor, boneWidth);
        Bone(upperSpine.x, upperSpine.y, lowerSpine.x, lowerSpine.y, skeletonColor, boneWidth);
        Bone(lowerSpine.x, lowerSpine.y, pelvis.x, pelvis.y, skeletonColor, boneWidth);

        // Clavicles and arms.
        Bone(neck.x, neck.y + 8.0f, shoulderL.x, shoulderL.y, skeletonColor, boneWidth);
        Bone(neck.x, neck.y + 8.0f, shoulderR.x, shoulderR.y, skeletonColor, boneWidth);
        Bone(shoulderL.x, shoulderL.y, elbowL.x, elbowL.y, skeletonColor, boneWidth);
        Bone(elbowL.x, elbowL.y, wristL.x, wristL.y, skeletonColor, boneWidth);
        Bone(wristL.x, wristL.y, handL.x, handL.y, skeletonColor, boneWidth);
        Bone(shoulderR.x, shoulderR.y, elbowR.x, elbowR.y, skeletonColor, boneWidth);
        Bone(elbowR.x, elbowR.y, wristR.x, wristR.y, skeletonColor, boneWidth);
        Bone(wristR.x, wristR.y, handR.x, handR.y, skeletonColor, boneWidth);

        // Pelvis triangle and legs.
        Bone(pelvis.x, pelvis.y, hipL.x, hipL.y, skeletonColor, boneWidth);
        Bone(pelvis.x, pelvis.y, hipR.x, hipR.y, skeletonColor, boneWidth);
        Bone(hipL.x, hipL.y, hipR.x, hipR.y, skeletonColor, boneWidth);
        Bone(hipL.x, hipL.y, kneeL.x, kneeL.y, skeletonColor, boneWidth);
        Bone(kneeL.x, kneeL.y, ankleL.x, ankleL.y, skeletonColor, boneWidth);
        Bone(ankleL.x, ankleL.y, footL.x, footL.y, skeletonColor, boneWidth);
        Bone(hipR.x, hipR.y, kneeR.x, kneeR.y, skeletonColor, boneWidth);
        Bone(kneeR.x, kneeR.y, ankleR.x, ankleR.y, skeletonColor, boneWidth);
        Bone(ankleR.x, ankleR.y, footR.x, footR.y, skeletonColor, boneWidth);

        const D2D1_POINT_2F joints[] = {
            neck, upperSpine, lowerSpine, pelvis,
            shoulderL, shoulderR, elbowL, elbowR, wristL, wristR,
            hipL, hipR, kneeL, kneeR, ankleL, ankleR
        };
        for (const auto& joint : joints) {
            Joint(joint.x, joint.y, skeletonColor, 2.5f);
        }
    }

    if (state_.crosshair) {
        const float cy = pvT + 150.0f;
        Line(centerX - 12.0f, cy, centerX - 3.0f, cy, crosshairColor, 1.25f);
        Line(centerX + 3.0f, cy, centerX + 12.0f, cy, crosshairColor, 1.25f);
        Line(centerX, cy - 12.0f, centerX, cy - 3.0f, crosshairColor, 1.25f);
        Line(centerX, cy + 3.0f, centerX, cy + 12.0f, crosshairColor, 1.25f);
        Ellipse(centerX, cy, 1.5f, 1.5f, crosshairColor);
    }

    if (state_.names) {
        Round(centerX - 54.0f, boxT - 27.0f, centerX + 54.0f, boxT - 6.0f, 6, Hex(0x101A40, 0.88f));
        Text(L"Enemy | 24m", centerX - 54.0f, boxT - 25.0f, 108.0f, 17.0f,
            nameColor, tiny_, DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    const float targetHealth = 0.30f + (std::sin(anim_ * 1.05f) * 0.5f + 0.5f) * 0.64f;
    previewHealth_ += (targetHealth - previewHealth_) * 0.085f;

    if (state_.health) {
        const float hpX = boxL - 12.0f;
        const float hpH = boxB - boxT;
        Round(hpX, boxT, hpX + 6.0f, boxB, 3.0f, Hex(0x202750));
        const float fillH = hpH * Clamp01(previewHealth_);
        Round(hpX, boxB - fillH, hpX + 6.0f, boxB, 3.0f, healthColor);
        Glow(hpX + 3.0f, boxB - fillH, 10.0f, 16.0f,
            D2D1::ColorF(healthColor.r, healthColor.g, healthColor.b, 0.18f));
        wchar_t hpText[16]{};
        swprintf_s(hpText, L"%d", static_cast<int>(previewHealth_ * 100.0f));
        Text(hpText, hpX - 14.0f, boxB - fillH - 9.0f, 22.0f, 14.0f,
            C::Text, tiny_, DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    Round(pvL + 16.0f, pvB - 47.0f, pvL + 120.0f, pvB - 20.0f, 7, Hex(0x101A40, 0.82f));
    Text(L"ESP PREVIEW", pvL + 24.0f, pvB - 43.0f, 88.0f, 18.0f, C::Muted, tiny_, DWRITE_TEXT_ALIGNMENT_CENTER);
}

void App::DrawMiscPage() {
    const float lx = Ui::Margin;
    const float rx = Ui::Margin + Ui::ColumnW + Ui::Gap;

    // Six separate cards. Every last control keeps at least 12 px from the card edge.
    DrawPanel(lx, 116, Ui::ColumnW, 132, L"ESSENTIALS", L"\xE7C1");
    DrawToggle(L"Stream-proof overlay", state_.streamProof, lx + 20, 166, Ui::ColumnW - 40);
    DrawToggle(L"Automatic saving", state_.autoSave, lx + 20, 203, Ui::ColumnW - 40);

    DrawPanel(lx, 263, Ui::ColumnW, 132, L"ALERTS", L"\xEA39");
    DrawToggle(L"Status notifications", state_.notifications, lx + 20, 313, Ui::ColumnW - 40);
    DrawToggle(L"Compact alerts", state_.compactAlerts, lx + 20, 350, Ui::ColumnW - 40);

    DrawPanel(lx, 410, Ui::ColumnW, 153, L"SESSION", L"\xE946");
    DrawInfoRow(L"Profile", selectedConfig_ >= 0 && selectedConfig_ < static_cast<int>(savedConfigs_.size())
        ? savedConfigs_[selectedConfig_] : L"Local", lx + 20, 458, Ui::ColumnW - 40);
    DrawActionButton(L"Unload interface", lx + 20, 510, Ui::ColumnW - 40);

    DrawPanel(rx, 116, Ui::ColumnW, 132, L"OVERLAY", L"\xE91B");
    DrawToggle(L"Menu watermark", state_.watermark, rx + 20, 166, Ui::ColumnW - 40);
    DrawToggle(L"Keybind hints", state_.keyHints, rx + 20, 203, Ui::ColumnW - 40);

    DrawPanel(rx, 263, Ui::ColumnW, 132, L"INPUT", L"\xE765");
    DrawToggle(L"Controller navigation", state_.controllerNavigation, rx + 20, 313, Ui::ColumnW - 40);
    DrawToggle(L"Hit sound feedback", state_.hitSound, rx + 20, 350, Ui::ColumnW - 40);

    DrawPanel(rx, 410, Ui::ColumnW, 153, L"PERFORMANCE", L"\xE7BA");
    DrawToggle(L"Performance mode", state_.performanceMode, rx + 20, 456, Ui::ColumnW - 40);
    DrawToggle(L"Low latency mode", state_.lowLatencyMode, rx + 20, 489, Ui::ColumnW - 40);
    DrawToggle(L"Check for updates", state_.autoUpdate, rx + 20, 522, Ui::ColumnW - 40);
}

void App::DrawSettingsPage() {
    const float lx = Ui::Margin;
    const float rx = Ui::Margin + Ui::ColumnW + Ui::Gap;
    const float top = 116.0f;

    DrawPanel(lx, top, Ui::ColumnW, 300, L"INTERFACE SETTINGS", L"\xE713");
    DrawSlider(L"scale", L"UI scale", state_.uiScale, 80, 120,
        lx + 20, top + 58, Ui::ColumnW - 40, L"%", 0);
    DrawSlider(L"opacity", L"Panel opacity", state_.opacity, 45, 100,
        lx + 20, top + 118, Ui::ColumnW - 40, L"%", 0);
    DrawSlider(L"brightness", L"Background brightness", state_.brightness, 40, 100,
        lx + 20, top + 178, Ui::ColumnW - 40, L"%", 0);
    DrawDropdown(L"theme", L"COLOR THEME",
        { L"Purple neon", L"Deep violet", L"Magenta night" },
        state_.theme, state_.dropdownTheme,
        lx + 20, top + 226, Ui::ColumnW - 40);

    DrawPanel(lx, 431, Ui::ColumnW, 132, L"BEHAVIOR", L"\xE7BA");
    DrawToggle(L"Limit animation FPS", state_.fpsLimit, lx + 20, 480, Ui::ColumnW - 40);
    DrawToggle(L"Animate background", state_.animateBg, lx + 20, 515, Ui::ColumnW - 40);

    DrawPanel(rx, top, Ui::ColumnW, 447, L"CONFIGURATION", L"\xE74E");
    Text(L"CONFIG NAME", rx + 20, top + 49, 130, 18, C::Muted, tiny_);

    const float inputY = top + 68.0f;
    Round(rx + 20, inputY, rx + Ui::ColumnW - 20, inputY + 36, 7,
        configNameFocused_ ? Hex(0x172454, 0.96f) : Hex(0x0E183B, 0.94f));
    RoundStroke(rx + 20, inputY, rx + Ui::ColumnW - 20, inputY + 36, 7,
        configNameFocused_ ? D2D1::ColorF(ActiveAccent().r, ActiveAccent().g, ActiveAccent().b, 0.72f)
        : Hex(0x51488D, 0.36f));
    Text(configName_.empty() ? L"Type a name..." : configName_,
        rx + 34, inputY + 4, Ui::ColumnW - 68, 27,
        configName_.empty() ? C::Muted2 : C::Text, small_);
    if (configNameFocused_ && (static_cast<int>(anim_ * 20.0f) % 2 == 0)) {
        const float caretX = std::min(rx + Ui::ColumnW - 35.0f,
            rx + 36.0f + static_cast<float>(configName_.size()) * 7.2f);
        Line(caretX, inputY + 8, caretX, inputY + 28, ActiveAccent(), 1.2f);
    }

    DrawActionButton(L"Save config", rx + 20, top + 112, Ui::ColumnW - 40);
    Text(configStatus_, rx + 20, top + 156, Ui::ColumnW - 40, 18, C::Muted, tiny_);

    wchar_t countText[48]{};
    swprintf_s(countText, L"SAVED CONFIGS  %d", static_cast<int>(savedConfigs_.size()));
    Text(countText, rx + 20, top + 177, 160, 18, C::Muted, tiny_);

    // Exactly three visible rows. Additional rows use the purple scrollbar.
    const float listY = top + 197.0f;
    const float rowStep = 31.0f;
    const int total = static_cast<int>(savedConfigs_.size());
    const int maxScroll = std::max(0, total - 3);
    configScroll_ = std::max(0, std::min(configScroll_, maxScroll));
    const int visible = std::min(3, std::max(0, total - configScroll_));

    Round(rx + 20, listY - 3, rx + Ui::ColumnW - 20, listY + 93, 8, Hex(0x08122E, 0.62f));
    RoundStroke(rx + 20, listY - 3, rx + Ui::ColumnW - 20, listY + 93, 8, Hex(0x4A3D82, 0.22f));

    if (total == 0) {
        Text(L"No saved configurations", rx + 34, listY + 27, Ui::ColumnW - 78, 27, C::Muted2, small_);
    }

    for (int row = 0; row < visible; ++row) {
        const int index = configScroll_ + row;
        const float rowY = listY + row * rowStep;
        const bool selected = selectedConfig_ == index;
        const bool hot = Hover(rx + 24, rowY, rx + Ui::ColumnW - 38, rowY + 27);
        Round(rx + 24, rowY, rx + Ui::ColumnW - 38, rowY + 27, 6,
            selected ? Hex(0x43236F, 0.92f) : (hot ? Hex(0x182454, 0.90f) : Hex(0x0C1535, 0.78f)));
        RoundStroke(rx + 24, rowY, rx + Ui::ColumnW - 38, rowY + 27, 6,
            selected ? D2D1::ColorF(ActiveAccent().r, ActiveAccent().g, ActiveAccent().b, 0.70f)
            : Hex(0x4B4384, 0.22f));
        Text(savedConfigs_[index], rx + 36, rowY, Ui::ColumnW - 116, 26,
            selected ? C::Bright : C::Text, small_);
        if (selected) {
            Ellipse(rx + Ui::ColumnW - 54, rowY + 13.5f, 3.2f, 3.2f, ActiveAccent());
        }
    }

    const float trackX = rx + Ui::ColumnW - 30.0f;
    const float trackY = listY;
    const float trackH = 89.0f;
    Round(trackX, trackY, trackX + 6.0f, trackY + trackH, 3.0f, Hex(0x252653, 0.82f));
    if (total > 3) {
        const float thumbH = std::max(24.0f, trackH * (3.0f / static_cast<float>(total)));
        const float thumbY = trackY + (trackH - thumbH) *
            (static_cast<float>(configScroll_) / static_cast<float>(maxScroll));
        Glow(trackX + 3.0f, thumbY + thumbH * 0.5f, 15.0f, thumbH * 0.8f,
            D2D1::ColorF(ActiveAccent().r, ActiveAccent().g, ActiveAccent().b, 0.18f));
        Round(trackX, thumbY, trackX + 6.0f, thumbY + thumbH, 3.0f, ActiveAccent());
    }
    else {
        Round(trackX, trackY, trackX + 6.0f, trackY + trackH, 3.0f,
            D2D1::ColorF(ActiveAccent().r, ActiveAccent().g, ActiveAccent().b, 0.34f));
    }

    const float half = (Ui::ColumnW - 52) * 0.5f;
    DrawActionButton(L"Load", rx + 20, top + 298, half);
    DrawActionButton(L"Delete", rx + 32 + half, top + 298, half);
    DrawActionButton(L"Restore defaults", rx + 20, top + 345, Ui::ColumnW - 40);
}

void App::DrawColorPicker() {
    if (!colorPickerOpen_ || !activeColor_) return;

    const float px = 230.0f;
    const float py = 105.0f;
    const float pw = 440.0f;
    const float ph = 334.0f;
    const float squareX = px + 24.0f;
    const float squareY = py + 62.0f;
    const float squareW = 392.0f;
    const float squareH = 176.0f;
    const float hueX = squareX;
    const float hueY = squareY + squareH + 18.0f;
    const float hueW = squareW;
    const float hueH = 18.0f;

    Glow(px + pw * 0.5f, py + ph * 0.5f, 255.0f, 200.0f, Hex(0x7D35FF, 0.15f));
    Round(px, py, px + pw, py + ph, 14, Hex(0x07102A, 0.995f));
    RoundStroke(px, py, px + pw, py + ph, 14, Hex(0x9A60E8, 0.64f), 1.2f);
    Text(L"CUSTOM RGB COLOR", px + 22, py + 12, 220, 28, C::Bright, heading_);
    Text(L"Drag inside the rectangle, then use the hue strip", px + 22, py + 36, 330, 18, C::Muted, tiny_);

    const bool closeHot = Hover(px + pw - 38, py + 12, px + pw - 14, py + 36);
    Line(px + pw - 33, py + 17, px + pw - 19, py + 31,
        closeHot ? C::Bright : C::Muted, 1.7f);
    Line(px + pw - 19, py + 17, px + pw - 33, py + 31,
        closeHot ? C::Bright : C::Muted, 1.7f);

    const D2D1_COLOR_F hueColor = FromColorRef(HsvToColorRef(pickerHue_, 1.0f, 1.0f));

    D2D1_GRADIENT_STOP satStops[] = {
        {0.0f, Hex(0xFFFFFF)},
        {1.0f, hueColor}
    };
    ID2D1GradientStopCollection* satCollection = nullptr;
    ID2D1LinearGradientBrush* satBrush = nullptr;
    if (SUCCEEDED(target_->CreateGradientStopCollection(satStops, 2, &satCollection))) {
        target_->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(squareX, squareY), D2D1::Point2F(squareX + squareW, squareY)),
            satCollection, &satBrush);
    }
    if (satBrush) {
        target_->FillRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(squareX, squareY, squareX + squareW, squareY + squareH), 6, 6),
            satBrush);
    }
    SafeRelease(satBrush);
    SafeRelease(satCollection);

    D2D1_GRADIENT_STOP valStops[] = {
        {0.0f, Hex(0x000000, 0.0f)},
        {1.0f, Hex(0x000000, 1.0f)}
    };
    ID2D1GradientStopCollection* valCollection = nullptr;
    ID2D1LinearGradientBrush* valBrush = nullptr;
    if (SUCCEEDED(target_->CreateGradientStopCollection(valStops, 2, &valCollection))) {
        target_->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(squareX, squareY), D2D1::Point2F(squareX, squareY + squareH)),
            valCollection, &valBrush);
    }
    if (valBrush) {
        target_->FillRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(squareX, squareY, squareX + squareW, squareY + squareH), 6, 6),
            valBrush);
    }
    SafeRelease(valBrush);
    SafeRelease(valCollection);
    RoundStroke(squareX, squareY, squareX + squareW, squareY + squareH, 6, Hex(0xE5D8FF, 0.30f));

    const D2D1_COLOR_F hueStops[] = {
        Hex(0xFF0000), Hex(0xFFFF00), Hex(0x00FF00), Hex(0x00FFFF),
        Hex(0x0000FF), Hex(0xFF00FF), Hex(0xFF0000)
    };
    for (int i = 0; i < 6; ++i) {
        D2D1_GRADIENT_STOP stops[] = { {0.0f, hueStops[i]}, {1.0f, hueStops[i + 1]} };
        ID2D1GradientStopCollection* collection = nullptr;
        ID2D1LinearGradientBrush* hueBrush = nullptr;
        const float x1 = hueX + hueW * (i / 6.0f);
        const float x2 = hueX + hueW * ((i + 1) / 6.0f);
        if (SUCCEEDED(target_->CreateGradientStopCollection(stops, 2, &collection))) {
            target_->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties(D2D1::Point2F(x1, hueY), D2D1::Point2F(x2, hueY)),
                collection, &hueBrush);
        }
        if (hueBrush) target_->FillRectangle(D2D1::RectF(x1, hueY, x2, hueY + hueH), hueBrush);
        SafeRelease(hueBrush);
        SafeRelease(collection);
    }
    RoundStroke(hueX, hueY, hueX + hueW, hueY + hueH, 4, Hex(0xE5D8FF, 0.34f));

    const float markerX = squareX + pickerSat_ * squareW;
    const float markerY = squareY + (1.0f - pickerVal_) * squareH;
    Ellipse(markerX, markerY, 7.0f, 7.0f, Hex(0xFFFFFF, 0.96f));
    Ellipse(markerX, markerY, 4.0f, 4.0f, Hex(0x101020, 0.92f));

    const float hueMarkerX = hueX + pickerHue_ * hueW;
    Line(hueMarkerX, hueY - 4, hueMarkerX, hueY + hueH + 4, Hex(0xFFFFFF, 0.96f), 1.7f);

    const COLORREF current = *activeColor_;
    Round(px + 24, py + 286, px + 84, py + 316, 7, FromColorRef(current));
    RoundStroke(px + 24, py + 286, px + 84, py + 316, 7, Hex(0xE7D9FF, 0.38f));
    wchar_t rgbText[96]{};
    swprintf_s(rgbText, L"R %d     G %d     B %d", GetRValue(current), GetGValue(current), GetBValue(current));
    Text(rgbText, px + 100, py + 286, 270, 30, C::Text, small_);
}

void App::Render() {
    if (FAILED(CreateDevice())) return;

    target_->BeginDraw();
    const float renderScale = std::max(0.80f, std::min(1.20f, state_.uiScale / 100.0f));
    target_->SetTransform(D2D1::Matrix3x2F::Scale(renderScale, renderScale));
    DrawBackground();
    DrawHeader();

    dropdownOverlays_.clear();
    dropdownOpenedThisFrame_.clear();
    const bool dropdownWasOpen = state_.dropdownTarget || state_.dropdownBone ||
        state_.dropdownTheme || state_.dropdownConfig;
    const bool savedPressed = pressed_;
    if (dropdownWasOpen) pressed_ = false;

    switch (state_.page) {
    case 0: DrawAimbotPage(); break;
    case 1: DrawVisualsPage(); break;
    case 2: DrawMiscPage(); break;
    default: DrawSettingsPage(); break;
    }

    pressed_ = savedPressed;
    DrawColorPicker();
    DrawDropdownOverlays();
    target_->SetTransform(D2D1::Matrix3x2F::Identity());

    const HRESULT hr = target_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) DiscardDevice();
    pressed_ = false;
}

LRESULT CALLBACK App::StaticProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    App* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<App*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }
    else {
        self = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    return self ? self->Proc(hwnd, msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT App::Proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_TIMER:
        ApplyUiScale();
        anim_ += state_.fpsLimit ? 0.021f : 0.032f;
        if (anim_ > 10000.0f) anim_ = 0.0f;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_MOUSEMOVE: {
        const float inputScale = std::max(0.80f, std::min(1.20f, state_.uiScale / 100.0f));
        mouse_.x = static_cast<LONG>(std::round(GET_X_LPARAM(lp) / inputScale));
        mouse_.y = static_cast<LONG>(std::round(GET_Y_LPARAM(lp) / inputScale));

        if (draggingConfigScrollbar_) {
            const float rx = Ui::Margin + Ui::ColumnW + Ui::Gap;
            const float trackY = 116.0f + 197.0f;
            const float trackH = 89.0f;
            const int total = static_cast<int>(savedConfigs_.size());
            const int maxScroll = std::max(0, total - 3);
            if (maxScroll > 0) {
                const float thumbH = std::max(24.0f, trackH * (3.0f / static_cast<float>(total)));
                const float usable = trackH - thumbH;
                const float thumbY = std::max(trackY, std::min(trackY + usable,
                    static_cast<float>(mouse_.y) - configScrollGrab_));
                configScroll_ = static_cast<int>(std::round((thumbY - trackY) / usable * maxScroll));
            }
        }

        if (draggingPickerSquare_ || draggingPickerHue_) UpdatePickerFromMouse();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        const float inputScale = std::max(0.80f, std::min(1.20f, state_.uiScale / 100.0f));
        mouse_.x = static_cast<LONG>(std::round(GET_X_LPARAM(lp) / inputScale));
        mouse_.y = static_cast<LONG>(std::round(GET_Y_LPARAM(lp) / inputScale));

        if (colorPickerOpen_) {
            const float px = 230.0f;
            const float py = 105.0f;
            const float pw = 440.0f;
            const float ph = 334.0f;
            const float squareX = px + 24.0f;
            const float squareY = py + 62.0f;
            const float squareW = 392.0f;
            const float squareH = 176.0f;
            const float hueX = squareX;
            const float hueY = squareY + squareH + 18.0f;
            const float hueW = squareW;
            const float hueH = 18.0f;

            if (Hover(px + pw - 38, py + 12, px + pw - 14, py + 36)) {
                colorPickerOpen_ = false;
                activeColor_ = nullptr;
                return 0;
            }
            if (Hover(squareX, squareY, squareX + squareW, squareY + squareH)) {
                draggingPickerSquare_ = true;
                SetCapture(hwnd);
                UpdatePickerFromMouse();
                return 0;
            }
            if (Hover(hueX, hueY, hueX + hueW, hueY + hueH)) {
                draggingPickerHue_ = true;
                SetCapture(hwnd);
                UpdatePickerFromMouse();
                return 0;
            }
            if (!Hover(px, py, px + pw, py + ph)) {
                colorPickerOpen_ = false;
                activeColor_ = nullptr;
            }
            return 0;
        }

        // Handle navbar clicks directly in the message handler. This prevents
        // the Settings icon from being mistaken for a title-bar drag region.
        const float navCenters[] = { 50.0f, 108.0f, 166.0f, 224.0f };
        for (int i = 0; i < 4; ++i) {
            if (Hover(navCenters[i] - 22.0f, 10.0f,
                navCenters[i] + 22.0f, 62.0f)) {
                state_.page = i;
                state_.dropdownTarget = false;
                state_.dropdownBone = false;
                state_.dropdownTheme = false;
                state_.dropdownConfig = false;
                colorPickerOpen_ = false;
                activeColor_ = nullptr;
                configNameFocused_ = false;
                down_ = false;
                pressed_ = false;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
        }

        if (state_.page == 1) {
            const float sx = Ui::Margin + 20.0f;
            struct ColorHit { float y; COLORREF* color; };
            ColorHit hits[] = {
                {116.0f + 58.0f, &state_.boxColor},
                {116.0f + 95.0f, &state_.nameColor},
                {116.0f + 132.0f, &state_.healthColor},
                {116.0f + 169.0f, &state_.skeletonColor},
                {447.0f, &state_.radarColor},
                {484.0f, &state_.glowColor},
                {521.0f, &state_.crosshairColor}
            };
            for (auto& hit : hits) {
                if (Hover(sx, hit.y + 3.0f, sx + 20.0f, hit.y + 23.0f)) {
                    OpenColorPicker(hit.color);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
            }
        }

        if (state_.page == 3) {
            const float rx = Ui::Margin + Ui::ColumnW + Ui::Gap;
            const float top = 116.0f;
            const float inputY = top + 68.0f;
            configNameFocused_ = Hover(rx + 20, inputY, rx + Ui::ColumnW - 20, inputY + 36);

            if (Hover(rx + 20, top + 112, rx + Ui::ColumnW - 20, top + 152)) {
                SaveConfig(configName_);
                configNameFocused_ = false;
                return 0;
            }

            const float listY = top + 197.0f;
            const float rowStep = 31.0f;
            const int total = static_cast<int>(savedConfigs_.size());
            const int maxScroll = std::max(0, total - 3);
            configScroll_ = std::max(0, std::min(configScroll_, maxScroll));
            const int visible = std::min(3, std::max(0, total - configScroll_));

            for (int row = 0; row < visible; ++row) {
                const int index = configScroll_ + row;
                const float rowY = listY + row * rowStep;
                if (Hover(rx + 24, rowY, rx + Ui::ColumnW - 36, rowY + 27)) {
                    selectedConfig_ = index;
                    configName_ = savedConfigs_[index];
                    configStatus_ = L"Selected " + savedConfigs_[index];
                    configNameFocused_ = false;
                    return 0;
                }
            }

            const float trackX = rx + Ui::ColumnW - 30.0f;
            const float trackY = listY;
            const float trackH = 89.0f;
            if (total > 3 && Hover(trackX - 5.0f, trackY, trackX + 10.0f, trackY + trackH)) {
                const float thumbH = std::max(24.0f, trackH * (3.0f / static_cast<float>(total)));
                const float thumbY = trackY + (trackH - thumbH) *
                    (static_cast<float>(configScroll_) / static_cast<float>(maxScroll));
                if (Hover(trackX - 5.0f, thumbY, trackX + 10.0f, thumbY + thumbH)) {
                    draggingConfigScrollbar_ = true;
                    configScrollGrab_ = static_cast<float>(mouse_.y) - thumbY;
                    SetCapture(hwnd);
                }
                else {
                    const float usable = trackH - thumbH;
                    const float newThumbY = std::max(trackY, std::min(trackY + usable,
                        static_cast<float>(mouse_.y) - thumbH * 0.5f));
                    configScroll_ = static_cast<int>(std::round((newThumbY - trackY) / usable * maxScroll));
                }
                return 0;
            }

            const float half = (Ui::ColumnW - 52) * 0.5f;
            if (Hover(rx + 20, top + 298, rx + 20 + half, top + 338)) {
                if (selectedConfig_ >= 0 && selectedConfig_ < static_cast<int>(savedConfigs_.size()))
                    LoadConfig(savedConfigs_[selectedConfig_]);
                else configStatus_ = L"Select a saved config first.";
                return 0;
            }
            if (Hover(rx + 32 + half, top + 298, rx + 32 + half + half, top + 338)) {
                if (selectedConfig_ >= 0 && selectedConfig_ < static_cast<int>(savedConfigs_.size()))
                    DeleteConfig(savedConfigs_[selectedConfig_]);
                else configStatus_ = L"Select a saved config first.";
                return 0;
            }
            if (Hover(rx + 20, top + 345, rx + Ui::ColumnW - 20, top + 385)) {
                ResetDefaults();
                return 0;
            }
        }
        else {
            configNameFocused_ = false;
        }

        if (Hover(814, 16, 844, 46)) {
            ShowWindow(hwnd, SW_MINIMIZE);
            return 0;
        }
        if (Hover(852, 16, 882, 46)) {
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
            return 0;
        }

        const bool overTabs = Hover(24, 10, 264, 66);
        const bool overButtons = Hover(805, 10, 890, 52);
        if (mouse_.y < static_cast<LONG>(Ui::HeaderH) && !overTabs && !overButtons) {
            ReleaseCapture();
            SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            return 0;
        }

        down_ = true;
        pressed_ = true;
        SetCapture(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONUP:
        down_ = false;
        dragSlider_.clear();
        draggingPickerSquare_ = false;
        draggingPickerHue_ = false;
        draggingConfigScrollbar_ = false;
        ReleaseCapture();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_CHAR:
        if (configNameFocused_) {
            if (wp == 8) {
                if (!configName_.empty()) configName_.pop_back();
            }
            else if (wp == 13) {
                SaveConfig(configName_);
                configNameFocused_ = false;
            }
            else if (wp >= 32 && wp != 127 && configName_.size() < 28) {
                configName_ += static_cast<wchar_t>(wp);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;

    case WM_MOUSEWHEEL: {
        if (state_.page == 3 && savedConfigs_.size() > 3) {
            POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            ScreenToClient(hwnd, &pt);
            const float inputScale = std::max(0.80f, std::min(1.20f, state_.uiScale / 100.0f));
            const float mx = pt.x / inputScale;
            const float my = pt.y / inputScale;
            const float rx = Ui::Margin + Ui::ColumnW + Ui::Gap;
            const float listY = 116.0f + 197.0f;
            if (Inside(mx, my, rx + 18, listY - 4, rx + Ui::ColumnW - 18, listY + 96)) {
                const int direction = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? -1 : 1;
                const int maxScroll = std::max(0, static_cast<int>(savedConfigs_.size()) - 3);
                configScroll_ = std::max(0, std::min(maxScroll, configScroll_ + direction));
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
        }
        break;
    }

    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) {
            if (colorPickerOpen_) {
                colorPickerOpen_ = false;
                activeColor_ = nullptr;
            }
            else if (configNameFocused_) {
                configNameFocused_ = false;
            }
            else {
                PostMessageW(hwnd, WM_CLOSE, 0, 0);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps{};
        BeginPaint(hwnd, &ps);
        Render();
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_SIZE:
        if (target_) target_->Resize(D2D1::SizeU(LOWORD(lp), HIWORD(lp)));
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    App app;
    if (!app.Init(instance)) return 1;
    const int code = app.Run();
    app.Shutdown();
    return code;
}
