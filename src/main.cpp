#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#include <gdiplus.h>

#include <cstring>

#include "word_generator.h"

namespace {

using namespace Gdiplus;

constexpr wchar_t kTitle[] = L"诡词生成器";
constexpr wchar_t kWindowClass[] = L"WeirdWordsWindow";
constexpr wchar_t kShortcutHint[] = L"Enter 再生成  ·  Ctrl+C 复制全部";
constexpr wchar_t kGenerateLabel[] = L"再生成";
constexpr wchar_t kCopyLabel[] = L"复制全部";
constexpr wchar_t kBodyFont[] = L"Microsoft YaHei UI";
constexpr wchar_t kDisplayFont[] = L"SimSun";
constexpr int kGenerate = 1010;
constexpr int kCopy = 1011;
constexpr int kStatus = 1012;
constexpr int kFirstWord = 1020;
constexpr UINT_PTR kStatusTimer = 1;
constexpr int kDefaultWidth = 808;
constexpr int kDefaultHeight = 552;
constexpr int kMinimumWidth = 674;
constexpr int kMinimumHeight = 512;

const Color kCanvas(0xfa, 0xf9, 0xf5);
const Color kInk(0x14, 0x14, 0x13);
const Color kMuted(0x6c, 0x6a, 0x64);
const Color kCoral(0xcc, 0x78, 0x5c);
const Color kCoralPressed(0xa9, 0x58, 0x3e);
const Color kHairline(0xe6, 0xdf, 0xd8);
const Color kSoft(0xf5, 0xf0, 0xe8);
const Color kPanel(0x18, 0x17, 0x15);
const Color kTile(0x25, 0x23, 0x20);
const Color kTilePressed(0x35, 0x30, 0x29);
const Color kTileIcon(0xa0, 0x9d, 0x96);
const Color kSuccess(0x3f, 0x6b, 0x4c);
const Color kError(0xa6, 0x3d, 0x35);

struct App {
    HWND window = nullptr;
    HWND generate = nullptr;
    HWND copy = nullptr;
    HWND status = nullptr;
    std::array<HWND, 4> words{};
    UINT dpi = USER_DEFAULT_SCREEN_DPI;
    std::mt19937 random{std::random_device{}()};
    weird_words::Batch batch;
    Color statusColor = kMuted;
};

struct Layout {
    int left;
    int top;
    int width;
};

int Pixels(int logical, UINT dpi) {
    return MulDiv(logical, dpi, USER_DEFAULT_SCREEN_DPI);
}

Layout GetLayout(const App& app) {
    RECT client{};
    GetClientRect(app.window, &client);
    const int width = MulDiv(client.right, USER_DEFAULT_SCREEN_DPI, app.dpi);
    const int height = MulDiv(client.bottom, USER_DEFAULT_SCREEN_DPI, app.dpi);
    const int contentWidth = std::min(width - 88, 860);
    return {(width - contentWidth) / 2, (height - 436) / 2, contentWidth};
}

void PositionControl(HWND control, UINT dpi, int x, int y, int width, int height) {
    const int left = Pixels(x, dpi);
    const int top = Pixels(y, dpi);
    SetWindowPos(control, nullptr, left, top,
                 Pixels(x + width, dpi) - left, Pixels(y + height, dpi) - top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void ArrangeControls(const App& app) {
    const auto [left, top, width] = GetLayout(app);
    constexpr int inset = 18;
    constexpr int gap = 12;
    const int firstColumnWidth = (width - 2 * inset - gap) / 2;
    const int secondColumnWidth = width - 2 * inset - gap - firstColumnWidth;
    for (std::size_t index = 0; index < app.words.size(); ++index) {
        const bool secondColumn = index % 2 == 1;
        const int x = left + inset + (secondColumn ? firstColumnWidth + gap : 0);
        const int y = top + 134 + static_cast<int>(index / 2) * 108;
        PositionControl(app.words[index], app.dpi, x, y,
                        secondColumn ? secondColumnWidth : firstColumnWidth, 96);
    }
    PositionControl(app.generate, app.dpi, left, top + 388, 144, 48);
    PositionControl(app.copy, app.dpi, left + 156, top + 388, 134, 48);
    PositionControl(app.status, app.dpi, left + 306, top + 388, width - 306, 48);
}

RectF Box(int x, int y, int width, int height) {
    return {static_cast<float>(x), static_cast<float>(y),
            static_cast<float>(width), static_cast<float>(height)};
}

void RoundedPath(GraphicsPath& path, const RectF& box, float radius) {
    const float diameter = radius * 2;
    path.AddArc(box.X, box.Y, diameter, diameter, 180, 90);
    path.AddArc(box.GetRight() - diameter, box.Y, diameter, diameter, 270, 90);
    path.AddArc(box.GetRight() - diameter, box.GetBottom() - diameter, diameter, diameter, 0, 90);
    path.AddArc(box.X, box.GetBottom() - diameter, diameter, diameter, 90, 90);
    path.CloseFigure();
}

void FillRounded(Graphics& graphics, const RectF& box, float radius, const Color& color) {
    GraphicsPath path;
    RoundedPath(path, box, radius);
    SolidBrush brush(color);
    graphics.FillPath(&brush, &path);
}

void StrokeRounded(Graphics& graphics, const RectF& box, float radius, const Color& color,
                   float width = 1.0f) {
    GraphicsPath path;
    RoundedPath(path, box, radius);
    Pen pen(color, width);
    graphics.DrawPath(&pen, &path);
}

void PaintText(Graphics& graphics, std::wstring_view text, const RectF& box,
               const wchar_t* family, float size, const Color& color,
               StringAlignment alignment = StringAlignmentNear) {
    Font font(family, size, FontStyleRegular, UnitPixel);
    SolidBrush brush(color);
    StringFormat format;
    format.SetFormatFlags(StringFormatFlagsNoWrap);
    format.SetAlignment(alignment);
    format.SetLineAlignment(StringAlignmentCenter);
    graphics.DrawString(text.data(), static_cast<INT>(text.size()), &font, box, &format, &brush);
}

template <typename Painter>
void PaintBuffered(HDC dc, const RECT& bounds, UINT dpi, const Color& background, Painter paint) {
    const int width = bounds.right - bounds.left;
    const int height = bounds.bottom - bounds.top;
    if (width <= 0 || height <= 0) {
        return;
    }
    Bitmap buffer(width, height, PixelFormat32bppPARGB);
    if (buffer.GetLastStatus() != Ok) {
        return;
    }
    const float scale = static_cast<float>(dpi) / USER_DEFAULT_SCREEN_DPI;
    {
        Graphics graphics(&buffer);
        graphics.Clear(background);
        graphics.SetPageUnit(UnitPixel);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
        graphics.ScaleTransform(scale, scale);
        paint(graphics, width / scale, height / scale);
    }
    Graphics output(dc);
    output.DrawImage(&buffer, bounds.left, bounds.top, width, height);
}

void PaintGlyph(Graphics& graphics, bool regenerate, float x, float y,
                const Color& ink, const Color& background) {
    if (!regenerate) {
        StrokeRounded(graphics, {x + 5, y + 1, 10, 10}, 1.8f, ink, 1.4f);
        FillRounded(graphics, {x + 1, y + 5, 10, 10}, 1.8f, background);
        StrokeRounded(graphics, {x + 1, y + 5, 10, 10}, 1.8f, ink, 1.4f);
        return;
    }

    Pen pen(ink, 1.55f);
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    pen.SetLineJoin(LineJoinRound);
    graphics.DrawArc(&pen, x + 1, y + 1, 14.0f, 14.0f, 38, 286);
    const PointF arrow[] = {{x + 14, y + 0.9f}, {x + 14, y + 4.4f}, {x + 10.4f, y + 4.4f}};
    graphics.DrawLines(&pen, arrow, static_cast<INT>(std::size(arrow)));
}

void PaintButton(const App& app, const DRAWITEMSTRUCT& item, bool generate) {
    const bool pressed = (item.itemState & ODS_SELECTED) != 0;
    const auto fill = generate ? (pressed ? kCoralPressed : kCoral) : (pressed ? kSoft : kCanvas);
    const auto ink = generate && pressed ? kCanvas : kInk;
    const std::wstring_view label = generate ? kGenerateLabel : kCopyLabel;
    PaintBuffered(item.hDC, item.rcItem, app.dpi, kCanvas, [&](Graphics& graphics, float width, float height) {
        const RectF button(0.5f, 0.5f, width - 1, height - 1);
        FillRounded(graphics, button, 8, fill);
        if (!generate) {
            StrokeRounded(graphics, button, 8, kHairline);
        }
        const float labelWidth = static_cast<float>(label.size()) * 16;
        const float groupStart = (width - (16 + 10 + labelWidth)) / 2;
        PaintGlyph(graphics, generate, groupStart, (height - 16) / 2, ink, fill);
        PaintText(graphics, label, {groupStart + 14, 0, labelWidth + 24, height},
                  kBodyFont, 16, ink, StringAlignmentCenter);
        if ((item.itemState & ODS_FOCUS) && !(item.itemState & ODS_NOFOCUSRECT)) {
            StrokeRounded(graphics, {3.0f, 3.0f, width - 6, height - 6}, 6, ink, 1.2f);
        }
    });
}

void PaintWord(const App& app, const DRAWITEMSTRUCT& item) {
    const auto& word = app.batch[item.CtlID - kFirstWord];
    if (word.empty()) {
        return;
    }
    PaintBuffered(item.hDC, item.rcItem, app.dpi, kPanel, [&](Graphics& graphics, float width, float height) {
        const auto fill = (item.itemState & ODS_SELECTED) ? kTilePressed : kTile;
        FillRounded(graphics, {0, 0, width, height}, 10, fill);
        PaintGlyph(graphics, false, width - 30, 10, kTileIcon, fill);
        const float fontSize = std::min(36.0f, (width - 32) / (static_cast<float>(word.size()) + 0.5f));
        PaintText(graphics, word, {0, 4, width, height - 4}, kDisplayFont, fontSize, kCanvas, StringAlignmentCenter);
        if ((item.itemState & ODS_FOCUS) && !(item.itemState & ODS_NOFOCUSRECT)) {
            StrokeRounded(graphics, {3.0f, 3.0f, width - 6, height - 6}, 7, kTileIcon, 1.2f);
        }
    });
}

void PaintStatus(const App& app, const DRAWITEMSTRUCT& item) {
    wchar_t status[64]{};
    GetWindowTextW(app.status, status, static_cast<int>(std::size(status)));
    PaintBuffered(item.hDC, item.rcItem, app.dpi, kCanvas, [&](Graphics& graphics, float width, float height) {
        PaintText(graphics, status, {0, 0, width, height}, kBodyFont, 13, app.statusColor, StringAlignmentFar);
    });
}

void PaintWindow(const App& app) {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(app.window, &paint);
    RECT client{};
    GetClientRect(app.window, &client);
    const auto layout = GetLayout(app);
    PaintBuffered(dc, client, app.dpi, kCanvas, [&](Graphics& graphics, float, float) {
        PaintText(graphics, kTitle, Box(layout.left - 4, layout.top, layout.width, 61), kDisplayFont, 54, kInk);
        PaintText(graphics, L"用 27 个字随机拼词，偶尔混入一个熟词。",
                  Box(layout.left - 1, layout.top + 66, layout.width, 28), kBodyFont, 16, kMuted);
        FillRounded(graphics, Box(layout.left, layout.top + 116, layout.width, 240), 16, kPanel);
    });
    EndPaint(app.window, &paint);
}

void SetStatus(App& app, const wchar_t* text, const Color& color) {
    app.statusColor = color;
    SetWindowTextW(app.status, text);
    InvalidateRect(app.status, nullptr, FALSE);
}

void RestoreHint(App& app) {
    KillTimer(app.window, kStatusTimer);
    SetStatus(app, kShortcutHint, kMuted);
}

void Generate(App& app) {
    app.batch = weird_words::GenerateBatch(app.random);
    for (std::size_t index = 0; index < app.words.size(); ++index) {
        const auto label = L"复制「" + app.batch[index] + L"」";
        SetWindowTextW(app.words[index], label.c_str());
        InvalidateRect(app.words[index], nullptr, FALSE);
    }
    RestoreHint(app);
}

bool CopyToClipboard(HWND owner, const std::wstring& text) {
    const auto bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!memory) {
        return false;
    }
    auto* data = GlobalLock(memory);
    if (!data) {
        GlobalFree(memory);
        return false;
    }
    std::memcpy(data, text.c_str(), bytes);
    GlobalUnlock(memory);

    bool copied = false;
    if (OpenClipboard(owner)) {
        copied = EmptyClipboard() && SetClipboardData(CF_UNICODETEXT, memory);
        CloseClipboard();
    }
    if (!copied) {
        GlobalFree(memory);
    }
    return copied;
}

void CopyWithFeedback(App& app, const std::wstring& text, const wchar_t* successMessage) {
    const bool copied = CopyToClipboard(app.window, text);
    SetStatus(app, copied ? successMessage : L"暂时无法复制，请再试一次", copied ? kSuccess : kError);
    SetTimer(app.window, kStatusTimer, 3200, nullptr);
}

void CopyBatch(App& app) {
    std::wstring text = app.batch.front();
    for (std::size_t index = 1; index < app.batch.size(); ++index) {
        text += L"\r\n";
        text += app.batch[index];
    }
    CopyWithFeedback(app, text, L"已复制 4 条结果");
}

HWND CreateControl(const App& app, const wchar_t* type, const wchar_t* label, DWORD style, int id) {
    return CreateWindowExW(0, type, label, WS_CHILD | WS_VISIBLE | style,
                           0, 0, 0, 0, app.window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                           GetModuleHandleW(nullptr), nullptr);
}

bool CreateControls(App& app) {
    for (std::size_t index = 0; index < app.words.size(); ++index) {
        app.words[index] = CreateControl(app, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP,
                                         kFirstWord + static_cast<int>(index));
    }
    app.generate = CreateControl(app, L"BUTTON", kGenerateLabel, BS_OWNERDRAW | WS_TABSTOP, kGenerate);
    app.copy = CreateControl(app, L"BUTTON", kCopyLabel, BS_OWNERDRAW | WS_TABSTOP, kCopy);
    app.status = CreateControl(app, L"STATIC", kShortcutHint, SS_OWNERDRAW, kStatus);
    return app.generate && app.copy && app.status &&
           std::all_of(app.words.begin(), app.words.end(), [](HWND word) { return word != nullptr; });
}

LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        auto* app = static_cast<App*>(create->lpCreateParams);
        app->window = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    auto* app = reinterpret_cast<App*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (!app) {
        return DefWindowProcW(window, message, wParam, lParam);
    }

    switch (message) {
    case WM_CREATE: {
        app->dpi = GetDpiForWindow(window);
        if (!CreateControls(*app)) {
            return -1;
        }
        constexpr COLORREF captionColor = RGB(0xfa, 0xf9, 0xf5);
        constexpr COLORREF captionText = RGB(0x14, 0x14, 0x13);
        DwmSetWindowAttribute(window, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
        DwmSetWindowAttribute(window, DWMWA_TEXT_COLOR, &captionText, sizeof(captionText));
        ArrangeControls(*app);
        Generate(*app);
        return 0;
    }
    case WM_SIZE:
        if (app->status && wParam != SIZE_MINIMIZED) {
            ArrangeControls(*app);
        }
        return 0;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        RECT minimum{0, 0, Pixels(kMinimumWidth, app->dpi), Pixels(kMinimumHeight, app->dpi)};
        AdjustWindowRectExForDpi(&minimum, WS_OVERLAPPEDWINDOW, FALSE, 0, app->dpi);
        info->ptMinTrackSize = {minimum.right - minimum.left, minimum.bottom - minimum.top};
        return 0;
    }
    case WM_DPICHANGED: {
        app->dpi = HIWORD(wParam);
        const auto* suggested = reinterpret_cast<const RECT*>(lParam);
        SetWindowPos(window, nullptr, suggested->left, suggested->top,
                     suggested->right - suggested->left, suggested->bottom - suggested->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        ArrangeControls(*app);
        RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
        return 0;
    }
    case WM_COMMAND: {
        if (HIWORD(wParam) != BN_CLICKED) {
            break;
        }
        const UINT action = LOWORD(wParam);
        if (action == kGenerate) {
            Generate(*app);
            return 0;
        }
        if (action == kCopy) {
            CopyBatch(*app);
            return 0;
        }
        if (action >= kFirstWord && action < kFirstWord + app->words.size()) {
            const auto& word = app->batch[action - kFirstWord];
            const auto feedback = L"已复制「" + word + L"」";
            CopyWithFeedback(*app, word, feedback.c_str());
            return 0;
        }
        break;
    }
    case WM_TIMER:
        if (wParam == kStatusTimer) {
            RestoreHint(*app);
            return 0;
        }
        break;
    case WM_DRAWITEM: {
        auto* item = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
        if (item->CtlID == kGenerate || item->CtlID == kCopy) {
            PaintButton(*app, *item, item->CtlID == kGenerate);
        } else if (item->CtlID == kStatus) {
            PaintStatus(*app, *item);
        } else if (item->CtlID >= kFirstWord && item->CtlID < kFirstWord + app->words.size()) {
            PaintWord(*app, *item);
        } else {
            break;
        }
        return TRUE;
    }
    case WM_PAINT:
        PaintWindow(*app);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

bool HandleShortcut(const App& app, const MSG& message) {
    if (message.message != WM_KEYDOWN) {
        return false;
    }
    const bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if (message.wParam == 'C' && control) {
        SendMessageW(app.window, WM_COMMAND, kCopy, 0);
        return true;
    }
    if (message.wParam == VK_RETURN && !control) {
        SendMessageW(app.window, WM_COMMAND, kGenerate, 0);
        return true;
    }
    return false;
}

int Run(HINSTANCE instance, int show) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(101));
    windowClass.hIconSm = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(101), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kWindowClass;
    if (!RegisterClassExW(&windowClass)) {
        return 1;
    }

    App app;
    app.dpi = GetDpiForSystem();
    RECT initial{0, 0, Pixels(kDefaultWidth, app.dpi), Pixels(kDefaultHeight, app.dpi)};
    AdjustWindowRectExForDpi(&initial, WS_OVERLAPPEDWINDOW, FALSE, 0, app.dpi);
    HWND window = CreateWindowExW(0, kWindowClass, kTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                  CW_USEDEFAULT, CW_USEDEFAULT, initial.right - initial.left,
                                  initial.bottom - initial.top, nullptr, nullptr, instance, &app);
    if (!window) {
        return 1;
    }
    ShowWindow(window, show);
    UpdateWindow(window);

    MSG message{};
    int status;
    while ((status = GetMessageW(&message, nullptr, 0, 0)) > 0) {
        if (!HandleShortcut(app, message) && !IsDialogMessageW(window, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    return status == -1 ? 1 : static_cast<int>(message.wParam);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    GdiplusStartupInput options;
    ULONG_PTR token = 0;
    if (GdiplusStartup(&token, &options, nullptr) != Ok) {
        return 1;
    }
    const int exitCode = Run(instance, show);
    GdiplusShutdown(token);
    return exitCode;
}
