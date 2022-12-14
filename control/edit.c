#include <stdio.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

static LRESULT HelloWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PLOGFONT caption_font;
    HWND ctrl;
    switch (message) {
        case MSG_CREATE:
            CreateWindowEx(CTRL_SLEDIT,
                    "abcd",
                    WS_VISIBLE | WS_BORDER,
                    WS_EX_TRANSPARENT,
                    401,
                    48, 15, 176, 25,
                    hWnd, 0);


            ctrl = CreateWindowEx(CTRL_BIDISLEDIT,
                    "abcd",
                    WS_VISIBLE | WS_BORDER,
                    WS_EX_TRANSPARENT,
                    402,
                    48, 50, 176, 25,
                    hWnd, 0);
            caption_font = CreateLogFont(NULL, "monospace", "ISO8859-6",
                    FONT_WEIGHT_BOOK , FONT_SLANT_ROMAN, FONT_FLIP_NIL,
                    FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE,
                    18, 0);
            SetWindowFont(ctrl, caption_font);
            return 0;

        case MSG_DESTROY:
            DestroyAllControls (hWnd);
            return 0;

        case MSG_CLOSE:
            DestroyMainWindow (hWnd);
            PostQuitMessage (hWnd);
            return 0;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

int MiniGUIMain (int argc, const char* argv[])
{
    MSG Msg;
    HWND hMainWnd;
    MAINWINCREATE CreateInfo;

    JoinLayer(NAME_DEF_LAYER , "mycontrol" , 0 , 0);

    CreateInfo.dwStyle = WS_VISIBLE | WS_BORDER | WS_CAPTION;
    CreateInfo.dwExStyle = WS_EX_NONE;
    CreateInfo.spCaption = "Hello, world";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = HelloWinProc;
    CreateInfo.lx = 0;
    CreateInfo.ty = 0;
    CreateInfo.rx = 480;
    CreateInfo.by = 480;
    CreateInfo.iBkColor = COLOR_lightgray;
    CreateInfo.dwAddData = 0;
    CreateInfo.hHosting = HWND_DESKTOP;

    hMainWnd = CreateMainWindow (&CreateInfo);

    if (hMainWnd == HWND_INVALID)
        return -1;

    ShowWindow(hMainWnd, SW_SHOWNORMAL);

    while (GetMessage(&Msg, hMainWnd)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    MainWindowThreadCleanup (hMainWnd);
    return 0;
}

