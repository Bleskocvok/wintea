#include <windows.h>
#include <string>
#include <vector>
#include <stdexcept>        // runtime_error
#include <iostream>
#include <sstream>
#include <chrono>
#include <algorithm>        // clamp
#include <string_view>      // wstring_view
#include <map>

namespace ch = std::chrono;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI myMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine,
                  int nCmdShow);

#define CAJIK_ICON 1337

struct rgb_t { int r, g, b; };

struct style_t
{
    rgb_t fg, bg;
    std::string font;
};

struct layout_t
{
    int width, height;
    int dx, dy;
    int icon_size;

    style_t wait_style;
    style_t ready_style;
};

layout_t LAYOUT =
{
    .width = 300,
    .height = 100,
    .dx = -30,
    .dy = -50,
    .icon_size = 64,
    .wait_style =
    {
        .fg = rgb_t{ 255, 255, 255 },
        .bg = rgb_t{ 64, 64, 64 },
        .font = "Iosevka Curly Slab Extralight",
    },
    .ready_style =
    {
        .fg = rgb_t{ 255, 255, 255 },
        .bg = rgb_t{ 64, 164, 64 },
        // .font = "Comic Sans MS",
        .font = "Iosevka Curly Slab Extralight",
    },
};

struct data_t
{
    HFONT     fontTitleA = nullptr;
    HFONT     fontTitleB = nullptr;
    HFONT     fontDescA  = nullptr;
    HFONT     fontDescB  = nullptr;
    HICON     icon       = nullptr;
    UINT_PTR  timer      = 10;
    HINSTANCE hInstance  = nullptr;

    std::chrono::steady_clock::time_point start = {};

    int tea_time_ms = 0;
};

data_t data;

std::vector<std::wstring> arguments;

void cleanup(HWND hwnd, data_t& d)
{
    KillTimer(hwnd, d.timer);
    DeleteObject(d.fontTitleA);
    DeleteObject(d.fontDescA);
    DeleteObject(d.fontTitleB);
    DeleteObject(d.fontDescB);
}

void sys_err(const std::string& desc)
{
    auto err = GetLastError();

    LPVOID buf;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                | FORMAT_MESSAGE_FROM_SYSTEM
                | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, err, 0, (LPTSTR) &buf, 0, nullptr);

    std::stringstream str;
    str << desc << " (" << err << "): " << (const char*) buf;

    LocalFree(buf);

    throw std::runtime_error(str.str());
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR pCmdLine, int nCmdShow)
{
    try
    {
        return myMain(hInstance, hPrevInstance, pCmdLine, nCmdShow);
    }
    catch (std::exception& ex)
    {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        return 1;
    }
}

HICON get_icon(data_t& d, int i)
{
    // icon = LoadIcon(d.hInstance, MAKEINTRESOURCE(i));
    auto icon = (HICON) LoadImage(d.hInstance, MAKEINTRESOURCE(i),
                                  IMAGE_ICON, 256, 256, LR_SHARED);
    if (icon == nullptr) sys_err("LoadIcon");

    return icon;
}

HFONT load_font(int height, const std::string& name, bool bold = false)
{
    auto res = CreateFont(height,
                      0,                           // <-- width
                      0,
                      0,                           // <-- angle
                      bold ? FW_BOLD : FW_NORMAL,  // <-- weight
                      false,                       // <-- italic
                      false,                       // <-- underline
                      false,                       // <-- strikeout
                      DEFAULT_CHARSET,             // <-- charset
                      OUT_OUTLINE_PRECIS,          // <-- precision
                      CLIP_DEFAULT_PRECIS,         // <-- clip precision
                      CLEARTYPE_QUALITY,           // <-- quality
                      DEFAULT_PITCH | FF_DONTCARE, // <-- pitch, family
                      name.c_str());
    if (!res) std::cerr << "Font '" << name << "' not found\n";
    return res;
}

auto get_arguments()
{
    auto result = std::vector<std::wstring>{};

    LPWSTR* argv;
    int argc;
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == nullptr)
        sys_err("CommandLineToArgvW");

    for (int i = 0; i < argc; ++i)
        result.emplace_back(argv[i]);

    LocalFree(argv);

    return result;
}

auto parse_time(const std::wstring& str)
{
    std::wstring_view view = str;

    auto invalid = []()
    {
        return std::runtime_error("invalid time format, must be hh:mm:ss, "
                                  "e.g. 01:03:12 (one hour, three minutes, "
                                  "twelve seconds), or 5:00 (five minutes)");
    };

    auto eat = [&]()
    {
        if (view.empty()) throw invalid();
        auto c = view.front();
        view.remove_prefix(1);
        return c;
    };

    auto digit = [&](wchar_t c)
    {
        static auto digit_map = std::map<wchar_t, int>
        {
            {L'0', 0}, {L'1', 1}, {L'2', 2}, {L'3', 3}, {L'4', 4},
            {L'5', 5}, {L'6', 6}, {L'7', 7}, {L'8', 8}, {L'9', 9},
        };
        if (auto found = digit_map.find(c); found != digit_map.end())
            return found->second;
        throw invalid();
    };

    int result = 0;
    bool is_fst = true;

    while (!view.empty())
    {
        auto c = eat();

        if (c == L':')
        {
            if (is_fst) throw invalid();
            result *= 60;
            auto a = digit(eat());
            auto n = 10 * a + digit(eat());
            if (n >= 60) throw invalid();
            result += n;
        }
        else
        {
            is_fst = false;
            if (!view.empty() && view.front() != L':')
                result += 10 * digit(c) + digit(eat());
            else
                result += digit(c);
        }
    }

    return result;
}

int WINAPI myMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    arguments = get_arguments();

    data.hInstance = hInstance;
    data.tea_time_ms = 1000 * parse_time(arguments.at(1));
    data.start = std::chrono::steady_clock::now();

    const char app_class[] = "Tea Notification Class";

    WNDCLASS wc = {};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = app_class;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon         = get_icon(data, CAJIK_ICON);

    RegisterClass(&wc);

    int sys_w = GetSystemMetrics(SM_CXSCREEN);
    int sys_h = GetSystemMetrics(SM_CYSCREEN);

    int x = LAYOUT.dx >= 0 ? LAYOUT.dx : sys_w - LAYOUT.width  + LAYOUT.dx;
    int y = LAYOUT.dy >= 0 ? LAYOUT.dy : sys_h - LAYOUT.height + LAYOUT.dy;

    HWND hwnd = CreateWindowEx(0, // <-- ext style
                              app_class, "Tea Notification",
                              WS_OVERLAPPEDWINDOW, // <-- style
                              0, 0,
                              LAYOUT.width, LAYOUT.height,
                              nullptr, nullptr, hInstance, nullptr);

    if (hwnd == nullptr)
        return sys_err("CreateWindow"), 1;

    data.fontTitleA = load_font(22, LAYOUT.wait_style.font, true);
    data.fontDescA  = load_font(15, LAYOUT.wait_style.font);

    data.fontTitleB = load_font(22, LAYOUT.ready_style.font, true);
    data.fontDescB  = load_font(15, LAYOUT.ready_style.font);

    data.icon = get_icon(data, 10001);

    if (SetTimer(hwnd, data.timer, 100, (TIMERPROC) nullptr) == 0)
        return sys_err("LoadImage"), 1;

    auto prev = GetForegroundWindow();

    SetWindowLong(hwnd, GWL_STYLE, 0);
    SetWindowLong(hwnd, GWL_EXSTYLE, 0);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    SetWindowPos(hwnd, HWND_TOPMOST, x, y, LAYOUT.width, LAYOUT.height,
                 SWP_SHOWWINDOW);

    // Since the above “show window” call steals focus, let's restore
    // it back its the rightful holder. This results in one small
    // flicker of the previous window's border. It would be preferable
    // if it were possible not to steal focus with the above call, but
    // sadly this is the best solution I was able to find.
    SetForegroundWindow(prev);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        cleanup(hwnd, data);
        PostQuitMessage(0);
        return 0;

    case WM_LBUTTONDOWN:
        DestroyWindow(hwnd);
        return 0;

    case WM_TIMER:
        InvalidateRect(hwnd, nullptr, false);
        return 0;

    case WM_PAINT:
        {
            auto end = ch::steady_clock::now();
            auto elapsed = ch::duration_cast<ch::milliseconds>(end - data.start).count();

            int angle = std::clamp<long long>(elapsed * 360 / data.tea_time_ms, 1, 360);
            bool done = angle >= 360;

            // No need to free the previous, since it's loaded as „shared“.
            data.icon = get_icon(data, done ? CAJIK_ICON : 10000 + angle);

            auto& fontTitle = done ? data.fontTitleB : data.fontTitleA;
            auto& fontDesc  = done ? data.fontDescB : data.fontDescA;
            auto& bg = done ? LAYOUT.ready_style.bg
                            : LAYOUT.wait_style.bg;
            auto& fg = done ? LAYOUT.ready_style.fg
                            : LAYOUT.wait_style.fg;

            std::string title;
            std::string desc;
            if (done)
            {
                title = "Tea ready";
                desc = "Enjoy.";
            }
            else
            {
                int remains = data.tea_time_ms / 1000 - elapsed / 1000;
                title = "Tea brewing";
                desc = "Wait " + std::to_string(remains) + " s.";
            }


            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            SetBkColor(hdc, RGB(bg.r, bg.g, bg.b));

            HBRUSH brush = CreateSolidBrush(RGB(bg.r, bg.g, bg.b));
            FillRect(hdc, &ps.rcPaint, brush);

            SelectObject(hdc, fontTitle);
            SetTextColor(hdc, RGB(fg.r, fg.g, fg.b));
            RECT rect_title;
            SetRect(&rect_title, 70, 5, 295, 30);
            DrawText(hdc, title.c_str(), -1, &rect_title, DT_LEFT | DT_WORD_ELLIPSIS);

            SelectObject(hdc, fontDesc);
            SetTextColor(hdc, RGB(fg.r, fg.g, fg.b));
            RECT rect_desc;
            SetRect(&rect_desc, 70, 35, 295, 95);
            DrawText(hdc, desc.c_str(), -1, &rect_desc, DT_LEFT | DT_WORDBREAK);

            DrawIconEx(hdc, 0, 0, data.icon, LAYOUT.icon_size, LAYOUT.icon_size,
                       0, nullptr, DI_NORMAL);

            EndPaint(hwnd, &ps);
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
