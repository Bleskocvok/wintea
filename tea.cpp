#include <windows.h>
#include <string>           // stoi
#include <vector>
#include <stdexcept>        // runtime_error
#include <iostream>
#include <sstream>
#include <chrono>
#include <algorithm>        // clamp
#include <string_view>      // wstring_view, string_view
#include <sstream>          // stringstream
#include <map>

#include <wchar.h>          // fgetwc
#include <stdio.h>          // fopen, fclose
#include <errno.h>          // errno

#include <cstdlib>          // getenv

namespace ch = std::chrono;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI myMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine,
                  int nCmdShow);

#define CAJIK_ICON 1337
#define LOADING_ICON_START 10000

struct rgb_t { int r, g, b; };

struct rect_t { int x_left, y_top, x_right, y_bottom; };

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

    rect_t title_rect;
    rect_t desc_rect;
};

struct labels_t
{
    std::string wait_text;
    std::string ready_text;
    std::string ready_desc;
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
        .font = "",
    },
    .ready_style =
    {
        .fg = rgb_t{ 255, 255, 255 },
        .bg = rgb_t{ 64, 164, 64 },
        .font = "",
    },
    .title_rect =
    {
        .x_left = 70,
        .y_top = 5,
        .x_right = 295,
        .y_bottom = 30,
    },
    .desc_rect =
    {
        .x_left = 70,
        .y_top = 35,
        .x_right = 295,
        .y_bottom = 95,
    },
};

labels_t LABELS =
{
    .wait_text = "Tea brewing",
    .ready_text = "Tea ready",
    .ready_desc = "Enjoy.",
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

void cleanup(HWND hwnd, data_t& d)
{
    KillTimer(hwnd, d.timer);
    DeleteObject(d.fontTitleA);
    DeleteObject(d.fontDescA);
    DeleteObject(d.fontTitleB);
    DeleteObject(d.fontDescB);
}

std::string sys_err_fmt(const std::string& desc)
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

    return str.str();
}

void sys_err(const std::string& desc)
{
    throw std::runtime_error(sys_err_fmt(desc));
}

void errno_err(std::string desc)
{
    int e = errno;
    desc += " (" + std::to_string(e) + "): " + strerror(e);
    throw std::runtime_error(desc);
}

std::wstring console_prompt(const std::string& text)
{
    FreeConsole();

    if (AllocConsole() == 0)
        sys_err("Console window creation");

    FILE* out = fopen("CONOUT$", "w");
    FILE* in = fopen("CONIN$", "r");

    // TODO: Uniqueptr-ify cause of throwing.
    if (!out || !in)
        errno_err("Open standard streams");

    fprintf(out, "%s", text.c_str());
    fflush(out);

    std::wstring res;
    wint_t wc;
    while ( ( wc = fgetwc(in) ) != WEOF && wc != L'\n' )
    {
        res += wc;
    }

    // This did appear needed, but doesn't anymore.
    fclose(out);
    fclose(in);

    // Doesn't appear to be needed. Console closes even without it.
    // PostMessage(GetConsoleWindow(), WM_CLOSE, 0, 0);

    if (FreeConsole() == 0)
        sys_err("Close console");

    return res;
}

void from_env(std::string& out, const char* var)
{
    auto env = std::getenv(var);
    if (env)
        out = env;
}

void from_env(int& out, const char* var)
{
    auto env = std::getenv(var);
    if (env)
        out = std::stoi(env);
}

void from_env(rgb_t& out, const char* var)
{
    auto env = std::getenv(var);
    if (!env) return;

    std::string_view str = env;
    if (str.size() < 7 || str.front() != '#') return;

    str.remove_prefix(1);
    auto str_r = std::string(str.substr(0, 2));
    auto str_g = std::string(str.substr(2, 2));
    auto str_b = std::string(str.substr(4, 2));

    int r = stoi(str_r, nullptr, 16);
    int g = stoi(str_g, nullptr, 16);
    int b = stoi(str_b, nullptr, 16);

    out.r = r;
    out.g = g;
    out.b = b;
}

void load_settings()
{
    from_env(LAYOUT.width, "WINTEA_WIDTH");
    from_env(LAYOUT.height, "WINTEA_HEIGHT");
    from_env(LAYOUT.dx, "WINTEA_DX");
    from_env(LAYOUT.dy, "WINTEA_DY");
    from_env(LAYOUT.icon_size, "WINTEA_ICON_SIZE");

    from_env(LAYOUT.wait_style.font, "WINTEA_WAIT_STYLE_FONT");
    from_env(LAYOUT.wait_style.fg, "WINTEA_WAIT_STYLE_FG");
    from_env(LAYOUT.wait_style.bg, "WINTEA_WAIT_STYLE_BG");

    from_env(LAYOUT.ready_style.font, "WINTEA_READY_STYLE_FONT");
    from_env(LAYOUT.ready_style.fg, "WINTEA_READY_STYLE_FG");
    from_env(LAYOUT.ready_style.bg, "WINTEA_READY_STYLE_BG");

    from_env(LABELS.wait_text, "WINTEA_WAIT_TEXT");
    from_env(LABELS.ready_text, "WINTEA_READY_TEXT");
    from_env(LABELS.ready_desc, "WINTEA_WAIT_DESC");

    from_env(LAYOUT.title_rect.x_left,   "WINTEA_TITLE_RECT_X_LEFT");
    from_env(LAYOUT.title_rect.y_top,    "WINTEA_TITLE_RECT_Y_TOP");
    from_env(LAYOUT.title_rect.x_right,  "WINTEA_TITLE_RECT_X_RIGHT");
    from_env(LAYOUT.title_rect.y_bottom, "WINTEA_TITLE_RECT_T_BOTTOM");

    from_env(LAYOUT.desc_rect.x_left,   "WINTEA_DESC_RECT_X_LEFT");
    from_env(LAYOUT.desc_rect.y_top,    "WINTEA_DESC_RECT_Y_TOP");
    from_env(LAYOUT.desc_rect.x_right,  "WINTEA_DESC_RECT_X_RIGHT");
    from_env(LAYOUT.desc_rect.y_bottom, "WINTEA_DESC_RECT_Y_BOTTOM");
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
        MessageBox(nullptr, ex.what(), "Error", MB_OK | MB_ICONERROR);
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

    if (str.empty())
        throw std::runtime_error("input empty");

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
            { L'0', 0 }, {  L'1', 1 }, { L'2', 2 }, { L'3', 3 }, { L'4', 4 },
            { L'5', 5 }, {  L'6', 6 }, { L'7', 7 }, { L'8', 8 }, { L'9', 9 },
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

std::string remaining_message(int secs)
{
    auto out = std::stringstream{};
    auto mins = (secs / 60) % 60;
    auto hours = secs / 3600;
    secs %= 60;
    out << "Wait";
    if (hours > 0) out << " " << hours << " h";
    if (mins > 0)  out << " " << mins  << " m";
    out << " " << secs << " s.";
    return out.str();
}

int WINAPI myMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    load_settings();

    std::vector<std::wstring> arguments = get_arguments();
    if (arguments.size() < 2)
        arguments.push_back(console_prompt("Time: "));

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

    data.icon = get_icon(data, LOADING_ICON_START + 1);

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

            // No need to free the previous, since it's loaded as “shared”.
            data.icon = get_icon(data, done ? CAJIK_ICON : LOADING_ICON_START + angle);

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
                title = LABELS.ready_text;
                desc = LABELS.ready_desc;
            }
            else
            {
                int remains = data.tea_time_ms / 1000 - elapsed / 1000;
                title = LABELS.wait_text;
                desc = remaining_message(remains);
            }


            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            SetBkColor(hdc, RGB(bg.r, bg.g, bg.b));

            HBRUSH brush = CreateSolidBrush(RGB(bg.r, bg.g, bg.b));
            FillRect(hdc, &ps.rcPaint, brush);

            SelectObject(hdc, fontTitle);
            SetTextColor(hdc, RGB(fg.r, fg.g, fg.b));
            RECT rect_title;
            //x_left, y_top, x_right, y_bottom
            SetRect(&rect_title, LAYOUT.title_rect.x_left,
                                 LAYOUT.title_rect.y_top,
                                 LAYOUT.title_rect.x_right,
                                 LAYOUT.title_rect.y_bottom);
            DrawText(hdc, title.c_str(), -1, &rect_title, DT_LEFT | DT_WORD_ELLIPSIS);

            SelectObject(hdc, fontDesc);
            SetTextColor(hdc, RGB(fg.r, fg.g, fg.b));
            RECT rect_desc;
            SetRect(&rect_desc, LAYOUT.desc_rect.x_left,
                                LAYOUT.desc_rect.y_top,
                                LAYOUT.desc_rect.x_right,
                                LAYOUT.desc_rect.y_bottom);
            DrawText(hdc, desc.c_str(), -1, &rect_desc, DT_LEFT | DT_WORDBREAK);

            DrawIconEx(hdc, 0, 0, data.icon, LAYOUT.icon_size, LAYOUT.icon_size,
                       0, nullptr, DI_NORMAL);

            EndPaint(hwnd, &ps);
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
