#include <windows.h>
#include <string>
#include <vector>
#include <stdexcept>        // runtime_error
#include <iostream>
#include <sstream>
#include <chrono>
#include <algorithm>        // clamp

namespace ch = std::chrono;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI myMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine,
                  int nCmdShow);

struct data_t
{
    HFONT     hFont     = nullptr;
    HICON     icon      = nullptr;
    UINT_PTR  timer     = 10;
    HINSTANCE hInstance = nullptr;

    std::chrono::steady_clock::time_point start = {};

    int tea_time_ms = 0;
};

data_t data;

std::vector<std::wstring> arguments;

void cleanup(HWND hwnd, data_t& d)
{
    KillTimer(hwnd, d.timer);
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

int WINAPI myMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    arguments = get_arguments();

    data.hInstance = hInstance;
    data.tea_time_ms = 1000 * std::stoi(arguments.at(1));
    data.start = std::chrono::steady_clock::now();

    const char app_class[] = "Tea Notification Class";

    WNDCLASS wc = {};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = app_class;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon         = get_icon(data, 1337);

    RegisterClass(&wc);

    int win_width = 300;
    int win_height = 100;

    int sys_w = GetSystemMetrics(SM_CXSCREEN);
    int sys_h = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowEx(0, // <-- ext style
                              app_class, "Tea Notification",
                              WS_OVERLAPPEDWINDOW, // <-- style
                              0, 0,
                              win_width, win_height,
                              nullptr, nullptr, hInstance, nullptr);

    if (hwnd == nullptr)
        return sys_err("CreateWindow"), 1;

    data.hFont = CreateFont(25,                          // <-- height
                            0,                           // <-- width
                            0,
                            0,                           // <-- angle
                            FW_NORMAL,                   // <-- weight
                            false,                       // <-- italic
                            false,                       // <-- underline
                            false,                       // <-- strikeout
                            DEFAULT_CHARSET,             // <-- charset
                            OUT_OUTLINE_PRECIS,          // <-- precision
                            CLIP_DEFAULT_PRECIS,         // <-- clip precision
                            CLEARTYPE_QUALITY,           // <-- quality
                            DEFAULT_PITCH | FF_DONTCARE, // <-- pitch, family
                            "Arial");

    data.icon = get_icon(data, 10001);

    if (SetTimer(hwnd, data.timer, 100, (TIMERPROC) nullptr) == 0)
        return sys_err("LoadImage"), 1;

    auto prev = GetForegroundWindow();

    SetWindowLong(hwnd, GWL_STYLE, 0);
    SetWindowLong(hwnd, GWL_EXSTYLE, 0);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    SetWindowPos(hwnd, HWND_TOPMOST, sys_w - win_width, sys_h - win_height,
                 win_width, win_height, SWP_SHOWWINDOW);

    // Since the above call steals focus, let's restore it back to
    // the rightful holder. This results in one small flicker of the
    // previous window's border. It would be preferable if it were
    // possible not to steal focus with the above call, but sadly
    // this is the best I could achieve.
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

            int angle = elapsed * 360 / data.tea_time_ms;

            if (angle >= 360)
                data.icon = get_icon(data, 1337);
            else
                data.icon = get_icon(data, 10000 + std::clamp(angle, 1, 360));

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            SetBkColor(hdc, RGB(123, 0, 123));

            HBRUSH brush = CreateSolidBrush(RGB(123, 0, 123));
            FillRect(hdc, &ps.rcPaint, brush);

            SelectObject(hdc, data.hFont);

            std::string text = "Hello, Window! Bla bla bla bla these nuts being dragged across your face.";
            SetTextColor(hdc, RGB(255, 255, 255));
            TextOutA(hdc, 100, 10, text.c_str(), text.length());

            DrawIconEx(hdc, 0, 0, data.icon, 64, 64, 0, nullptr, DI_NORMAL);

            EndPaint(hwnd, &ps);
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
