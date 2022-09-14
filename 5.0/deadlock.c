#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include <stdio.h>
#include <string.h>

static LRESULT pwdlogin_ime_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    printf("message in : %d \n", message);
    switch (message) {
        case MSG_CREATE:
            {
                hWnd = CreateWindowEx(CTRL_BUTTON,
                        "333333333",
                        WS_CHILD | WS_VISIBLE ,
                        WS_EX_NONE,
                        103,
                        50, 100, 105, 40, hWnd, 0);
                break;
            }

        default:
            break;

    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

static LRESULT pwdlogin_window_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    printf("message in : %d \n", message);
    switch (message) {
        case MSG_CREATE:
            {
                hWnd = CreateWindowEx(CTRL_BUTTON,
                        "222222",
                        WS_CHILD | WS_VISIBLE ,
                        WS_EX_NONE,
                        101,
                        50, 100, 105, 40, hWnd, 0);
                break;
            }
        case MSG_COMMAND:
            {
                if(101 == wParam)
                {
                    MoveWindow(hWnd,0,-100,200,200,TRUE);
                    DLGTEMPLATE stDlg;
                    memset(&stDlg, 0, sizeof(stDlg));
                    stDlg.dwStyle = WS_VISIBLE | WS_CAPTION | WS_BORDER;
                    stDlg.dwExStyle = WS_EX_NONE;
                    stDlg.x = 0;
                    stDlg.y = 0;
                    stDlg.w = 200;
                    stDlg.h = 200;
                    stDlg.caption = "1111";
                    CreateMainWindowIndirectParam(&stDlg, hWnd, pwdlogin_ime_proc, 0);

                }
                break;
            }
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

static LRESULT HelloWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case MSG_CREATE:
            hWnd = CreateWindowEx(CTRL_BUTTON,
                    "11111111111",
                    WS_CHILD | WS_VISIBLE ,
                    WS_EX_TRANSPARENT,
                    100,
                    50, 50, 105, 40, hWnd, 0);
            break;

        case MSG_COMMAND:

            if(100 == wParam) {

                DLGTEMPLATE stDlg;
                memset(&stDlg, 0, sizeof(stDlg));
                stDlg.dwStyle = WS_VISIBLE | WS_CAPTION | WS_BORDER;
                stDlg.dwExStyle = WS_EX_NONE;
                stDlg.x = 0;
                stDlg.y = 0;
                stDlg.w = 200;
                stDlg.h = 200;
                stDlg.caption = "";
                CreateMainWindowIndirectParam(&stDlg, hWnd, pwdlogin_window_proc, 0);
            }

            break;

        case MSG_CLOSE:
            DestroyMainWindow (hWnd);
            PostQuitMessage (hWnd);
            break;

    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

int MiniGUIMain (int argc, const char* argv[])
{
    MSG Msg;
    HWND hMainWnd;
    MAINWINCREATE CreateInfo;
#ifdef _MGRM_PROCESSES
    JoinLayer(NAME_DEF_LAYER , "helloworld" , 0 , 0);
#endif

    CreateInfo.dwStyle = WS_VISIBLE | WS_CAPTION;
    CreateInfo.dwExStyle = WS_EX_NONE;
    CreateInfo.spCaption = "Hello";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = HelloWinProc;
    CreateInfo.lx = 0;
    CreateInfo.ty = 0;
    CreateInfo.rx = 480;
    CreateInfo.by = 272;
    CreateInfo.iBkColor = COLOR_lightwhite;
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

#ifdef _MGRM_THREADS
#include <minigui/dti.c>
#endif

