// CustomTealTerminal.cpp
// Single-file C++17 Win32 + Direct2D custom CMD terminal.
// Build: Visual Studio x64, Unicode, Windows subsystem.
//
// VIDEO/PROJECT CUSTOMIZATION:
// Search for "CUSTOMIZE YOUR TERMINAL" near the top of this file.
// Viewers can change the complete design without editing rendering logic.
// Prompt layout: [vector home icon] [full directory] ~ [editable command].
// Type "help" for a beginner-friendly guide or "help native" for CMD help.

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
#include <shellapi.h>
#include <shlobj.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

namespace
{
    std::wstring gStartupDirectoryOverride;

    // ========================================================================
    //                         CUSTOMIZE YOUR TERMINAL
    // Edit only the values in this section for your own project or video.
    // Colors use hexadecimal RGB: 0xRRGGBB.
    // ========================================================================

    namespace Customize
    {
        // Window
        constexpr int WindowWidth = 1060;
        constexpr int WindowHeight = 620;
        constexpr int TitleBarHeight = 32;
        constexpr int WindowCornerRadius = 18;
        constexpr BYTE WindowOpacity = 238; // 0 = invisible, 255 = solid

        // Branding
        constexpr const wchar_t* AppTitle = L"Maotaw Terminal";
        constexpr const wchar_t* TrayTooltip = L"Maotaw Terminal";

        // Starting folder. Environment variables such as %USERPROFILE% work.
        constexpr const wchar_t* StartFolder = L"%USERPROFILE%\\Desktop";

        // Fonts
        constexpr const wchar_t* TerminalFont = L"Cascadia Mono";
        constexpr const wchar_t* UiFont = L"Segoe UI";
        constexpr float TerminalFontSize = 13.0f;
        constexpr float TitleFontSize = 13.0f;
        constexpr float SmallFontSize = 10.0f;

        // Main theme
        constexpr unsigned int BackgroundColor = 0x020303;
        constexpr unsigned int TitleBarColor = 0x111315;
        constexpr unsigned int AccentColor = 0xB8D900;
        constexpr unsigned int DirectoryColor = 0xC9EC1A;
        constexpr unsigned int MutedTextColor = 0x819900;
        constexpr unsigned int ErrorTextColor = 0xE6D94C;
        constexpr unsigned int SelectionColor = 0x465600;
        constexpr unsigned int BorderColor = 0x3A3E43;

        // Prompt
        constexpr bool ShowHomeIcon = true;
        constexpr const wchar_t* PromptSeparator = L"~";
        constexpr float PromptSpacing = 9.0f;

        // Scrollbar
        constexpr float ScrollbarWidth = 4.0f;
        constexpr unsigned int ScrollbarTrackColor = 0x171A0D;
        constexpr unsigned int ScrollbarThumbColor = 0x829900;
        constexpr unsigned int ScrollbarActiveColor = 0xD0F000;

        // Behavior
        constexpr bool MinimizeToTray = true;
        constexpr bool ShowTrayIcon = true;
        constexpr int CommandHistoryLimit = 500;
        constexpr int ScrollWheelRows = 3;
        constexpr int ExtraRowsBelowPrompt = 8;

        // Built-in editor
        constexpr unsigned int EditorLineNumberColor = 0x667000;
        constexpr unsigned int EditorKeywordColor = 0xD28CFF;
        constexpr unsigned int EditorFunctionColor = 0x58DCE8;
        constexpr unsigned int EditorStringColor = 0xE5C07B;
        constexpr unsigned int EditorCommentColor = 0x6A9955;
        constexpr unsigned int EditorNumberColor = 0xF39C6B;
        constexpr unsigned int EditorStatusColor = 0xB8D900;
        constexpr int EditorTabSize = 4;
    }

    constexpr int kWindowWidth = Customize::WindowWidth;
    constexpr int kWindowHeight = Customize::WindowHeight;
    constexpr int kTitleHeight = Customize::TitleBarHeight;
    constexpr int kInputHeight = 0;
    constexpr int kPadding = 10;
    constexpr int kTimerId = 1;
    constexpr UINT WM_TERMINAL_OUTPUT = WM_APP + 1;
    constexpr UINT WM_TRAYICON = WM_APP + 3;
    constexpr UINT ID_TRAY_OPEN = 5001;
    constexpr UINT ID_TRAY_EXIT = 5002;

    template <typename T>
    void SafeRelease(T*& value)
    {
        if (value)
        {
            value->Release();
            value = nullptr;
        }
    }

    D2D1_COLOR_F Color(unsigned int rgb, float alpha = 1.0f)
    {
        return D2D1::ColorF(
            ((rgb >> 16) & 0xFF) / 255.0f,
            ((rgb >> 8) & 0xFF) / 255.0f,
            (rgb & 0xFF) / 255.0f,
            alpha);
    }

    std::wstring Utf8ToWide(const std::string& input)
    {
        if (input.empty()) return {};

        int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            input.data(), static_cast<int>(input.size()), nullptr, 0);

        if (count <= 0)
        {
            count = MultiByteToWideChar(CP_ACP, 0,
                input.data(), static_cast<int>(input.size()), nullptr, 0);
            if (count <= 0) return {};

            std::wstring result(static_cast<size_t>(count), L'\0');
            MultiByteToWideChar(CP_ACP, 0, input.data(),
                static_cast<int>(input.size()), result.data(), count);
            return result;
        }

        std::wstring result(static_cast<size_t>(count), L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(),
            static_cast<int>(input.size()), result.data(), count);
        return result;
    }

    std::string WideToUtf8(const std::wstring& input)
    {
        if (input.empty()) return {};

        int count = WideCharToMultiByte(CP_UTF8, 0,
            input.data(), static_cast<int>(input.size()),
            nullptr, 0, nullptr, nullptr);
        if (count <= 0) return {};

        std::string result(static_cast<size_t>(count), '\0');
        WideCharToMultiByte(CP_UTF8, 0,
            input.data(), static_cast<int>(input.size()),
            result.data(), count, nullptr, nullptr);
        return result;
    }

    std::vector<std::wstring> SplitLines(const std::wstring& text)
    {
        std::vector<std::wstring> result;
        std::wstring current;

        for (wchar_t ch : text)
        {
            if (ch == L'\r') continue;
            if (ch == L'\n')
            {
                result.push_back(current);
                current.clear();
            }
            else
            {
                current.push_back(ch);
            }
        }

        if (!current.empty()) result.push_back(current);
        return result;
    }

    std::wstring ResolveStartDirectory()
    {
        if (!gStartupDirectoryOverride.empty())
        {
            std::error_code overrideError;
            std::filesystem::path overridePath =
                std::filesystem::path(gStartupDirectoryOverride).lexically_normal();

            if (
                std::filesystem::exists(overridePath, overrideError) &&
                std::filesystem::is_directory(overridePath, overrideError))
            {
                return overridePath.wstring();
            }
        }

        wchar_t expanded[32768]{};

        const DWORD expandedLength = ExpandEnvironmentStringsW(
            Customize::StartFolder,
            expanded,
            static_cast<DWORD>(std::size(expanded)));

        std::filesystem::path requested =
            expandedLength > 0 && expandedLength < std::size(expanded)
            ? std::filesystem::path(expanded)
            : std::filesystem::path(Customize::StartFolder);

        std::error_code error;

        if (
            !requested.empty() &&
            std::filesystem::exists(requested, error) &&
            std::filesystem::is_directory(requested, error))
        {
            return requested.wstring();
        }

        wchar_t current[32768]{};

        if (GetCurrentDirectoryW(
            static_cast<DWORD>(std::size(current)),
            current) > 0)
        {
            return current;
        }

        return L"C:\\";
    }
}

class App
{
public:
    bool Initialize(HINSTANCE instance);
    int Run();
    void Shutdown();

private:
    HWND hwnd_ = nullptr;
    HICON appIcon_ = nullptr;
    NOTIFYICONDATAW trayIcon_{};
    bool trayAdded_ = false;
    bool exiting_ = false;

    ID2D1Factory* d2dFactory_ = nullptr;
    ID2D1HwndRenderTarget* target_ = nullptr;
    ID2D1SolidColorBrush* brush_ = nullptr;
    IDWriteFactory* writeFactory_ = nullptr;
    IDWriteTextFormat* titleFont_ = nullptr;
    IDWriteTextFormat* terminalFont_ = nullptr;
    IDWriteTextFormat* smallFont_ = nullptr;

    HANDLE childProcess_ = nullptr;
    HANDLE childThread_ = nullptr;
    HANDLE childStdinWrite_ = nullptr;
    HANDLE childStdoutRead_ = nullptr;
    std::thread readerThread_;
    std::atomic<bool> readerRunning_{ false };

    std::mutex outputMutex_;
    std::wstring pendingOutput_;

    std::deque<std::wstring> lines_;
    struct InputSnapshot
    {
        std::wstring text;
        std::size_t caret = 0;
        std::size_t selectionAnchor = 0;
        bool selecting = false;
    };

    std::wstring input_;
    std::size_t caretPosition_ = 0;
    std::size_t selectionAnchor_ = 0;
    bool selecting_ = false;

    std::vector<InputSnapshot> undoStack_;
    bool restoringUndo_ = false;

    std::wstring cwd_ = ResolveStartDirectory();

    std::wstring detectedPythonCommand_;
    std::wstring detectedCargoCommand_;
    std::wstring detectedGitCommand_;
    std::wstring detectedNodeCommand_;
    std::vector<std::wstring> history_;
    int historyIndex_ = -1;
    int scrollOffset_ = 0;
    int bottomOverscroll_ = 0;
    int visibleRows_ = 1;
    int virtualRows_ = 1;
    int normalBottomStart_ = 0;
    int viewStartRow_ = 0;
    int maxViewStart_ = 0;

    bool scrollbarDragging_ = false;
    float scrollbarDragOffset_ = 0.0f;

    struct OutputPoint
    {
        int row = -1;
        std::size_t column = 0;
    };

    bool outputSelecting_ = false;
    bool outputHasSelection_ = false;
    OutputPoint outputSelectionAnchor_{};
    OutputPoint outputSelectionCurrent_{};

    int outputFirstVisibleRow_ = 0;
    int outputVisibleRows_ = 1;
    float outputTop_ = 0.0f;
    float outputBottom_ = 0.0f;
    float outputLineHeight_ = 20.0f;
    float outputTextLeft_ = 17.0f;

    bool floatingPromptVisible_ = false;

    bool contextMenuOpen_ = false;
    bool contextMenuCopyMode_ = false;
    float contextMenuX_ = 0.0f;
    float contextMenuY_ = 0.0f;
    float contextMenuWidth_ = 92.0f;
    float contextMenuHeight_ = 28.0f;
    float scrollbarTrackTop_ = 0.0f;
    float scrollbarTrackBottom_ = 0.0f;
    float scrollbarThumbTop_ = 0.0f;
    float scrollbarThumbBottom_ = 0.0f;

    bool commandRunning_ = false;
    bool waitingForDirectory_ = false;
    bool caretVisible_ = true;
    std::wstring windowTitle_ = Customize::AppTitle;
    D2D1_COLOR_F terminalForeground_ = Color(Customize::AccentColor);
    D2D1_COLOR_F terminalBackground_ = Color(Customize::BackgroundColor);

    // Built-in editor state.
    bool editorMode_ = false;
    bool editorDirty_ = false;
    std::filesystem::path editorPath_;
    std::vector<std::wstring> editorLines_{ L"" };
    struct EditorPoint
    {
        std::size_t row = 0;
        std::size_t column = 0;
    };

    std::size_t editorRow_ = 0;
    std::size_t editorColumn_ = 0;
    int editorScrollRow_ = 0;
    int editorScrollColumn_ = 0;
    std::wstring editorStatus_;

    bool editorSelecting_ = false;
    bool editorHasSelection_ = false;
    EditorPoint editorSelectionAnchor_{};
    EditorPoint editorSelectionCurrent_{};

    float editorTextLeft_ = 0.0f;
    float editorTextTop_ = 0.0f;
    float editorLineHeight_ = 20.0f;
    float editorTextBottom_ = 0.0f;
    int editorVisibleRows_ = 1;

    POINT mouse_{ -1, -1 };

    static LRESULT CALLBACK StaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT Proc(UINT message, WPARAM wParam, LPARAM lParam);

    bool CreateFactories();
    bool CreateResources();
    void DiscardResources();
    void Render();

    void SetBrush(const D2D1_COLOR_F& color);
    void FillRect(float l, float t, float r, float b, const D2D1_COLOR_F& color);
    void FillRound(float l, float t, float r, float b, float radius, const D2D1_COLOR_F& color);
    void DrawRound(float l, float t, float r, float b, float radius, const D2D1_COLOR_F& color, float stroke = 1.0f);
    void DrawLine(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, float stroke = 1.0f);
    void FillPowerSegment(float x, float y, float w, float h, const D2D1_COLOR_F& color, bool leftNotch, bool rightPoint);
    void DrawTextLine(const std::wstring& text, float l, float t, float r, float b,
        const D2D1_COLOR_F& color, IDWriteTextFormat* format,
        DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING);

    bool StartShell();
    void StopShell();
    void ReaderLoop();
    void SendRaw(const std::wstring& text);
    void ExecuteInput();
    void AppendLine(const std::wstring& line);
    void ProcessShellOutput(const std::wstring& text);
    void AddBanner();

    std::wstring PromptLabel() const;
    float MeasureTextWidth(const std::wstring& text, IDWriteTextFormat* format);

    bool CopyToClipboard(const std::wstring& text);
    std::wstring PasteFromClipboard();

    bool HasSelection() const;
    std::pair<std::size_t, std::size_t> SelectionRange() const;
    void ClearSelection();
    void SelectAllInput();
    void DeleteSelection();
    void InsertInputText(const std::wstring& text);
    void MoveCaret(std::size_t position, bool keepSelection);

    void SaveUndoSnapshot();
    void UndoInput();

    std::wstring FindExecutable(const std::wstring& command) const;
    std::wstring DetectPythonCommand() const;
    bool CommandExists(const std::wstring& command) const;
    void RefreshDetectedTools();
    void PrintToolCheck();
    void RestartShell();

    std::wstring ExecutablePath() const;
    bool SetRegistryString(
        HKEY root,
        const std::wstring& subKey,
        const wchar_t* valueName,
        const std::wstring& value) const;
    bool InstallExplorerMenu();
    bool RemoveExplorerMenu();

    bool OpenEditor(const std::wstring& fileName);
    bool SaveEditor();
    void CloseEditor();
    void RenderEditor(float width, float height);
    void DrawEditorSyntaxLine(
        const std::wstring& line,
        float x,
        float y,
        float right,
        float lineHeight);
    void InsertEditorCharacter(wchar_t character);
    void InsertEditorNewLine();
    void EditorBackspace();
    void EditorDelete();
    void KeepEditorCursorVisible();
    EditorPoint EditorPointFromMouse(float x, float y);
    bool EditorHasSelection() const;
    void ClearEditorSelection();
    std::wstring SelectedEditorText() const;
    void DeleteEditorSelection();
    std::wstring DecodeUtf8(const std::string& bytes) const;
    std::string EncodeUtf8(const std::wstring& text) const;

    OutputPoint OutputPointFromMouse(float x, float y);
    std::wstring SelectedOutputText() const;
    void ClearOutputSelection();
    void OpenContextMenu(float x, float y, bool copyMode);
    void CloseContextMenu();
    bool ContextMenuHit(float x, float y) const;

    HICON CreateCmdIcon(int size);
    void AddTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayMenu();
    void RestoreFromTray();
};

bool App::CreateFactories()
{
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory_)))
        return false;

    if (FAILED(DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&writeFactory_))))
        return false;

    auto createFont = [&](const wchar_t* family, float size, DWRITE_FONT_WEIGHT weight,
        IDWriteTextFormat** out) -> bool
        {
            HRESULT hr = writeFactory_->CreateTextFormat(
                family,
                nullptr,
                weight,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                size,
                L"en-US",
                out);

            if (FAILED(hr)) return false;
            (*out)->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            (*out)->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            return true;
        };

    return createFont(
        Customize::UiFont,
        Customize::TitleFontSize,
        DWRITE_FONT_WEIGHT_NORMAL,
        &titleFont_) &&
        createFont(
            Customize::TerminalFont,
            Customize::TerminalFontSize,
            DWRITE_FONT_WEIGHT_NORMAL,
            &terminalFont_) &&
        createFont(
            Customize::UiFont,
            Customize::SmallFontSize,
            DWRITE_FONT_WEIGHT_NORMAL,
            &smallFont_);
}

bool App::CreateResources()
{
    if (target_) return true;

    RECT rc{};
    GetClientRect(hwnd_, &rc);

    HRESULT hr = d2dFactory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_HARDWARE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
        D2D1::HwndRenderTargetProperties(
            hwnd_,
            D2D1::SizeU(static_cast<UINT32>(rc.right), static_cast<UINT32>(rc.bottom)),
            D2D1_PRESENT_OPTIONS_IMMEDIATELY),
        &target_);

    if (FAILED(hr)) return false;
    return SUCCEEDED(target_->CreateSolidColorBrush(Color(0xFFFFFF), &brush_));
}

void App::DiscardResources()
{
    SafeRelease(brush_);
    SafeRelease(target_);
}



std::wstring App::ExecutablePath() const
{
    std::wstring path(32768, L'\0');

    const DWORD length = GetModuleFileNameW(
        nullptr,
        path.data(),
        static_cast<DWORD>(path.size()));

    if (length == 0 || length >= path.size())
        return L"";

    path.resize(length);
    return path;
}

bool App::SetRegistryString(
    HKEY root,
    const std::wstring& subKey,
    const wchar_t* valueName,
    const std::wstring& value) const
{
    HKEY key = nullptr;

    const LONG createResult = RegCreateKeyExW(
        root,
        subKey.c_str(),
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        nullptr,
        &key,
        nullptr);

    if (createResult != ERROR_SUCCESS)
        return false;

    const DWORD byteCount = static_cast<DWORD>(
        (value.size() + 1) * sizeof(wchar_t));

    const LONG setResult = RegSetValueExW(
        key,
        valueName,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(value.c_str()),
        byteCount);

    RegCloseKey(key);
    return setResult == ERROR_SUCCESS;
}

bool App::InstallExplorerMenu()
{
    const std::wstring exe = ExecutablePath();

    if (exe.empty())
    {
        AppendLine(L"[error] Could not determine the executable path.");
        return false;
    }

    const std::wstring label = L"Open in Maotaw Terminal";
    const std::wstring icon = L"%SystemRoot%\\System32\\cmd.exe,0";
    const std::wstring folderCommand =
        L"\"" + exe + L"\" --dir \"%1\"";
    const std::wstring backgroundCommand =
        L"\"" + exe + L"\" --dir \"%V\"";

    const std::wstring folderBase =
        L"Software\\Classes\\Directory\\shell\\MaotawTerminal";

    const std::wstring backgroundBase =
        L"Software\\Classes\\Directory\\Background\\shell\\MaotawTerminal";

    const std::wstring driveBase =
        L"Software\\Classes\\Drive\\shell\\MaotawTerminal";

    bool success = true;

    success &= SetRegistryString(
        HKEY_CURRENT_USER,
        folderBase,
        nullptr,
        label);

    success &= SetRegistryString(
        HKEY_CURRENT_USER,
        folderBase,
        L"Icon",
        icon);

    success &= SetRegistryString(
        HKEY_CURRENT_USER,
        folderBase + L"\\command",
        nullptr,
        folderCommand);

    success &= SetRegistryString(
        HKEY_CURRENT_USER,
        backgroundBase,
        nullptr,
        label);

    success &= SetRegistryString(
        HKEY_CURRENT_USER,
        backgroundBase,
        L"Icon",
        icon);

    success &= SetRegistryString(
        HKEY_CURRENT_USER,
        backgroundBase + L"\\command",
        nullptr,
        backgroundCommand);

    success &= SetRegistryString(
        HKEY_CURRENT_USER,
        driveBase,
        nullptr,
        label);

    success &= SetRegistryString(
        HKEY_CURRENT_USER,
        driveBase,
        L"Icon",
        icon);

    success &= SetRegistryString(
        HKEY_CURRENT_USER,
        driveBase + L"\\command",
        nullptr,
        folderCommand);

    if (success)
    {
        SHChangeNotify(
            SHCNE_ASSOCCHANGED,
            SHCNF_IDLIST,
            nullptr,
            nullptr);

        AppendLine(L"Explorer integration installed.");
        AppendLine(L"Right-click a folder, drive, or empty folder space.");
        AppendLine(L"Choose: Open in Maotaw Terminal");
    }
    else
    {
        AppendLine(L"[error] Could not install every Explorer menu entry.");
    }

    return success;
}

bool App::RemoveExplorerMenu()
{
    const std::wstring folderBase =
        L"Software\\Classes\\Directory\\shell\\MaotawTerminal";

    const std::wstring backgroundBase =
        L"Software\\Classes\\Directory\\Background\\shell\\MaotawTerminal";

    const std::wstring driveBase =
        L"Software\\Classes\\Drive\\shell\\MaotawTerminal";

    const LONG folderResult = RegDeleteTreeW(
        HKEY_CURRENT_USER,
        folderBase.c_str());

    const LONG backgroundResult = RegDeleteTreeW(
        HKEY_CURRENT_USER,
        backgroundBase.c_str());

    const LONG driveResult = RegDeleteTreeW(
        HKEY_CURRENT_USER,
        driveBase.c_str());

    const bool success =
        (folderResult == ERROR_SUCCESS || folderResult == ERROR_FILE_NOT_FOUND) &&
        (backgroundResult == ERROR_SUCCESS || backgroundResult == ERROR_FILE_NOT_FOUND) &&
        (driveResult == ERROR_SUCCESS || driveResult == ERROR_FILE_NOT_FOUND);

    SHChangeNotify(
        SHCNE_ASSOCCHANGED,
        SHCNF_IDLIST,
        nullptr,
        nullptr);

    if (success)
        AppendLine(L"Explorer integration removed.");
    else
        AppendLine(L"[error] Some Explorer menu entries could not be removed.");

    return success;
}

App::EditorPoint App::EditorPointFromMouse(float x, float y)
{
    EditorPoint point{};

    const int screenRow = static_cast<int>(
        (y - editorTextTop_) / editorLineHeight_);

    const int fileRow = editorScrollRow_ + screenRow;

    if (
        screenRow < 0 ||
        screenRow >= editorVisibleRows_ ||
        fileRow < 0 ||
        fileRow >= static_cast<int>(editorLines_.size()))
    {
        point.row = editorRow_;
        point.column = editorColumn_;
        return point;
    }

    point.row = static_cast<std::size_t>(fileRow);

    const std::wstring& line = editorLines_[point.row];
    const float localX = std::max(0.0f, x - editorTextLeft_);

    std::size_t bestColumn = 0;
    float bestDistance = localX;

    for (std::size_t column = 1; column <= line.size(); ++column)
    {
        const float width = MeasureTextWidth(
            line.substr(0, column),
            terminalFont_);

        const float distance = std::abs(localX - width);

        if (distance <= bestDistance)
        {
            bestDistance = distance;
            bestColumn = column;
        }
        else if (width > localX)
        {
            break;
        }
    }

    point.column = std::min(bestColumn, line.size());
    return point;
}

bool App::EditorHasSelection() const
{
    return
        editorHasSelection_ &&
        (editorSelectionAnchor_.row != editorSelectionCurrent_.row ||
            editorSelectionAnchor_.column != editorSelectionCurrent_.column);
}

void App::ClearEditorSelection()
{
    editorSelecting_ = false;
    editorHasSelection_ = false;
    editorSelectionAnchor_ = { editorRow_, editorColumn_ };
    editorSelectionCurrent_ = editorSelectionAnchor_;
}

std::wstring App::SelectedEditorText() const
{
    if (!EditorHasSelection())
        return L"";

    EditorPoint first = editorSelectionAnchor_;
    EditorPoint last = editorSelectionCurrent_;

    if (
        first.row > last.row ||
        (first.row == last.row && first.column > last.column))
    {
        std::swap(first, last);
    }

    std::wstring result;

    for (std::size_t row = first.row; row <= last.row; ++row)
    {
        const std::wstring& line = editorLines_[row];

        const std::size_t begin =
            row == first.row
            ? std::min(first.column, line.size())
            : 0;

        const std::size_t end =
            row == last.row
            ? std::min(last.column, line.size())
            : line.size();

        if (end > begin)
            result += line.substr(begin, end - begin);

        if (row != last.row)
            result += L"\r\n";
    }

    return result;
}

void App::DeleteEditorSelection()
{
    if (!EditorHasSelection())
        return;

    EditorPoint first = editorSelectionAnchor_;
    EditorPoint last = editorSelectionCurrent_;

    if (
        first.row > last.row ||
        (first.row == last.row && first.column > last.column))
    {
        std::swap(first, last);
    }

    if (first.row == last.row)
    {
        std::wstring& line = editorLines_[first.row];
        line.erase(
            std::min(first.column, line.size()),
            std::min(last.column, line.size()) -
            std::min(first.column, line.size()));
    }
    else
    {
        std::wstring merged =
            editorLines_[first.row].substr(
                0,
                std::min(first.column, editorLines_[first.row].size()));

        merged += editorLines_[last.row].substr(
            std::min(last.column, editorLines_[last.row].size()));

        editorLines_[first.row] = std::move(merged);

        editorLines_.erase(
            editorLines_.begin() +
            static_cast<std::ptrdiff_t>(first.row + 1),
            editorLines_.begin() +
            static_cast<std::ptrdiff_t>(last.row + 1));
    }

    editorRow_ = first.row;
    editorColumn_ = first.column;
    ClearEditorSelection();
    editorDirty_ = true;
    editorStatus_ = L"Modified";
}

std::wstring App::DecodeUtf8(const std::string& bytes) const
{
    if (bytes.empty())
        return L"";

    int count = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        bytes.data(),
        static_cast<int>(bytes.size()),
        nullptr,
        0);

    if (count <= 0)
    {
        count = MultiByteToWideChar(
            CP_ACP,
            0,
            bytes.data(),
            static_cast<int>(bytes.size()),
            nullptr,
            0);

        if (count <= 0)
            return L"";
    }

    std::wstring result(static_cast<std::size_t>(count), L'\0');

    MultiByteToWideChar(
        count > 0 ? CP_UTF8 : CP_ACP,
        0,
        bytes.data(),
        static_cast<int>(bytes.size()),
        result.data(),
        count);

    return result;
}

std::string App::EncodeUtf8(const std::wstring& text) const
{
    if (text.empty())
        return {};

    const int count = WideCharToMultiByte(
        CP_UTF8,
        0,
        text.data(),
        static_cast<int>(text.size()),
        nullptr,
        0,
        nullptr,
        nullptr);

    if (count <= 0)
        return {};

    std::string result(static_cast<std::size_t>(count), '\0');

    WideCharToMultiByte(
        CP_UTF8,
        0,
        text.data(),
        static_cast<int>(text.size()),
        result.data(),
        count,
        nullptr,
        nullptr);

    return result;
}

bool App::OpenEditor(const std::wstring& fileName)
{
    if (fileName.empty())
    {
        AppendLine(L"Usage: edit filename");
        AppendLine(L"Examples: edit main.cpp   edit script.py   edit app.js");
        return false;
    }

    std::wstring cleanName = fileName;

    while (!cleanName.empty() && iswspace(cleanName.front()))
        cleanName.erase(cleanName.begin());

    while (!cleanName.empty() && iswspace(cleanName.back()))
        cleanName.pop_back();

    if (
        cleanName.size() >= 2 &&
        cleanName.front() == L'"' &&
        cleanName.back() == L'"')
    {
        cleanName = cleanName.substr(1, cleanName.size() - 2);
    }

    std::filesystem::path path(cleanName);

    if (path.is_relative())
        path = std::filesystem::path(cwd_) / path;

    path = path.lexically_normal();

    editorLines_.clear();

    std::error_code error;

    if (std::filesystem::exists(path, error))
    {
        std::ifstream file(path, std::ios::binary);

        if (!file)
        {
            AppendLine(L"[error] Could not open: " + path.wstring());
            return false;
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();

        std::wstring text = DecodeUtf8(buffer.str());
        std::wstring current;

        for (wchar_t character : text)
        {
            if (character == L'\r')
                continue;

            if (character == L'\n')
            {
                editorLines_.push_back(current);
                current.clear();
            }
            else
            {
                current.push_back(character);
            }
        }

        editorLines_.push_back(current);
    }
    else
    {
        editorLines_.push_back(L"");
    }

    if (editorLines_.empty())
        editorLines_.push_back(L"");

    editorPath_ = path;
    editorRow_ = 0;
    editorColumn_ = 0;
    editorScrollRow_ = 0;
    editorScrollColumn_ = 0;
    editorVisibleRows_ = 1;
    editorDirty_ = false;
    editorMode_ = true;
    ClearEditorSelection();
    editorStatus_ = L"Ctrl+S Save   Esc Close   Tab Indent";

    windowTitle_ =
        L"Maotaw Editor - " +
        editorPath_.filename().wstring();

    SetWindowTextW(hwnd_, windowTitle_.c_str());
    InvalidateRect(hwnd_, nullptr, FALSE);
    return true;
}

bool App::SaveEditor()
{
    if (!editorMode_ || editorPath_.empty())
        return false;

    std::error_code error;
    const std::filesystem::path parent = editorPath_.parent_path();

    if (!parent.empty())
        std::filesystem::create_directories(parent, error);

    std::wstring combined;

    for (std::size_t index = 0; index < editorLines_.size(); ++index)
    {
        combined += editorLines_[index];

        if (index + 1 < editorLines_.size())
            combined += L"\r\n";
    }

    std::ofstream file(
        editorPath_,
        std::ios::binary |
        std::ios::trunc);

    if (!file)
    {
        editorStatus_ = L"Save failed";
        return false;
    }

    const std::string utf8 = EncodeUtf8(combined);
    file.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));

    if (!file.good())
    {
        editorStatus_ = L"Save failed";
        return false;
    }

    editorDirty_ = false;
    editorStatus_ = L"Saved: " + editorPath_.wstring();
    return true;
}

void App::CloseEditor()
{
    editorMode_ = false;
    editorDirty_ = false;
    editorStatus_.clear();
    windowTitle_ = Customize::AppTitle;
    SetWindowTextW(hwnd_, windowTitle_.c_str());
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::KeepEditorCursorVisible()
{
    RECT client{};
    GetClientRect(hwnd_, &client);

    const float lineHeight = 20.0f;
    const int visibleRows = std::max(
        1,
        static_cast<int>(
            (client.bottom - kTitleHeight - 55.0f) /
            lineHeight));

    if (static_cast<int>(editorRow_) < editorScrollRow_)
        editorScrollRow_ = static_cast<int>(editorRow_);

    if (
        static_cast<int>(editorRow_) >=
        editorScrollRow_ + visibleRows)
    {
        editorScrollRow_ =
            static_cast<int>(editorRow_) -
            visibleRows +
            1;
    }

    const int maximumScroll = std::max(
        0,
        static_cast<int>(editorLines_.size()) - visibleRows);

    editorScrollRow_ = std::clamp(
        editorScrollRow_,
        0,
        maximumScroll);
}

void App::InsertEditorCharacter(wchar_t character)
{
    if (!editorMode_)
        return;

    if (EditorHasSelection())
        DeleteEditorSelection();

    std::wstring& line = editorLines_[editorRow_];
    editorColumn_ = std::min(editorColumn_, line.size());
    line.insert(line.begin() + static_cast<std::ptrdiff_t>(editorColumn_), character);
    ++editorColumn_;
    editorDirty_ = true;
    editorStatus_ = L"Modified";
    KeepEditorCursorVisible();
}

void App::InsertEditorNewLine()
{
    if (EditorHasSelection())
        DeleteEditorSelection();

    std::wstring& line = editorLines_[editorRow_];
    editorColumn_ = std::min(editorColumn_, line.size());

    const std::wstring right = line.substr(editorColumn_);
    line.erase(editorColumn_);

    editorLines_.insert(
        editorLines_.begin() +
        static_cast<std::ptrdiff_t>(editorRow_ + 1),
        right);

    ++editorRow_;
    editorColumn_ = 0;
    editorDirty_ = true;
    editorStatus_ = L"Modified";
    KeepEditorCursorVisible();
}

void App::EditorBackspace()
{
    if (EditorHasSelection())
    {
        DeleteEditorSelection();
        return;
    }

    if (editorColumn_ > 0)
    {
        std::wstring& line = editorLines_[editorRow_];
        line.erase(editorColumn_ - 1, 1);
        --editorColumn_;
    }
    else if (editorRow_ > 0)
    {
        const std::size_t previousLength =
            editorLines_[editorRow_ - 1].size();

        editorLines_[editorRow_ - 1] += editorLines_[editorRow_];

        editorLines_.erase(
            editorLines_.begin() +
            static_cast<std::ptrdiff_t>(editorRow_));

        --editorRow_;
        editorColumn_ = previousLength;
    }
    else
    {
        return;
    }

    editorDirty_ = true;
    editorStatus_ = L"Modified";
    KeepEditorCursorVisible();
}

void App::EditorDelete()
{
    if (EditorHasSelection())
    {
        DeleteEditorSelection();
        return;
    }

    std::wstring& line = editorLines_[editorRow_];

    if (editorColumn_ < line.size())
    {
        line.erase(editorColumn_, 1);
    }
    else if (editorRow_ + 1 < editorLines_.size())
    {
        line += editorLines_[editorRow_ + 1];

        editorLines_.erase(
            editorLines_.begin() +
            static_cast<std::ptrdiff_t>(editorRow_ + 1));
    }
    else
    {
        return;
    }

    editorDirty_ = true;
    editorStatus_ = L"Modified";
}

void App::DrawEditorSyntaxLine(
    const std::wstring& line,
    float x,
    float y,
    float right,
    float lineHeight)
{
    static const std::vector<std::wstring> keywords =
    {
        L"auto", L"bool", L"break", L"case", L"catch", L"class",
        L"const", L"constexpr", L"continue", L"def", L"delete",
        L"do", L"double", L"else", L"enum", L"except", L"false",
        L"float", L"for", L"from", L"function", L"if", L"import",
        L"in", L"int", L"interface", L"let", L"long", L"namespace",
        L"new", L"None", L"nullptr", L"private", L"protected",
        L"public", L"return", L"static", L"string", L"struct",
        L"switch", L"this", L"throw", L"true", L"try", L"using",
        L"var", L"void", L"while"
    };

    float currentX = x;
    std::size_t index = 0;

    auto drawToken =
        [&](const std::wstring& token, const D2D1_COLOR_F& color)
        {
            if (token.empty())
                return;

            const float tokenWidth =
                MeasureTextWidth(token, terminalFont_);

            if (currentX + tokenWidth >= x && currentX <= right)
            {
                DrawTextLine(
                    token,
                    currentX,
                    y,
                    right,
                    y + lineHeight,
                    color,
                    terminalFont_);
            }

            currentX += tokenWidth;
        };

    while (index < line.size())
    {
        if (
            line[index] == L'/' &&
            index + 1 < line.size() &&
            line[index + 1] == L'/')
        {
            drawToken(
                line.substr(index),
                Color(Customize::EditorCommentColor));
            break;
        }

        if (line[index] == L'#')
        {
            drawToken(
                line.substr(index),
                Color(Customize::EditorCommentColor));
            break;
        }

        if (line[index] == L'"' || line[index] == L'\'')
        {
            const wchar_t quote = line[index];
            std::size_t end = index + 1;
            bool escaped = false;

            while (end < line.size())
            {
                if (!escaped && line[end] == quote)
                {
                    ++end;
                    break;
                }

                escaped = !escaped && line[end] == L'\\';

                if (line[end] != L'\\')
                    escaped = false;

                ++end;
            }

            drawToken(
                line.substr(index, end - index),
                Color(Customize::EditorStringColor));

            index = end;
            continue;
        }

        if (iswdigit(line[index]))
        {
            std::size_t end = index + 1;

            while (
                end < line.size() &&
                (iswdigit(line[end]) ||
                    line[end] == L'.' ||
                    line[end] == L'x' ||
                    (line[end] >= L'A' && line[end] <= L'F') ||
                    (line[end] >= L'a' && line[end] <= L'f')))
            {
                ++end;
            }

            drawToken(
                line.substr(index, end - index),
                Color(Customize::EditorNumberColor));

            index = end;
            continue;
        }

        if (iswalpha(line[index]) || line[index] == L'_')
        {
            std::size_t end = index + 1;

            while (
                end < line.size() &&
                (iswalnum(line[end]) || line[end] == L'_'))
            {
                ++end;
            }

            const std::wstring word =
                line.substr(index, end - index);

            bool keyword = false;

            for (const std::wstring& item : keywords)
            {
                if (word == item)
                {
                    keyword = true;
                    break;
                }
            }

            std::size_t lookAhead = end;

            while (
                lookAhead < line.size() &&
                iswspace(line[lookAhead]))
            {
                ++lookAhead;
            }

            const bool functionName =
                !keyword &&
                lookAhead < line.size() &&
                line[lookAhead] == L'(';

            drawToken(
                word,
                keyword
                ? Color(Customize::EditorKeywordColor)
                : functionName
                ? Color(Customize::EditorFunctionColor)
                : terminalForeground_);

            index = end;
            continue;
        }

        drawToken(
            line.substr(index, 1),
            terminalForeground_);

        ++index;
    }
}

void App::RenderEditor(float width, float height)
{
    const D2D1_COLOR_F background =
        Color(Customize::BackgroundColor);

    const D2D1_COLOR_F titleBar =
        Color(Customize::TitleBarColor);

    const D2D1_COLOR_F accent =
        Color(Customize::AccentColor);

    const D2D1_COLOR_F border =
        Color(Customize::BorderColor);

    FillRect(0.0f, 0.0f, width, height, background);
    FillRect(0.0f, 0.0f, width, static_cast<float>(kTitleHeight), titleBar);

    DrawLine(
        0.0f,
        static_cast<float>(kTitleHeight),
        width,
        static_cast<float>(kTitleHeight),
        border,
        1.0f);

    FillRound(
        12.0f,
        8.0f,
        28.0f,
        24.0f,
        2.0f,
        accent);

    DrawTextLine(
        L"E",
        12.0f,
        8.0f,
        28.0f,
        24.0f,
        Color(0x101214),
        smallFont_,
        DWRITE_TEXT_ALIGNMENT_CENTER);

    const std::wstring title =
        L"Editing  " +
        editorPath_.filename().wstring() +
        (editorDirty_ ? L"  *" : L"");

    DrawTextLine(
        title,
        36.0f,
        0.0f,
        width - 100.0f,
        static_cast<float>(kTitleHeight),
        accent,
        titleFont_);

    const float lineHeight = 20.0f;
    const float editorTop = static_cast<float>(kTitleHeight) + 10.0f;
    const float statusHeight = 28.0f;
    const float editorBottom = height - statusHeight;
    const int visibleRows = std::max(
        1,
        static_cast<int>((editorBottom - editorTop) / lineHeight));

    // Do not force the viewport back to the cursor while rendering.
    // Cursor-following is performed only after keyboard/cursor movement.
    const int maximumScroll = std::max(
        0,
        static_cast<int>(editorLines_.size()) - visibleRows);

    editorScrollRow_ = std::clamp(
        editorScrollRow_,
        0,
        maximumScroll);

    const int digits = std::max(
        3,
        static_cast<int>(
            std::to_wstring(editorLines_.size()).size()));

    const float gutterWidth =
        14.0f +
        digits * MeasureTextWidth(L"0", terminalFont_);

    editorTextLeft_ = gutterWidth + 10.0f;
    editorTextTop_ = editorTop;
    editorTextBottom_ = editorBottom;
    editorLineHeight_ = lineHeight;
    editorVisibleRows_ = visibleRows;

    FillRect(
        0.0f,
        editorTop - 4.0f,
        gutterWidth,
        editorBottom,
        Color(0x090B08));

    DrawLine(
        gutterWidth,
        editorTop - 4.0f,
        gutterWidth,
        editorBottom,
        border,
        1.0f);

    for (int screenRow = 0; screenRow < visibleRows; ++screenRow)
    {
        const int fileRow = editorScrollRow_ + screenRow;

        if (fileRow >= static_cast<int>(editorLines_.size()))
            break;

        const float y = editorTop + screenRow * lineHeight;

        const std::wstring lineNumber =
            std::to_wstring(fileRow + 1);

        DrawTextLine(
            lineNumber,
            4.0f,
            y,
            gutterWidth - 8.0f,
            y + lineHeight,
            Color(Customize::EditorLineNumberColor),
            terminalFont_,
            DWRITE_TEXT_ALIGNMENT_TRAILING);

        const std::wstring& editorLine =
            editorLines_[static_cast<std::size_t>(fileRow)];

        if (EditorHasSelection())
        {
            EditorPoint first = editorSelectionAnchor_;
            EditorPoint last = editorSelectionCurrent_;

            if (
                first.row > last.row ||
                (first.row == last.row &&
                    first.column > last.column))
            {
                std::swap(first, last);
            }

            if (
                static_cast<std::size_t>(fileRow) >= first.row &&
                static_cast<std::size_t>(fileRow) <= last.row)
            {
                const std::size_t begin =
                    static_cast<std::size_t>(fileRow) == first.row
                    ? std::min(first.column, editorLine.size())
                    : 0;

                const std::size_t finish =
                    static_cast<std::size_t>(fileRow) == last.row
                    ? std::min(last.column, editorLine.size())
                    : editorLine.size();

                if (finish > begin)
                {
                    const float selectionLeft =
                        editorTextLeft_ +
                        MeasureTextWidth(
                            editorLine.substr(0, begin),
                            terminalFont_);

                    const float selectionRight =
                        editorTextLeft_ +
                        MeasureTextWidth(
                            editorLine.substr(0, finish),
                            terminalFont_);

                    FillRound(
                        selectionLeft,
                        y,
                        selectionRight,
                        y + lineHeight,
                        1.0f,
                        Color(Customize::SelectionColor));
                }
            }
        }

        DrawEditorSyntaxLine(
            editorLine,
            editorTextLeft_,
            y,
            width - 12.0f,
            lineHeight);
    }

    const int cursorScreenRow =
        static_cast<int>(editorRow_) -
        editorScrollRow_;

    if (
        cursorScreenRow >= 0 &&
        cursorScreenRow < visibleRows &&
        caretVisible_)
    {
        const std::wstring& line = editorLines_[editorRow_];

        const float cursorX =
            gutterWidth +
            10.0f +
            MeasureTextWidth(
                line.substr(
                    0,
                    std::min(editorColumn_, line.size())),
                terminalFont_);

        const float cursorY =
            editorTop +
            cursorScreenRow * lineHeight;

        FillRect(
            cursorX,
            cursorY + 2.0f,
            cursorX + 7.0f,
            cursorY + lineHeight - 2.0f,
            accent);
    }

    FillRect(
        0.0f,
        height - statusHeight,
        width,
        height,
        titleBar);

    DrawLine(
        0.0f,
        height - statusHeight,
        width,
        height - statusHeight,
        border,
        1.0f);

    const std::wstring position =
        L"Ln " +
        std::to_wstring(editorRow_ + 1) +
        L", Col " +
        std::to_wstring(editorColumn_ + 1);

    DrawTextLine(
        editorStatus_,
        12.0f,
        height - statusHeight,
        width - 180.0f,
        height,
        Color(Customize::EditorStatusColor),
        smallFont_);

    DrawTextLine(
        position,
        width - 170.0f,
        height - statusHeight,
        width - 12.0f,
        height,
        Color(Customize::DirectoryColor),
        smallFont_,
        DWRITE_TEXT_ALIGNMENT_TRAILING);
}

std::wstring App::FindExecutable(const std::wstring& command) const
{
    if (command.empty()) return L"";

    wchar_t result[32768]{};

    const DWORD length = SearchPathW(
        nullptr,
        command.c_str(),
        L".exe",
        static_cast<DWORD>(std::size(result)),
        result,
        nullptr);

    if (length > 0 && length < std::size(result))
        return result;

    return L"";
}

bool App::CommandExists(const std::wstring& command) const
{
    return !FindExecutable(command).empty();
}

std::wstring App::DetectPythonCommand() const
{
    const std::wstring pythonPath = FindExecutable(L"python");

    if (!pythonPath.empty())
    {
        std::wstring lower = pythonPath;

        std::transform(
            lower.begin(),
            lower.end(),
            lower.begin(),
            [](wchar_t character)
            {
                return static_cast<wchar_t>(towlower(character));
            });

        if (lower.find(L"windowsapps") == std::wstring::npos)
            return L"python";
    }

    if (CommandExists(L"py")) return L"py";
    if (CommandExists(L"python3")) return L"python3";
    if (!pythonPath.empty()) return L"python";

    return L"";
}

void App::RefreshDetectedTools()
{
    detectedPythonCommand_ = DetectPythonCommand();
    detectedCargoCommand_ = CommandExists(L"cargo") ? L"cargo" : L"";
    detectedGitCommand_ = CommandExists(L"git") ? L"git" : L"";
    detectedNodeCommand_ = CommandExists(L"node") ? L"node" : L"";
}

void App::PrintToolCheck()
{
    RefreshDetectedTools();

    AppendLine(L"DEVELOPER TOOL CHECK");
    AppendLine(L"");

    auto printTool = [&](const std::wstring& name, const std::wstring& command)
        {
            if (command.empty())
            {
                AppendLine(L"  " + name + L": not found in Windows PATH");
                return;
            }

            AppendLine(L"  " + name + L": " + command);

            const std::wstring path = FindExecutable(command);
            if (!path.empty())
                AppendLine(L"    " + path);
        };

    printTool(L"Python", detectedPythonCommand_);
    printTool(L"Cargo", detectedCargoCommand_);
    printTool(L"Git", detectedGitCommand_);
    printTool(L"Node.js", detectedNodeCommand_);
    printTool(L"npm", CommandExists(L"npm") ? L"npm" : L"");
    printTool(L"Rust compiler", CommandExists(L"rustc") ? L"rustc" : L"");
    printTool(L"Java", CommandExists(L"java") ? L"java" : L"");
    printTool(L"CMake", CommandExists(L"cmake") ? L"cmake" : L"");

    AppendLine(L"");
    AppendLine(L"Use refreshenv after installing a tool while this terminal is open.");

    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::RestartShell()
{
    // Use the terminal's existing shell cleanup routine. It safely stops the
    // reader thread and closes childProcess_, childThread_,
    // childStdinWrite_ and childStdoutRead_.
    StopShell();

    commandRunning_ = false;
    waitingForDirectory_ = false;

    if (StartShell())
    {
        RefreshDetectedTools();
        AppendLine(L"Environment refreshed from Windows.");
    }
    else
    {
        AppendLine(L"[error] Could not restart cmd.exe.");
    }

    InvalidateRect(hwnd_, nullptr, FALSE);
}

App::OutputPoint App::OutputPointFromMouse(float x, float y)
{
    OutputPoint point{};

    if (
        y < outputTop_ ||
        y >= outputBottom_ ||
        outputLineHeight_ <= 0.0f)
    {
        return point;
    }

    const int screenRow = static_cast<int>(
        (y - outputTop_) / outputLineHeight_);

    if (
        screenRow < 0 ||
        screenRow >= outputVisibleRows_)
    {
        return point;
    }

    point.row = outputFirstVisibleRow_ + screenRow;

    if (
        point.row < 0 ||
        point.row >= static_cast<int>(lines_.size()))
    {
        point.row = -1;
        return point;
    }

    const std::wstring& line =
        lines_[static_cast<std::size_t>(point.row)];

    const float localX = std::max(0.0f, x - outputTextLeft_);

    // Find the nearest character boundary. This works with any configured
    // font rather than assuming one fixed character width.
    std::size_t bestColumn = 0;
    float bestDistance = localX;

    for (std::size_t column = 1; column <= line.size(); ++column)
    {
        const float textWidth = MeasureTextWidth(
            line.substr(0, column),
            terminalFont_);

        const float distance = std::abs(localX - textWidth);

        if (distance <= bestDistance)
        {
            bestDistance = distance;
            bestColumn = column;
        }
        else if (textWidth > localX)
        {
            break;
        }
    }

    point.column = std::min(bestColumn, line.size());
    return point;
}

std::wstring App::SelectedOutputText() const
{
    if (!outputHasSelection_)
        return L"";

    OutputPoint first = outputSelectionAnchor_;
    OutputPoint last = outputSelectionCurrent_;

    if (
        first.row > last.row ||
        (first.row == last.row && first.column > last.column))
    {
        std::swap(first, last);
    }

    if (
        first.row < 0 ||
        last.row < 0 ||
        first.row >= static_cast<int>(lines_.size()))
    {
        return L"";
    }

    last.row = std::min(
        last.row,
        static_cast<int>(lines_.size()) - 1);

    std::wstring result;

    for (int row = first.row; row <= last.row; ++row)
    {
        const std::wstring& line =
            lines_[static_cast<std::size_t>(row)];

        const std::size_t begin =
            row == first.row
            ? std::min(first.column, line.size())
            : 0;

        const std::size_t end =
            row == last.row
            ? std::min(last.column, line.size())
            : line.size();

        if (end > begin)
            result += line.substr(begin, end - begin);

        if (row != last.row)
            result += L"\r\n";
    }

    return result;
}

void App::ClearOutputSelection()
{
    outputSelecting_ = false;
    outputHasSelection_ = false;
    outputSelectionAnchor_ = OutputPoint{};
    outputSelectionCurrent_ = OutputPoint{};
}

void App::OpenContextMenu(float x, float y, bool copyMode)
{
    RECT client{};
    GetClientRect(hwnd_, &client);

    const float clientWidth = static_cast<float>(client.right);
    const float clientHeight = static_cast<float>(client.bottom);

    contextMenuCopyMode_ = copyMode;
    contextMenuX_ = std::clamp(
        x,
        4.0f,
        std::max(4.0f, clientWidth - contextMenuWidth_ - 4.0f));

    contextMenuY_ = std::clamp(
        y,
        static_cast<float>(kTitleHeight) + 4.0f,
        std::max(
            static_cast<float>(kTitleHeight) + 4.0f,
            clientHeight - contextMenuHeight_ - 4.0f));

    contextMenuOpen_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::CloseContextMenu()
{
    contextMenuOpen_ = false;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

bool App::ContextMenuHit(float x, float y) const
{
    return
        contextMenuOpen_ &&
        x >= contextMenuX_ &&
        x <= contextMenuX_ + contextMenuWidth_ &&
        y >= contextMenuY_ &&
        y <= contextMenuY_ + contextMenuHeight_;
}


HICON App::CreateCmdIcon(int size)
{
    HDC screen = GetDC(nullptr);
    HDC memory = CreateCompatibleDC(screen);

    BITMAPV5HEADER header{};
    header.bV5Size = sizeof(header);
    header.bV5Width = size;
    header.bV5Height = -size;
    header.bV5Planes = 1;
    header.bV5BitCount = 32;
    header.bV5Compression = BI_BITFIELDS;
    header.bV5RedMask = 0x00FF0000;
    header.bV5GreenMask = 0x0000FF00;
    header.bV5BlueMask = 0x000000FF;
    header.bV5AlphaMask = 0xFF000000;

    void* bits = nullptr;
    HBITMAP colorBitmap = CreateDIBSection(
        screen,
        reinterpret_cast<BITMAPINFO*>(&header),
        DIB_RGB_COLORS,
        &bits,
        nullptr,
        0);

    HBITMAP oldBitmap =
        static_cast<HBITMAP>(SelectObject(memory, colorBitmap));

    RECT bounds{ 0, 0, size, size };

    HBRUSH background = CreateSolidBrush(RGB(25, 28, 32));
    ::FillRect(memory, &bounds, background);
    DeleteObject(background);

    const int radius = std::max(2, size / 7);
    HPEN borderPen = CreatePen(
        PS_SOLID,
        std::max(1, size / 16),
        RGB(77, 84, 94));
    HBRUSH panelBrush = CreateSolidBrush(RGB(32, 36, 41));

    HGDIOBJ oldPen = SelectObject(memory, borderPen);
    HGDIOBJ oldBrush = SelectObject(memory, panelBrush);

    RoundRect(
        memory,
        1,
        1,
        size - 1,
        size - 1,
        radius,
        radius);

    SelectObject(memory, oldPen);
    SelectObject(memory, oldBrush);
    DeleteObject(borderPen);
    DeleteObject(panelBrush);

    HPEN glyphPen = CreatePen(
        PS_SOLID,
        std::max(1, size / 10),
        RGB(235, 239, 243));

    oldPen = SelectObject(memory, glyphPen);

    const int left = size / 4;
    const int centerY = size / 2;
    const int arrow = size / 6;

    MoveToEx(memory, left, centerY - arrow, nullptr);
    LineTo(memory, left + arrow, centerY);
    LineTo(memory, left, centerY + arrow);

    MoveToEx(memory, left + arrow + size / 10, centerY + arrow, nullptr);
    LineTo(memory, size - size / 5, centerY + arrow);

    SelectObject(memory, oldPen);
    DeleteObject(glyphPen);

    // Make every painted pixel opaque.
    if (bits)
    {
        auto* pixels = static_cast<unsigned int*>(bits);
        for (int index = 0; index < size * size; ++index)
            pixels[index] |= 0xFF000000;
    }

    SelectObject(memory, oldBitmap);

    HBITMAP maskBitmap = CreateBitmap(size, size, 1, 1, nullptr);

    ICONINFO iconInfo{};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = colorBitmap;
    iconInfo.hbmMask = maskBitmap;

    HICON icon = CreateIconIndirect(&iconInfo);

    DeleteObject(maskBitmap);
    DeleteObject(colorBitmap);
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);

    return icon;
}

void App::AddTrayIcon()
{
    if (!Customize::ShowTrayIcon || trayAdded_ || !hwnd_)
        return;

    trayIcon_ = {};
    trayIcon_.cbSize = sizeof(trayIcon_);
    trayIcon_.hWnd = hwnd_;
    trayIcon_.uID = 1;
    trayIcon_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    trayIcon_.uCallbackMessage = WM_TRAYICON;
    trayIcon_.hIcon = appIcon_;

    wcscpy_s(
        trayIcon_.szTip,
        _countof(trayIcon_.szTip),
        Customize::TrayTooltip);

    trayAdded_ =
        Shell_NotifyIconW(NIM_ADD, &trayIcon_) != FALSE;

    if (trayAdded_)
    {
        trayIcon_.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &trayIcon_);
    }
}

void App::RemoveTrayIcon()
{
    if (!trayAdded_)
        return;

    Shell_NotifyIconW(NIM_DELETE, &trayIcon_);
    trayAdded_ = false;
}

void App::RestoreFromTray()
{
    ShowWindow(hwnd_, SW_RESTORE);
    ShowWindow(hwnd_, SW_SHOW);
    SetForegroundWindow(hwnd_);
    SetFocus(hwnd_);
}

void App::ShowTrayMenu()
{
    POINT cursor{};
    GetCursorPos(&cursor);

    HMENU menu = CreatePopupMenu();
    if (!menu)
        return;

    AppendMenuW(menu, MF_STRING, ID_TRAY_OPEN, L"Open Maotaw Terminal");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"Exit");

    SetForegroundWindow(hwnd_);

    const UINT selected = TrackPopupMenu(
        menu,
        TPM_RETURNCMD |
        TPM_RIGHTBUTTON |
        TPM_NONOTIFY,
        cursor.x,
        cursor.y,
        0,
        hwnd_,
        nullptr);

    DestroyMenu(menu);

    if (selected == ID_TRAY_OPEN)
    {
        RestoreFromTray();
    }
    else if (selected == ID_TRAY_EXIT)
    {
        exiting_ = true;
        RemoveTrayIcon();
        DestroyWindow(hwnd_);
    }
}

bool App::Initialize(HINSTANCE instance)
{
    SetProcessDPIAware();
    if (!CreateFactories()) return false;

    appIcon_ = CreateCmdIcon(64);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = StaticProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = appIcon_;
    wc.hIconSm = appIcon_;
    wc.lpszClassName = L"MaotawCustomTealTerminal";

    if (!RegisterClassExW(&wc)) return false;

    int x = (GetSystemMetrics(SM_CXSCREEN) - kWindowWidth) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - kWindowHeight) / 2;

    hwnd_ = CreateWindowExW(
        WS_EX_APPWINDOW | WS_EX_LAYERED,
        wc.lpszClassName,
        Customize::AppTitle,
        WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU,
        x, y, kWindowWidth, kWindowHeight,
        nullptr, nullptr, instance, this);

    if (!hwnd_) return false;

    SendMessageW(
        hwnd_,
        WM_SETICON,
        ICON_BIG,
        reinterpret_cast<LPARAM>(appIcon_));

    SendMessageW(
        hwnd_,
        WM_SETICON,
        ICON_SMALL,
        reinterpret_cast<LPARAM>(appIcon_));

    // A slightly translucent layered window creates the black-glass look.
    // The terminal remains readable while the desktop faintly shows through.
    SetLayeredWindowAttributes(
        hwnd_,
        0,
        Customize::WindowOpacity,
        LWA_ALPHA);

    HRGN region = CreateRoundRectRgn(
        0,
        0,
        kWindowWidth + 1,
        kWindowHeight + 1,
        Customize::WindowCornerRadius,
        Customize::WindowCornerRadius);
    SetWindowRgn(hwnd_, region, TRUE);

    StartShell();
    RefreshDetectedTools();
    AddTrayIcon();

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    SetFocus(hwnd_);
    SetTimer(hwnd_, kTimerId, 500, nullptr);
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
    if (hwnd_) KillTimer(hwnd_, kTimerId);
    StopShell();
    RemoveTrayIcon();

    if (appIcon_)
    {
        DestroyIcon(appIcon_);
        appIcon_ = nullptr;
    }

    DiscardResources();
    SafeRelease(smallFont_);
    SafeRelease(terminalFont_);
    SafeRelease(titleFont_);
    SafeRelease(writeFactory_);
    SafeRelease(d2dFactory_);
}

bool App::StartShell()
{
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE stdoutRead = nullptr;
    HANDLE stdoutWrite = nullptr;
    HANDLE stdinRead = nullptr;
    HANDLE stdinWrite = nullptr;

    if (!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0)) return false;
    if (!SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0)) return false;
    if (!CreatePipe(&stdinRead, &stdinWrite, &sa, 0)) return false;
    if (!SetHandleInformation(stdinWrite, HANDLE_FLAG_INHERIT, 0)) return false;

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = stdinRead;
    si.hStdOutput = stdoutWrite;
    si.hStdError = stdoutWrite;

    PROCESS_INFORMATION pi{};
    wchar_t commandLine[] = L"cmd.exe /Q /D /K \"prompt $S\"";
    const std::wstring startDirectory =
        cwd_.empty()
        ? ResolveStartDirectory()
        : cwd_;

    cwd_ = startDirectory;

    BOOL created = CreateProcessW(
        nullptr,
        commandLine,
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        startDirectory.c_str(),
        &si,
        &pi);

    CloseHandle(stdinRead);
    CloseHandle(stdoutWrite);

    if (!created)
    {
        CloseHandle(stdoutRead);
        CloseHandle(stdinWrite);
        AppendLine(L"[error] Could not start cmd.exe");
        return false;
    }

    childProcess_ = pi.hProcess;
    childThread_ = pi.hThread;
    childStdoutRead_ = stdoutRead;
    childStdinWrite_ = stdinWrite;

    readerRunning_ = true;
    readerThread_ = std::thread(&App::ReaderLoop, this);

    SendRaw(L"chcp 65001>nul\r\n");
    return true;
}

void App::StopShell()
{
    readerRunning_ = false;

    if (childStdinWrite_)
    {
        const char* exitCommand = "exit\r\n";
        DWORD written = 0;
        WriteFile(childStdinWrite_, exitCommand,
            static_cast<DWORD>(strlen(exitCommand)), &written, nullptr);
    }

    if (childProcess_)
    {
        WaitForSingleObject(childProcess_, 300);
        TerminateProcess(childProcess_, 0);
    }

    if (childStdoutRead_)
    {
        CloseHandle(childStdoutRead_);
        childStdoutRead_ = nullptr;
    }

    if (readerThread_.joinable()) readerThread_.join();

    if (childStdinWrite_) CloseHandle(childStdinWrite_);
    if (childThread_) CloseHandle(childThread_);
    if (childProcess_) CloseHandle(childProcess_);

    childStdinWrite_ = nullptr;
    childThread_ = nullptr;
    childProcess_ = nullptr;
}

void App::ReaderLoop()
{
    std::string carry;
    char buffer[4096];

    while (readerRunning_)
    {
        DWORD read = 0;
        BOOL ok = ReadFile(childStdoutRead_, buffer, sizeof(buffer), &read, nullptr);
        if (!ok || read == 0) break;

        carry.append(buffer, buffer + read);
        std::wstring wide = Utf8ToWide(carry);
        if (!wide.empty())
        {
            {
                std::lock_guard<std::mutex> lock(outputMutex_);
                pendingOutput_ += wide;
            }
            carry.clear();
            PostMessageW(hwnd_, WM_TERMINAL_OUTPUT, 0, 0);
        }
    }
}

void App::SendRaw(const std::wstring& text)
{
    if (!childStdinWrite_) return;
    std::string bytes = WideToUtf8(text);
    DWORD written = 0;
    WriteFile(childStdinWrite_, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr);
}

void App::ExecuteInput()
{
    if (input_.empty())
        return;

    std::wstring command = input_;

    undoStack_.clear();
    input_.clear();
    caretPosition_ = 0;
    selectionAnchor_ = 0;
    selecting_ = false;
    // Preserve the user's current scrollbar/view position.

    // If a running command requests keyboard input, send the line directly.
    if (commandRunning_)
    {
        SendRaw(command + L"\r\n");
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }

    history_.push_back(command);

    if (
        Customize::CommandHistoryLimit > 0 &&
        history_.size() >
        static_cast<std::size_t>(Customize::CommandHistoryLimit))
    {
        history_.erase(history_.begin());
    }

    historyIndex_ = static_cast<int>(history_.size());

    std::wstring trimmed = command;

    while (!trimmed.empty() && iswspace(trimmed.front()))
        trimmed.erase(trimmed.begin());

    while (!trimmed.empty() && iswspace(trimmed.back()))
        trimmed.pop_back();

    if (trimmed.empty())
        return;

    std::wstring lower = trimmed;

    std::transform(
        lower.begin(),
        lower.end(),
        lower.begin(),
        [](wchar_t character)
        {
            return static_cast<wchar_t>(towlower(character));
        });

    // Commands that operate on the visible terminal itself.
    if (lower == L"cls" || lower == L"clear")
    {
        lines_.clear();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }

    // Custom beginner-friendly HELP guide.
    // Use "help native" to open CMD's original built-in help list.
    if (lower == L"help")
    {
        const std::vector<std::wstring> guide =
        {
            L"MAOTAW TERMINAL - COMMAND GUIDE",
            L"Type a command exactly like the examples below, then press Enter.",
            L"Use quotation marks around names or paths containing spaces.",
            L"",
            L"NAVIGATION",
            L"  cd FolderName",
            L"    Open a folder inside the current directory.",
            L"    Example: cd Projects",
            L"",
            L"  cd ..",
            L"    Move one folder back.",
            L"",
            L"  cd /d \"D:\\My Files\"",
            L"    Move to a full path, including a different drive.",
            L"",
            L"  cd \\",
            L"    Move to the root of the current drive.",
            L"",
            L"  dir",
            L"    Show files and folders in the current directory.",
            L"    Useful: dir /a     shows hidden items",
            L"    Useful: dir /s     includes subfolders",
            L"    Useful: dir /b     names only",
            L"",
            L"CREATE FILES AND FOLDERS",
            L"  mkdir FolderName",
            L"    Create a new folder.",
            L"    Example: mkdir MyProject",
            L"",
            L"  mkdir \"My New Folder\"",
            L"    Create a folder whose name contains spaces.",
            L"",
            L"  type nul > file.txt",
            L"    Create an empty file.",
            L"",
            L"  echo Hello world > file.txt",
            L"    Create or overwrite a text file with content.",
            L"",
            L"  echo Another line >> file.txt",
            L"    Add text to the end of an existing file.",
            L"",
            L"  notepad file.txt",
            L"    Open or create a text file in Notepad.",
            L"",
            L"READ AND VIEW",
            L"  type file.txt",
            L"    Print a text file inside the terminal.",
            L"",
            L"  tree",
            L"    Show the folder structure.",
            L"",
            L"  tree /f",
            L"    Show folders and files in a tree.",
            L"",
            L"COPY, MOVE AND RENAME",
            L"  copy source.txt backup.txt",
            L"    Copy one file.",
            L"",
            L"  copy \"C:\\Path One\\file.txt\" \"D:\\Backup\\file.txt\"",
            L"    Copy using complete paths.",
            L"",
            L"  xcopy SourceFolder DestinationFolder /e /i",
            L"    Copy a folder and all of its subfolders.",
            L"",
            L"  robocopy SourceFolder DestinationFolder /e",
            L"    Reliably copy a complete folder tree.",
            L"",
            L"  move file.txt FolderName",
            L"    Move a file into a folder.",
            L"",
            L"  ren oldname.txt newname.txt",
            L"    Rename a file.",
            L"",
            L"DELETE",
            L"  del file.txt",
            L"    Delete one file.",
            L"",
            L"  del /q *.tmp",
            L"    Delete all matching files without confirmation.",
            L"",
            L"  rmdir EmptyFolder",
            L"    Remove an empty folder.",
            L"",
            L"  rmdir /s /q FolderName",
            L"    Delete a folder and everything inside it.",
            L"    Warning: this does not move items to the Recycle Bin.",
            L"",
            L"SEARCH",
            L"  dir /s /b filename.txt",
            L"    Search for a file below the current folder.",
            L"",
            L"  where program.exe",
            L"    Find where an installed command or program is located.",
            L"",
            L"  findstr /s /i \"word\" *.txt",
            L"    Search for text inside .txt files and subfolders.",
            L"",
            L"SYSTEM AND PROCESSES",
            L"  systeminfo",
            L"    Show detailed Windows and hardware information.",
            L"",
            L"  tasklist",
            L"    Show running processes.",
            L"",
            L"  taskkill /im app.exe /f",
            L"    Force-close a process by its executable name.",
            L"    Example: taskkill /im notepad.exe /f",
            L"",
            L"  sfc /scannow",
            L"    Scan and repair protected Windows system files.",
            L"    Run the terminal as administrator.",
            L"",
            L"  chkdsk C:",
            L"    Check a drive for file-system problems.",
            L"",
            L"NETWORK",
            L"  ipconfig",
            L"    Show basic network configuration.",
            L"",
            L"  ipconfig /all",
            L"    Show detailed adapter and DNS information.",
            L"",
            L"  ping google.com",
            L"    Test whether a server is reachable.",
            L"",
            L"  ping -t google.com",
            L"    Keep pinging until you press Ctrl+C.",
            L"",
            L"  tracert google.com",
            L"    Show the network route to a server.",
            L"",
            L"  netstat -ano",
            L"    Show network connections, ports and process IDs.",
            L"",
            L"  nslookup example.com",
            L"    Look up DNS information for a domain.",
            L"",
            L"POWER AND WINDOWS",
            L"  shutdown /s /t 60",
            L"    Shut down Windows after 60 seconds.",
            L"",
            L"  shutdown /r /t 0",
            L"    Restart Windows immediately.",
            L"",
            L"  shutdown /a",
            L"    Cancel a scheduled shutdown or restart.",
            L"",
            L"  start .",
            L"    Open the current directory in File Explorer.",
            L"",
            L"  start \"\" \"file.txt\"",
            L"    Open a file with its default application.",
            L"",
            L"TERMINAL CONTROLS",
            L"  cls",
            L"    Clear this custom terminal.",
            L"",
            L"  title My Terminal",
            L"    Change the title-bar text.",
            L"",
            L"  color 0A",
            L"    Change background and text using CMD color codes.",
            L"",
            L"  time",
            L"    Display the current time without changing it.",
            L"",
            L"  date",
            L"    Display the current date without changing it.",
            L"",
            L"  Ctrl+C",
            L"    Copy selected input, or stop a running command.",
            L"",
            L"  Ctrl+V or right-click",
            L"    Paste clipboard text.",
            L"",
            L"WINDOWS EXPLORER INTEGRATION",
            L"  installmenu",
            L"    Add 'Open in Maotaw Terminal' to Explorer right-click menus.",
            L"    Works on folders, drives and empty folder background space.",
            L"",
            L"  removemenu",
            L"    Remove the Explorer right-click entries.",
            L"",
            L"  MaotawTerminal.exe --dir \"C:\\Projects\"",
            L"    Launch the terminal directly in a chosen directory.",
            L"",
            L"BUILT-IN EDITOR",
            L"  edit filename",
            L"    Open or create a file inside Maotaw Terminal.",
            L"    Examples: edit main.cpp   edit script.py   edit app.js",
            L"    Ctrl+S saves, Esc closes, Tab inserts spaces.",
            L"    Keywords, functions, strings, comments and numbers use colors.",
            L"",
            L"DEVELOPER TOOLS",
            L"  checktools",
            L"    Detect Python, Cargo, Git, Node, npm, Rust, Java and CMake.",
            L"    It also shows the executable path Windows found.",
            L"",
            L"  refreshenv",
            L"    Restart the internal CMD and reload the latest Windows PATH.",
            L"",
            L"  python --version",
            L"    Automatically uses python, py or python3 when available.",
            L"",
            L"  python script.py",
            L"    Run a Python script using the detected launcher.",
            L"",
            L"  cargo --version",
            L"    Check whether Rust Cargo is available from PATH.",
            L"",
            L"MORE HELP",
            L"  help native",
            L"    Display the original Windows CMD command list.",
            L"",
            L"  command /?",
            L"    Show full help for one command.",
            L"    Examples: copy /?     dir /?     robocopy /?",
            L"",
            L"Safety: be careful with del, rmdir /s /q, format and shutdown."
        };

        for (const std::wstring& helpLine : guide)
            AppendLine(helpLine);

        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }

    if (lower == L"help native")
    {
        command = L"help";
    }

    if (lower == L"edit")
    {
        AppendLine(L"Usage: edit filename");
        AppendLine(L"Example: edit main.cpp");
        return;
    }

    if (lower.rfind(L"edit ", 0) == 0)
    {
        OpenEditor(trimmed.substr(5));
        return;
    }

    if (lower == L"installmenu")
    {
        InstallExplorerMenu();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }

    if (lower == L"removemenu")
    {
        RemoveExplorerMenu();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }

    if (lower == L"checktools")
    {
        PrintToolCheck();
        return;
    }

    if (lower == L"refreshenv")
    {
        RestartShell();
        return;
    }

    if (
        lower == L"python" ||
        lower.rfind(L"python ", 0) == 0)
    {
        RefreshDetectedTools();

        if (
            !detectedPythonCommand_.empty() &&
            detectedPythonCommand_ != L"python")
        {
            command =
                detectedPythonCommand_ +
                trimmed.substr(6);
        }
    }

    if (lower == L"title" || lower.rfind(L"title ", 0) == 0)
    {
        std::wstring newTitle =
            trimmed.size() > 5
            ? trimmed.substr(5)
            : L"Maotaw Terminal";

        while (!newTitle.empty() && iswspace(newTitle.front()))
            newTitle.erase(newTitle.begin());

        if (newTitle.empty())
            newTitle = L"Maotaw Terminal";

        windowTitle_ = newTitle;
        SetWindowTextW(hwnd_, windowTitle_.c_str());
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }

    if (lower == L"color" || lower.rfind(L"color ", 0) == 0)
    {
        static const D2D1_COLOR_F colors[16] =
        {
            Color(0x000000), Color(0x000080), Color(0x008000), Color(0x008080),
            Color(0x800000), Color(0x800080), Color(0x808000), Color(0xC0C0C0),
            Color(0x808080), Color(0x0000FF), Color(0x00FF00), Color(0x00FFFF),
            Color(0xFF0000), Color(0xFF00FF), Color(0xFFFF00), Color(0xFFFFFF)
        };

        std::wstring argument =
            trimmed.size() > 5
            ? trimmed.substr(5)
            : L"07";

        while (!argument.empty() && iswspace(argument.front()))
            argument.erase(argument.begin());

        if (argument.empty())
            argument = L"07";

        auto hexValue = [](wchar_t character) -> int
            {
                character = static_cast<wchar_t>(towupper(character));

                if (character >= L'0' && character <= L'9')
                    return character - L'0';

                if (character >= L'A' && character <= L'F')
                    return 10 + character - L'A';

                return -1;
            };

        if (argument.size() >= 2)
        {
            const int background = hexValue(argument[0]);
            const int foreground = hexValue(argument[1]);

            if (
                background >= 0 &&
                foreground >= 0 &&
                background != foreground)
            {
                terminalBackground_ = colors[background];
                terminalForeground_ = colors[foreground];
                InvalidateRect(hwnd_, nullptr, FALSE);
                return;
            }
        }

        AppendLine(
            L"The color command requires two different hexadecimal digits.");
        return;
    }

    if (lower == L"exit")
    {
        exiting_ = true;
        RemoveTrayIcon();
        DestroyWindow(hwnd_);
        return;
    }

    // Plain TIME and DATE normally ask the user to enter a new value.
    // In this custom terminal they should only display the current value,
    // so the next typed line remains a new command instead of prompt input.
    if (lower == L"time")
        command = L"time /t";
    else if (lower == L"date")
        command = L"date /t";

    // Show exactly what was executed.
    AppendLine(L"> " + trimmed);

    commandRunning_ = true;

    // Send the command to the persistent cmd.exe process.
    //
    // This preserves native CMD behavior for:
    // dir, tree, mkdir/md, rmdir/rd, del/erase, copy, xcopy, robocopy,
    // move, ren/rename, type, attrib, mklink, fsutil, echo redirection,
    // pipes, &&, ||, environment variables, batch files, pushd and popd.
    SendRaw(command + L"\r\n");

    // Emit the completion marker and the real current directory on one line.
    // %CD% is expanded by the persistent cmd.exe process.
    SendRaw(
        L"echo __MAOTAW_CMD_DONE__%CD%\r\n");

    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::AppendLine(const std::wstring& line)
{
    lines_.push_back(line);
    while (lines_.size() > 1200) lines_.pop_front();
}

void App::ProcessShellOutput(const std::wstring& text)
{
    std::wstring cleaned = text;

    size_t marker = 0;
    while ((marker = cleaned.find(L"__MAOTAW_CMD_DONE__")) != std::wstring::npos)
    {
        size_t lineEnd = cleaned.find(L'\n', marker);
        std::wstring markerLine = cleaned.substr(marker,
            lineEnd == std::wstring::npos ? std::wstring::npos : lineEnd - marker);

        std::wstring newCwd = markerLine.substr(wcslen(L"__MAOTAW_CMD_DONE__"));
        while (!newCwd.empty() && (newCwd.back() == L'\r' || newCwd.back() == L'\n' || newCwd.back() == L' '))
            newCwd.pop_back();
        while (!newCwd.empty() && newCwd.front() == L' ')
            newCwd.erase(newCwd.begin());
        const bool drivePath =
            newCwd.size() >= 3 &&
            iswalpha(newCwd[0]) &&
            newCwd[1] == L':' &&
            (newCwd[2] == L'\\' || newCwd[2] == L'/');

        const bool uncPath =
            newCwd.size() >= 2 &&
            newCwd[0] == L'\\' &&
            newCwd[1] == L'\\';

        if (drivePath || uncPath)
            cwd_ = newCwd;

        size_t eraseCount = lineEnd == std::wstring::npos ? cleaned.size() - marker : lineEnd - marker + 1;
        cleaned.erase(marker, eraseCount);
        commandRunning_ = false;
    }

    for (const std::wstring& line : SplitLines(cleaned))
    {
        std::wstring trimmed = line;
        while (!trimmed.empty() && trimmed.front() == L' ') trimmed.erase(trimmed.begin());
        if (!trimmed.empty()) AppendLine(trimmed);
    }
}

void App::AddBanner()
{
    // Clean startup: no fake output or banner.
}

std::wstring App::PromptLabel() const
{
    if (!cwd_.empty())
        return cwd_;

    return ResolveStartDirectory();
}

void App::SaveUndoSnapshot()
{
    if (restoringUndo_)
        return;

    InputSnapshot snapshot{};
    snapshot.text = input_;
    snapshot.caret = caretPosition_;
    snapshot.selectionAnchor = selectionAnchor_;
    snapshot.selecting = selecting_;

    if (!undoStack_.empty())
    {
        const InputSnapshot& previous = undoStack_.back();

        if (
            previous.text == snapshot.text &&
            previous.caret == snapshot.caret &&
            previous.selectionAnchor == snapshot.selectionAnchor &&
            previous.selecting == snapshot.selecting)
        {
            return;
        }
    }

    undoStack_.push_back(std::move(snapshot));

    constexpr std::size_t MaximumUndoSteps = 200;

    if (undoStack_.size() > MaximumUndoSteps)
        undoStack_.erase(undoStack_.begin());
}

void App::UndoInput()
{
    if (undoStack_.empty())
        return;

    restoringUndo_ = true;

    const InputSnapshot snapshot = undoStack_.back();
    undoStack_.pop_back();

    input_ = snapshot.text;
    caretPosition_ = std::min(snapshot.caret, input_.size());
    selectionAnchor_ = std::min(
        snapshot.selectionAnchor,
        input_.size());

    selecting_ =
        snapshot.selecting &&
        caretPosition_ != selectionAnchor_;

    restoringUndo_ = false;

    InvalidateRect(hwnd_, nullptr, FALSE);
}

bool App::HasSelection() const
{
    return selecting_ && caretPosition_ != selectionAnchor_;
}

std::pair<std::size_t, std::size_t> App::SelectionRange() const
{
    return {
        std::min(caretPosition_, selectionAnchor_),
        std::max(caretPosition_, selectionAnchor_)
    };
}

void App::ClearSelection()
{
    selecting_ = false;
    selectionAnchor_ = caretPosition_;
}

void App::SelectAllInput()
{
    selectionAnchor_ = 0;
    caretPosition_ = input_.size();
    selecting_ = !input_.empty();
}

void App::DeleteSelection()
{
    if (!HasSelection())
        return;

    const auto [begin, end] = SelectionRange();
    input_.erase(begin, end - begin);
    caretPosition_ = begin;
    ClearSelection();
}

void App::InsertInputText(const std::wstring& text)
{
    if (text.empty())
        return;

    SaveUndoSnapshot();
    DeleteSelection();
    caretPosition_ = std::min(caretPosition_, input_.size());
    input_.insert(caretPosition_, text);
    caretPosition_ += text.size();
    ClearSelection();
}

void App::MoveCaret(std::size_t position, bool keepSelection)
{
    position = std::min(position, input_.size());

    if (keepSelection)
    {
        if (!selecting_)
        {
            selectionAnchor_ = caretPosition_;
            selecting_ = true;
        }
    }
    else
    {
        selecting_ = false;
        selectionAnchor_ = position;
    }

    caretPosition_ = position;

    if (selecting_ && caretPosition_ == selectionAnchor_)
        selecting_ = false;
}

bool App::CopyToClipboard(const std::wstring& text)
{
    if (text.empty())
        return false;

    if (!OpenClipboard(hwnd_))
        return false;

    EmptyClipboard();

    const SIZE_T byteCount = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, byteCount);

    if (!memory)
    {
        CloseClipboard();
        return false;
    }

    void* destination = GlobalLock(memory);
    if (!destination)
    {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }

    memcpy(destination, text.c_str(), byteCount);
    GlobalUnlock(memory);

    if (!SetClipboardData(CF_UNICODETEXT, memory))
    {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return true;
}

std::wstring App::PasteFromClipboard()
{
    std::wstring result;

    if (!OpenClipboard(hwnd_))
        return result;

    HANDLE data = GetClipboardData(CF_UNICODETEXT);
    if (data)
    {
        const wchar_t* text =
            static_cast<const wchar_t*>(GlobalLock(data));

        if (text)
        {
            result = text;
            GlobalUnlock(data);
        }
    }

    CloseClipboard();

    // Keep pasted content on the current command line.
    for (wchar_t& character : result)
    {
        if (character == L'\r' || character == L'\n' || character == L'\t')
            character = L' ';
    }

    return result;
}

float App::MeasureTextWidth(const std::wstring& text, IDWriteTextFormat* format)
{
    if (!writeFactory_ || text.empty()) return 0.0f;

    IDWriteTextLayout* layout = nullptr;
    if (FAILED(writeFactory_->CreateTextLayout(
        text.c_str(), static_cast<UINT32>(text.size()), format,
        5000.0f, 40.0f, &layout)))
        return 0.0f;

    DWRITE_TEXT_METRICS metrics{};
    layout->GetMetrics(&metrics);
    SafeRelease(layout);
    return metrics.widthIncludingTrailingWhitespace;
}

void App::SetBrush(const D2D1_COLOR_F& color)
{
    brush_->SetColor(color);
}

void App::FillRect(float l, float t, float r, float b, const D2D1_COLOR_F& color)
{
    SetBrush(color);
    target_->FillRectangle(D2D1::RectF(l, t, r, b), brush_);
}

void App::FillRound(float l, float t, float r, float b, float radius, const D2D1_COLOR_F& color)
{
    SetBrush(color);
    target_->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(l, t, r, b), radius, radius), brush_);
}

void App::DrawRound(float l, float t, float r, float b, float radius, const D2D1_COLOR_F& color, float stroke)
{
    SetBrush(color);
    target_->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(l, t, r, b), radius, radius), brush_, stroke);
}

void App::DrawLine(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, float stroke)
{
    SetBrush(color);
    target_->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush_, stroke);
}

void App::FillPowerSegment(float x, float y, float w, float h,
    const D2D1_COLOR_F& color, bool leftNotch, bool rightPoint)
{
    ID2D1PathGeometry* geometry = nullptr;
    ID2D1GeometrySink* sink = nullptr;
    if (FAILED(d2dFactory_->CreatePathGeometry(&geometry))) return;
    if (FAILED(geometry->Open(&sink))) { SafeRelease(geometry); return; }

    const float notch = 13.0f;
    sink->BeginFigure(D2D1::Point2F(x + (leftNotch ? notch : 0.0f), y), D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLine(D2D1::Point2F(x + w - (rightPoint ? notch : 0.0f), y));
    if (rightPoint) sink->AddLine(D2D1::Point2F(x + w, y + h * 0.5f));
    sink->AddLine(D2D1::Point2F(x + w - (rightPoint ? notch : 0.0f), y + h));
    sink->AddLine(D2D1::Point2F(x + (leftNotch ? notch : 0.0f), y + h));
    if (leftNotch) sink->AddLine(D2D1::Point2F(x, y + h * 0.5f));
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    sink->Close();

    SetBrush(color);
    target_->FillGeometry(geometry, brush_);
    SafeRelease(sink);
    SafeRelease(geometry);
}

void App::DrawTextLine(const std::wstring& text, float l, float t, float r, float b,
    const D2D1_COLOR_F& color, IDWriteTextFormat* format, DWRITE_TEXT_ALIGNMENT alignment)
{
    format->SetTextAlignment(alignment);
    SetBrush(color);
    target_->DrawTextW(text.c_str(), static_cast<UINT32>(text.size()), format,
        D2D1::RectF(l, t, r, b), brush_, D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

void App::Render()
{
    if (!CreateResources())
        return;

    RECT rc{};
    GetClientRect(hwnd_, &rc);

    const float width = static_cast<float>(rc.right);
    const float height = static_cast<float>(rc.bottom);

    const D2D1_COLOR_F titleBar = Color(Customize::TitleBarColor);
    const D2D1_COLOR_F titleBarHover = Color(0x24272B);
    const D2D1_COLOR_F terminalBlack = terminalBackground_;
    const D2D1_COLOR_F border = Color(Customize::BorderColor);
    const D2D1_COLOR_F normalText = terminalForeground_;
    const D2D1_COLOR_F mutedText = Color(Customize::MutedTextColor);
    const D2D1_COLOR_F promptGreen = Color(Customize::AccentColor);
    const D2D1_COLOR_F promptCyan = Color(Customize::DirectoryColor);
    const D2D1_COLOR_F errorText = Color(Customize::ErrorTextColor);
    const D2D1_COLOR_F cursorColor = Color(Customize::AccentColor);

    target_->BeginDraw();
    target_->Clear(terminalBlack);

    if (editorMode_)
    {
        RenderEditor(width, height);

        HRESULT editorResult = target_->EndDraw();

        if (editorResult == D2DERR_RECREATE_TARGET)
            DiscardResources();

        return;
    }

    // Modern compact title bar.
    FillRect(
        0.0f,
        0.0f,
        width,
        static_cast<float>(kTitleHeight),
        titleBar);

    DrawLine(
        0.0f,
        static_cast<float>(kTitleHeight),
        width,
        static_cast<float>(kTitleHeight),
        Color(0x25282C),
        1.0f);

    // Clean compact title: icon only, then "Maotaw Terminal".
    const float iconLeft = 12.0f;
    const float iconTop = 8.0f;
    const float iconSize = 16.0f;

    // No icon border and no background plate.
    FillRound(
        iconLeft,
        iconTop,
        iconLeft + iconSize,
        iconTop + iconSize,
        2.0f,
        Color(Customize::AccentColor));

    DrawTextLine(
        L">_",
        iconLeft,
        iconTop,
        iconLeft + iconSize,
        iconTop + iconSize,
        Color(0x101214),
        smallFont_,
        DWRITE_TEXT_ALIGNMENT_CENTER);

    DrawTextLine(
        Customize::AppTitle,
        iconLeft + iconSize + 7.0f,
        0.0f,
        220.0f,
        static_cast<float>(kTitleHeight),
        Color(0xE8EAED),
        titleFont_);

    const bool minimizeHovered =
        mouse_.x >= width - 78.0f &&
        mouse_.x < width - 40.0f &&
        mouse_.y >= 0 &&
        mouse_.y < kTitleHeight;

    const bool closeHovered =
        mouse_.x >= width - 40.0f &&
        mouse_.y >= 0 &&
        mouse_.y < kTitleHeight;

    if (minimizeHovered)
    {
        FillRect(
            width - 78.0f,
            0.0f,
            width - 40.0f,
            static_cast<float>(kTitleHeight),
            titleBarHover);
    }

    if (closeHovered)
    {
        FillRect(
            width - 40.0f,
            0.0f,
            width,
            static_cast<float>(kTitleHeight),
            Color(0xB52E37));
    }

    DrawLine(
        width - 65.0f,
        16.0f,
        width - 53.0f,
        16.0f,
        Color(0xF1F2F3),
        1.5f);

    DrawLine(
        width - 27.0f,
        10.0f,
        width - 15.0f,
        22.0f,
        Color(0xF1F2F3),
        1.5f);

    DrawLine(
        width - 15.0f,
        10.0f,
        width - 27.0f,
        22.0f,
        Color(0xF1F2F3),
        1.5f);

    // Deep black translucent terminal body.
    FillRect(
        0.0f,
        static_cast<float>(kTitleHeight),
        width,
        height,
        terminalBlack);

    DrawRound(
        0.5f,
        0.5f,
        width - 0.5f,
        height - 0.5f,
        9.0f,
        border,
        1.0f);

    const float left = 17.0f;
    const float rightPadding = 18.0f;
    const float contentTop = static_cast<float>(kTitleHeight) + 14.0f;
    const float lineHeight = 20.0f;

    outputTextLeft_ = left;
    outputTop_ = contentTop;
    outputLineHeight_ = lineHeight;

    const int totalOutputRows = static_cast<int>(lines_.size());
    const int totalRowsWithPrompt = totalOutputRows + 1;

    // Normal behavior: the prompt follows the newest output, exactly as before.
    const int fullVisibleRows = std::max(
        1,
        static_cast<int>((height - contentTop - 10.0f) / lineHeight));

    normalBottomStart_ = std::max(
        0,
        totalRowsWithPrompt - fullVisibleRows);

    scrollOffset_ = std::clamp(
        scrollOffset_,
        0,
        normalBottomStart_);

    const int normalViewStart =
        normalBottomStart_ - scrollOffset_;

    const int normalPromptScreenRow =
        totalOutputRows - normalViewStart;

    floatingPromptVisible_ =
        normalPromptScreenRow < 0 ||
        normalPromptScreenRow >= fullVisibleRows;

    // Only reserve a bottom row when scrolling has hidden the normal prompt.
    const int outputRowsAvailable = floatingPromptVisible_
        ? std::max(1, fullVisibleRows - 1)
        : fullVisibleRows;

    outputVisibleRows_ = outputRowsAvailable;
    visibleRows_ = outputRowsAvailable;
    outputBottom_ =
        contentTop +
        static_cast<float>(outputRowsAvailable) * lineHeight;

    const int outputBottomStart = std::max(
        0,
        totalOutputRows - outputRowsAvailable);

    const int viewStart = floatingPromptVisible_
        ? std::clamp(
            outputBottomStart - scrollOffset_,
            0,
            outputBottomStart)
        : normalViewStart;

    viewStartRow_ = viewStart;
    outputFirstVisibleRow_ = viewStart;
    maxViewStart_ = outputBottomStart;
    virtualRows_ = std::max(totalOutputRows, outputRowsAvailable);

    OutputPoint selectionFirst = outputSelectionAnchor_;
    OutputPoint selectionLast = outputSelectionCurrent_;

    if (
        selectionFirst.row > selectionLast.row ||
        (selectionFirst.row == selectionLast.row &&
            selectionFirst.column > selectionLast.column))
    {
        std::swap(selectionFirst, selectionLast);
    }

    for (int screenRow = 0; screenRow < outputRowsAvailable; ++screenRow)
    {
        const int bufferRow = viewStart + screenRow;
        const float rowY = contentTop + screenRow * lineHeight;

        if (bufferRow >= totalOutputRows)
            break;

        const std::wstring& line =
            lines_[static_cast<std::size_t>(bufferRow)];

        // Character-level selection background.
        if (
            outputHasSelection_ &&
            bufferRow >= selectionFirst.row &&
            bufferRow <= selectionLast.row)
        {
            const std::size_t begin =
                bufferRow == selectionFirst.row
                ? std::min(selectionFirst.column, line.size())
                : 0;

            const std::size_t finish =
                bufferRow == selectionLast.row
                ? std::min(selectionLast.column, line.size())
                : line.size();

            if (finish > begin)
            {
                const float selectionLeft =
                    left +
                    MeasureTextWidth(
                        line.substr(0, begin),
                        terminalFont_);

                const float selectionRight =
                    left +
                    MeasureTextWidth(
                        line.substr(0, finish),
                        terminalFont_);

                FillRound(
                    selectionLeft,
                    rowY,
                    selectionRight,
                    rowY + lineHeight,
                    2.0f,
                    Color(Customize::SelectionColor));
            }
        }

        D2D1_COLOR_F lineColor = normalText;

        if (
            line.find(L"error") != std::wstring::npos ||
            line.find(L"Error") != std::wstring::npos ||
            line.find(L"not recognized") != std::wstring::npos)
        {
            lineColor = errorText;
        }
        else if (
            line.rfind(L"Volume", 0) == 0 ||
            line.rfind(L"Directory", 0) == 0)
        {
            lineColor = mutedText;
        }

        DrawTextLine(
            line,
            left,
            rowY,
            width - rightPadding,
            rowY + lineHeight,
            lineColor,
            terminalFont_);
    }

    // Draw the normal prompt in its natural position when visible.
    float promptY = contentTop +
        static_cast<float>(normalPromptScreenRow) * lineHeight;

    if (floatingPromptVisible_)
    {
        // The user scrolled far enough that the normal prompt left the screen.
        // Show a temporary typing row at the bottom without changing history.
        promptY = height - lineHeight - 8.0f;

        FillRect(
            0.0f,
            promptY - 4.0f,
            width,
            height,
            Color(Customize::BackgroundColor));

        DrawLine(
            0.0f,
            promptY - 4.0f,
            width,
            promptY - 4.0f,
            Color(Customize::BorderColor),
            1.0f);
    }

    const std::wstring fullDirectory = PromptLabel();
    float directoryX = left;

    if (Customize::ShowHomeIcon)
    {
        const float iconLeft = left;
        const float iconTop = promptY + 3.0f;
        const float iconWidth = 14.0f;
        const float roofY = iconTop;
        const float wallTop = iconTop + 6.0f;
        const float wallBottom = iconTop + 15.0f;
        const float centerX = iconLeft + iconWidth * 0.5f;

        DrawLine(iconLeft, wallTop, centerX, roofY, promptGreen, 1.8f);
        DrawLine(centerX, roofY, iconLeft + iconWidth, wallTop, promptGreen, 1.8f);
        DrawLine(iconLeft + 2.0f, wallTop - 1.0f, iconLeft + 2.0f, wallBottom, promptGreen, 1.8f);
        DrawLine(iconLeft + iconWidth - 2.0f, wallTop - 1.0f, iconLeft + iconWidth - 2.0f, wallBottom, promptGreen, 1.8f);
        DrawLine(iconLeft + 2.0f, wallBottom, iconLeft + iconWidth - 2.0f, wallBottom, promptGreen, 1.8f);
        DrawLine(centerX - 2.0f, wallBottom, centerX - 2.0f, wallBottom - 5.0f, promptGreen, 1.4f);
        DrawLine(centerX + 2.0f, wallBottom, centerX + 2.0f, wallBottom - 5.0f, promptGreen, 1.4f);

        directoryX =
            iconLeft +
            iconWidth +
            Customize::PromptSpacing;
    }

    DrawTextLine(
        fullDirectory,
        directoryX,
        promptY - 2.0f,
        width - rightPadding,
        promptY + lineHeight,
        promptCyan,
        terminalFont_);

    const float directoryRight =
        directoryX +
        MeasureTextWidth(fullDirectory, terminalFont_);

    const float separatorX =
        directoryRight +
        Customize::PromptSpacing;

    DrawTextLine(
        Customize::PromptSeparator,
        separatorX,
        promptY - 2.0f,
        separatorX + 20.0f,
        promptY + lineHeight,
        promptGreen,
        terminalFont_);

    const float inputX =
        separatorX +
        MeasureTextWidth(
            Customize::PromptSeparator,
            terminalFont_) +
        Customize::PromptSpacing;

    if (HasSelection())
    {
        const auto [begin, finish] = SelectionRange();

        const float selectionLeft =
            inputX +
            MeasureTextWidth(
                input_.substr(0, begin),
                terminalFont_);

        const float selectionRight =
            selectionLeft +
            MeasureTextWidth(
                input_.substr(begin, finish - begin),
                terminalFont_);

        FillRound(
            selectionLeft,
            promptY,
            selectionRight,
            promptY + lineHeight,
            2.0f,
            Color(Customize::SelectionColor));
    }

    DrawTextLine(
        input_,
        inputX,
        promptY - 2.0f,
        width - rightPadding,
        promptY + lineHeight,
        normalText,
        terminalFont_);

    if (caretVisible_ && GetFocus() == hwnd_)
    {
        const float caretX =
            inputX +
            MeasureTextWidth(
                input_.substr(
                    0,
                    std::min(caretPosition_, input_.size())),
                terminalFont_) +
            1.0f;

        DrawRound(
            caretX,
            promptY + 2.0f,
            caretX + 7.0f,
            promptY + lineHeight - 2.0f,
            0.0f,
            cursorColor,
            1.0f);
    }

    // Slim custom scrollbar.
    const float scrollbarX = width - Customize::ScrollbarWidth - 3.0f;
    scrollbarTrackTop_ = outputTop_;
    scrollbarTrackBottom_ = outputBottom_;

    const float trackHeight =
        std::max(1.0f, scrollbarTrackBottom_ - scrollbarTrackTop_);

    FillRound(
        scrollbarX,
        scrollbarTrackTop_,
        scrollbarX + Customize::ScrollbarWidth * 0.5f,
        scrollbarTrackBottom_,
        1.0f,
        Color(Customize::ScrollbarTrackColor));

    const float visibleRatio = std::clamp(
        static_cast<float>(visibleRows_) /
        static_cast<float>(std::max(1, virtualRows_)),
        0.0f,
        1.0f);

    const float thumbHeight =
        std::clamp(trackHeight * visibleRatio, 24.0f, trackHeight);

    const float travel = std::max(0.0f, trackHeight - thumbHeight);
    const float positionRatio =
        maxViewStart_ > 0
        ? static_cast<float>(viewStartRow_) /
        static_cast<float>(maxViewStart_)
        : 0.0f;

    scrollbarThumbTop_ =
        scrollbarTrackTop_ + travel * positionRatio;
    scrollbarThumbBottom_ =
        scrollbarThumbTop_ + thumbHeight;

    FillRound(
        scrollbarX,
        scrollbarThumbTop_,
        scrollbarX + Customize::ScrollbarWidth,
        scrollbarThumbBottom_,
        2.0f,
        scrollbarDragging_
        ? Color(Customize::ScrollbarActiveColor)
        : Color(Customize::ScrollbarThumbColor));

    if (contextMenuOpen_)
    {
        const bool hovered =
            mouse_.x >= contextMenuX_ &&
            mouse_.x <= contextMenuX_ + contextMenuWidth_ &&
            mouse_.y >= contextMenuY_ &&
            mouse_.y <= contextMenuY_ + contextMenuHeight_;

        const D2D1_COLOR_F menuBackground =
            Color(Customize::TitleBarColor);

        const D2D1_COLOR_F menuHover =
            Color(0x20240F);

        const D2D1_COLOR_F menuBorder =
            Color(Customize::BorderColor);

        FillRound(
            contextMenuX_,
            contextMenuY_,
            contextMenuX_ + contextMenuWidth_,
            contextMenuY_ + contextMenuHeight_,
            1.0f,
            hovered ? menuHover : menuBackground);

        DrawRound(
            contextMenuX_,
            contextMenuY_,
            contextMenuX_ + contextMenuWidth_,
            contextMenuY_ + contextMenuHeight_,
            1.0f,
            menuBorder,
            1.0f);

        DrawTextLine(
            contextMenuCopyMode_ ? L"Copy" : L"Paste",
            contextMenuX_ + 10.0f,
            contextMenuY_,
            contextMenuX_ + contextMenuWidth_ - 8.0f,
            contextMenuY_ + contextMenuHeight_,
            Color(Customize::AccentColor),
            smallFont_);
    }

    HRESULT result = target_->EndDraw();
    if (result == D2DERR_RECREATE_TARGET)
        DiscardResources();
}

LRESULT CALLBACK App::StaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    App* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE)
    {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        app->hwnd_ = hwnd;
    }

    return app ? app->Proc(message, wParam, lParam)
        : DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT App::Proc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SYSCOMMAND:
        if (
            Customize::MinimizeToTray &&
            (wParam & 0xFFF0) == SC_MINIMIZE)
        {
            ShowWindow(hwnd_, SW_HIDE);
            return 0;
        }
        break;

    case WM_TRAYICON:
    {
        const UINT eventCode = LOWORD(lParam);

        if (
            eventCode == WM_LBUTTONDBLCLK ||
            eventCode == NIN_SELECT ||
            eventCode == NIN_KEYSELECT)
        {
            RestoreFromTray();
            return 0;
        }

        if (eventCode == WM_CONTEXTMENU)
        {
            ShowTrayMenu();
            return 0;
        }

        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TRAY_OPEN)
        {
            RestoreFromTray();
            return 0;
        }

        if (LOWORD(wParam) == ID_TRAY_EXIT)
        {
            exiting_ = true;
            RemoveTrayIcon();
            DestroyWindow(hwnd_);
            return 0;
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        BeginPaint(hwnd_, &ps);
        Render();
        EndPaint(hwnd_, &ps);
        return 0;
    }

    case WM_SIZE:
        if (target_)
            target_->Resize(D2D1::SizeU(LOWORD(lParam), HIWORD(lParam)));
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;

    case WM_MOUSEMOVE:
        mouse_.x = GET_X_LPARAM(lParam);
        mouse_.y = GET_Y_LPARAM(lParam);

        if (editorMode_ && editorSelecting_)
        {
            EditorPoint point =
                EditorPointFromMouse(
                    static_cast<float>(mouse_.x),
                    static_cast<float>(mouse_.y));

            if (mouse_.y < editorTextTop_)
            {
                editorScrollRow_ = std::max(0, editorScrollRow_ - 1);
                point = EditorPointFromMouse(
                    static_cast<float>(mouse_.x),
                    editorTextTop_ + 1.0f);
            }
            else if (mouse_.y >= editorTextBottom_)
            {
                const int maxScroll = std::max(
                    0,
                    static_cast<int>(editorLines_.size()) -
                    editorVisibleRows_);

                editorScrollRow_ = std::min(
                    maxScroll,
                    editorScrollRow_ + 1);

                point = EditorPointFromMouse(
                    static_cast<float>(mouse_.x),
                    editorTextBottom_ - 1.0f);
            }

            editorSelectionCurrent_ = point;
            editorHasSelection_ =
                editorSelectionAnchor_.row != point.row ||
                editorSelectionAnchor_.column != point.column;

            editorRow_ = point.row;
            editorColumn_ = point.column;

            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (outputSelecting_)
        {
            OutputPoint point = OutputPointFromMouse(
                static_cast<float>(mouse_.x),
                static_cast<float>(mouse_.y));

            // When dragging above/below the output area, continue selecting
            // from the first or last visible character.
            if (point.row < 0 && !lines_.empty())
            {
                if (mouse_.y < outputTop_)
                {
                    point.row = outputFirstVisibleRow_;
                    point.column = 0;
                }
                else if (mouse_.y >= outputBottom_)
                {
                    point.row = std::min(
                        static_cast<int>(lines_.size()) - 1,
                        outputFirstVisibleRow_ +
                        outputVisibleRows_ - 1);

                    point.column =
                        lines_[static_cast<std::size_t>(point.row)].size();
                }
            }

            if (point.row >= 0)
            {
                outputSelectionCurrent_ = point;
                outputHasSelection_ = true;
            }
        }

        if (scrollbarDragging_)
        {
            const float thumbHeight =
                scrollbarThumbBottom_ - scrollbarThumbTop_;

            const float travel = std::max(
                1.0f,
                (scrollbarTrackBottom_ - scrollbarTrackTop_) -
                thumbHeight);

            const float newThumbTop = std::clamp(
                static_cast<float>(mouse_.y) - scrollbarDragOffset_,
                scrollbarTrackTop_,
                scrollbarTrackBottom_ - thumbHeight);

            const float ratio =
                (newThumbTop - scrollbarTrackTop_) / travel;

            const int newViewStart = static_cast<int>(
                std::round(ratio * maxViewStart_));

            if (newViewStart <= normalBottomStart_)
            {
                scrollOffset_ = normalBottomStart_ - newViewStart;
                bottomOverscroll_ = 0;
            }
            else
            {
                scrollOffset_ = 0;
                bottomOverscroll_ = newViewStart - normalBottomStart_;
            }
        }

        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDOWN:
    {
        SetFocus(hwnd_);
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        if (editorMode_)
        {
            if (
                y >= editorTextTop_ &&
                y < editorTextBottom_)
            {
                const EditorPoint point =
                    EditorPointFromMouse(
                        static_cast<float>(x),
                        static_cast<float>(y));

                editorRow_ = point.row;
                editorColumn_ = point.column;

                editorSelecting_ = true;
                editorHasSelection_ = false;
                editorSelectionAnchor_ = point;
                editorSelectionCurrent_ = point;

                SetCapture(hwnd_);
                KeepEditorCursorVisible();
                InvalidateRect(hwnd_, nullptr, FALSE);
            }

            return 0;
        }

        if (contextMenuOpen_)
        {
            if (ContextMenuHit(
                static_cast<float>(x),
                static_cast<float>(y)))
            {
                if (contextMenuCopyMode_)
                {
                    CopyToClipboard(SelectedOutputText());
                }
                else
                {
                    InsertInputText(PasteFromClipboard());
                }

                CloseContextMenu();
                return 0;
            }

            CloseContextMenu();
        }

        RECT rc{};
        GetClientRect(hwnd_, &rc);

        const OutputPoint outputPoint =
            OutputPointFromMouse(
                static_cast<float>(x),
                static_cast<float>(y));

        if (outputPoint.row >= 0)
        {
            outputSelecting_ = true;
            outputHasSelection_ = true;
            outputSelectionAnchor_ = outputPoint;
            outputSelectionCurrent_ = outputPoint;
            SetCapture(hwnd_);
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (
            x >= rc.right - 12 &&
            y >= scrollbarTrackTop_ &&
            y <= scrollbarTrackBottom_)
        {
            scrollbarDragging_ = true;
            SetCapture(hwnd_);

            if (
                y >= scrollbarThumbTop_ &&
                y <= scrollbarThumbBottom_)
            {
                scrollbarDragOffset_ =
                    static_cast<float>(y) - scrollbarThumbTop_;
            }
            else
            {
                scrollbarDragOffset_ =
                    (scrollbarThumbBottom_ - scrollbarThumbTop_) * 0.5f;
            }

            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (y < kTitleHeight)
        {
            if (x >= rc.right - 48)
                DestroyWindow(hwnd_);
            else if (x >= rc.right - 92)
                ShowWindow(hwnd_, SW_MINIMIZE);
            else
            {
                ReleaseCapture();
                SendMessageW(hwnd_, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            }
        }
        return 0;
    }

    case WM_LBUTTONUP:
        if (editorMode_ && editorSelecting_)
        {
            editorSelecting_ = false;
            ReleaseCapture();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (outputSelecting_)
        {
            outputSelecting_ = false;
            ReleaseCapture();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (scrollbarDragging_)
        {
            scrollbarDragging_ = false;
            ReleaseCapture();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }
        break;

    case WM_RBUTTONDOWN:
    {
        const float x =
            static_cast<float>(GET_X_LPARAM(lParam));

        const float y =
            static_cast<float>(GET_Y_LPARAM(lParam));

        const OutputPoint point =
            OutputPointFromMouse(x, y);

        if (
            outputHasSelection_ &&
            point.row >= 0)
        {
            OpenContextMenu(x, y, true);
        }
        else
        {
            OpenContextMenu(x, y, false);
        }

        return 0;
    }

    case WM_MOUSEWHEEL:
    {
        if (editorMode_)
        {
            const int delta =
                GET_WHEEL_DELTA_WPARAM(wParam);

            const int wheelNotches =
                delta / WHEEL_DELTA;

            const int rowsPerNotch = 4;

            const int maxScroll = std::max(
                0,
                static_cast<int>(editorLines_.size()) -
                std::max(1, editorVisibleRows_));

            editorScrollRow_ = std::clamp(
                editorScrollRow_ -
                wheelNotches * rowsPerNotch,
                0,
                maxScroll);

            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (contextMenuOpen_)
            CloseContextMenu();

        const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        const int step = Customize::ScrollWheelRows;

        if (delta > 0)
        {
            if (bottomOverscroll_ > 0)
            {
                bottomOverscroll_ =
                    std::max(0, bottomOverscroll_ - step);
            }
            else
            {
                scrollOffset_ = std::min(
                    normalBottomStart_,
                    scrollOffset_ + step);
            }
        }
        else
        {
            if (scrollOffset_ > 0)
            {
                scrollOffset_ =
                    std::max(0, scrollOffset_ - step);
            }
            else
            {
                const int maximumOverscroll =
                    std::max(0, maxViewStart_ - normalBottomStart_);

                bottomOverscroll_ = std::min(
                    maximumOverscroll,
                    bottomOverscroll_ + step);
            }
        }

        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    }

    case WM_CHAR:
        if (editorMode_)
        {
            const wchar_t character =
                static_cast<wchar_t>(wParam);

            if (character >= 32 && character != 127)
            {
                InsertEditorCharacter(character);
                InvalidateRect(hwnd_, nullptr, FALSE);
            }

            return 0;
        }

        if (true)
        {
            wchar_t ch = static_cast<wchar_t>(wParam);
            if (ch >= 32 && ch != 127)
            {
                InsertInputText(std::wstring(1, ch));
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
        }
        return 0;

    case WM_KEYDOWN:
    {
        const bool controlDown =
            (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool shiftDown =
            (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (editorMode_)
        {
            if (controlDown && wParam == 'C')
            {
                if (EditorHasSelection())
                    CopyToClipboard(SelectedEditorText());

                return 0;
            }

            if (controlDown && wParam == 'X')
            {
                if (EditorHasSelection())
                {
                    CopyToClipboard(SelectedEditorText());
                    DeleteEditorSelection();
                    InvalidateRect(hwnd_, nullptr, FALSE);
                }

                return 0;
            }

            if (controlDown && wParam == 'A')
            {
                editorSelectionAnchor_ = { 0, 0 };
                editorSelectionCurrent_ =
                {
                    editorLines_.size() - 1,
                    editorLines_.back().size()
                };

                editorHasSelection_ = true;
                editorSelecting_ = false;
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (controlDown && wParam == 'S')
            {
                SaveEditor();
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (controlDown && wParam == 'V')
            {
                const std::wstring pasted = PasteFromClipboard();

                for (wchar_t character : pasted)
                {
                    if (character == L'\r')
                        continue;

                    if (character == L'\n')
                        InsertEditorNewLine();
                    else
                        InsertEditorCharacter(character);
                }

                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (wParam == VK_ESCAPE)
            {
                if (editorDirty_)
                {
                    editorStatus_ =
                        L"Unsaved changes - Ctrl+S to save, Esc again to discard";

                    static bool confirmDiscard = false;

                    if (confirmDiscard)
                    {
                        confirmDiscard = false;
                        CloseEditor();
                    }
                    else
                    {
                        confirmDiscard = true;
                    }
                }
                else
                {
                    CloseEditor();
                }

                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (wParam == VK_RETURN)
            {
                InsertEditorNewLine();
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (wParam == VK_TAB)
            {
                for (int index = 0; index < Customize::EditorTabSize; ++index)
                    InsertEditorCharacter(L' ');

                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (wParam == VK_BACK)
            {
                EditorBackspace();
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (wParam == VK_DELETE)
            {
                EditorDelete();
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (wParam == VK_LEFT)
            {
                if (editorColumn_ > 0)
                {
                    --editorColumn_;
                }
                else if (editorRow_ > 0)
                {
                    --editorRow_;
                    editorColumn_ = editorLines_[editorRow_].size();
                }

                KeepEditorCursorVisible();
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (wParam == VK_RIGHT)
            {
                if (editorColumn_ < editorLines_[editorRow_].size())
                {
                    ++editorColumn_;
                }
                else if (editorRow_ + 1 < editorLines_.size())
                {
                    ++editorRow_;
                    editorColumn_ = 0;
                }

                KeepEditorCursorVisible();
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (wParam == VK_UP)
            {
                if (editorRow_ > 0)
                    --editorRow_;

                editorColumn_ = std::min(
                    editorColumn_,
                    editorLines_[editorRow_].size());

                KeepEditorCursorVisible();
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (wParam == VK_DOWN)
            {
                if (editorRow_ + 1 < editorLines_.size())
                    ++editorRow_;

                editorColumn_ = std::min(
                    editorColumn_,
                    editorLines_[editorRow_].size());

                KeepEditorCursorVisible();
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (wParam == VK_HOME)
            {
                editorColumn_ = 0;
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (wParam == VK_END)
            {
                editorColumn_ =
                    editorLines_[editorRow_].size();

                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (wParam == VK_PRIOR)
            {
                editorRow_ = static_cast<std::size_t>(
                    std::max(
                        0,
                        static_cast<int>(editorRow_) - 20));

                editorColumn_ = std::min(
                    editorColumn_,
                    editorLines_[editorRow_].size());

                KeepEditorCursorVisible();
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (wParam == VK_NEXT)
            {
                editorRow_ = std::min(
                    editorLines_.size() - 1,
                    editorRow_ + 20);

                editorColumn_ = std::min(
                    editorColumn_,
                    editorLines_[editorRow_].size());

                KeepEditorCursorVisible();
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            return 0;
        }

        if (controlDown && wParam == 'V')
        {
            InsertInputText(PasteFromClipboard());
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (shiftDown && wParam == VK_INSERT)
        {
            InsertInputText(PasteFromClipboard());
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (controlDown && wParam == 'Z')
        {
            UndoInput();
            return 0;
        }

        if (controlDown && wParam == 'A')
        {
            SelectAllInput();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (controlDown && wParam == 'C')
        {
            if (outputHasSelection_)
            {
                CopyToClipboard(SelectedOutputText());
            }
            else if (HasSelection())
            {
                const auto [begin, end] = SelectionRange();
                CopyToClipboard(input_.substr(begin, end - begin));
            }
            else if (commandRunning_)
            {
                SendRaw(std::wstring(1, static_cast<wchar_t>(3)));
                AppendLine(L"^C");
            }
            else
            {
                CopyToClipboard(input_);
            }
            return 0;
        }

        if (wParam == VK_RETURN)
        {
            ExecuteInput();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (wParam == VK_BACK)
        {
            if (HasSelection())
            {
                SaveUndoSnapshot();
                DeleteSelection();
            }
            else if (caretPosition_ > 0)
            {
                SaveUndoSnapshot();
                input_.erase(caretPosition_ - 1, 1);
                --caretPosition_;
                ClearSelection();
            }

            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (wParam == VK_DELETE)
        {
            if (HasSelection())
            {
                SaveUndoSnapshot();
                DeleteSelection();
            }
            else if (caretPosition_ < input_.size())
            {
                SaveUndoSnapshot();
                input_.erase(caretPosition_, 1);
                ClearSelection();
            }

            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (wParam == VK_LEFT)
        {
            if (!shiftDown && HasSelection())
            {
                const auto [begin, end] = SelectionRange();
                MoveCaret(begin, false);
            }
            else
            {
                MoveCaret(
                    caretPosition_ > 0 ? caretPosition_ - 1 : 0,
                    shiftDown);
            }

            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (wParam == VK_RIGHT)
        {
            if (!shiftDown && HasSelection())
            {
                const auto [begin, end] = SelectionRange();
                MoveCaret(end, false);
            }
            else
            {
                MoveCaret(
                    std::min(caretPosition_ + 1, input_.size()),
                    shiftDown);
            }

            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (wParam == VK_HOME)
        {
            MoveCaret(0, shiftDown);
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (wParam == VK_END)
        {
            MoveCaret(input_.size(), shiftDown);
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (!commandRunning_ && wParam == VK_UP && !history_.empty())
        {
            historyIndex_ = std::max(0, historyIndex_ - 1);
            input_ = history_[static_cast<size_t>(historyIndex_)];
            caretPosition_ = input_.size();
            ClearSelection();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (!commandRunning_ && wParam == VK_DOWN && !history_.empty())
        {
            historyIndex_ = std::min(static_cast<int>(history_.size()), historyIndex_ + 1);
            input_ = historyIndex_ == static_cast<int>(history_.size())
                ? L""
                : history_[static_cast<size_t>(historyIndex_)];
            caretPosition_ = input_.size();
            ClearSelection();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }

        if (wParam == VK_ESCAPE)
        {
            if (contextMenuOpen_)
            {
                CloseContextMenu();
                return 0;
            }

            if (outputHasSelection_)
            {
                ClearOutputSelection();
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            if (!input_.empty())
                SaveUndoSnapshot();

            input_.clear();
            caretPosition_ = 0;
            ClearSelection();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }
        break;
    }

    case WM_TERMINAL_OUTPUT:
    {
        std::wstring output;
        {
            std::lock_guard<std::mutex> lock(outputMutex_);
            output.swap(pendingOutput_);
        }
        ProcessShellOutput(output);
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    }

    case WM_TIMER:
        if (wParam == kTimerId)
        {
            caretVisible_ = !caretVisible_;
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd_, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    int argumentCount = 0;
    LPWSTR* arguments = CommandLineToArgvW(
        GetCommandLineW(),
        &argumentCount);

    if (arguments)
    {
        for (int index = 1; index < argumentCount; ++index)
        {
            const std::wstring argument = arguments[index];

            if (
                (argument == L"--dir" || argument == L"-d") &&
                index + 1 < argumentCount)
            {
                gStartupDirectoryOverride = arguments[++index];
            }
        }

        LocalFree(arguments);
    }

    App app;
    if (!app.Initialize(instance))
    {
        MessageBoxW(nullptr,
            L"Could not initialize the custom terminal.",
            Customize::AppTitle,
            MB_ICONERROR);
        return 1;
    }

    int result = app.Run();
    app.Shutdown();
    return result;
}
