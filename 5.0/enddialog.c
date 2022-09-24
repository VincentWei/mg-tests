#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include <stdio.h>
#include <string.h>
HWND hMainWnd;

static LRESULT pwdlogin_ime_next_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
        case MSG_CREATE:
            {
                printf("44444444 hwnd id is : %p\n",hWnd);
                 CreateWindowEx(CTRL_BUTTON,
                        "44444444444",
                        WS_CHILD | WS_VISIBLE ,  
                        WS_EX_NONE,
                        104,
                        150, 0, 40, 40, hWnd, 0);
                break;
            }

        case MSG_CLOSE:
            DestroyMainWindow (hWnd);
            MainWindowThreadCleanup (hWnd);
            break ;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}
static void pwdlogin_ime_deal(HWND hWnd, UINT dwId, WPARAM dwNc, LPARAM add_data)
{
    HWND hWndParent = GetParent(hWnd);
    DLGTEMPLATE stDlg;
    switch (dwId)
    {
        case 102:

            memset(&stDlg, 0, sizeof(stDlg));
            stDlg.dwStyle = WS_VISIBLE | WS_CAPTION | WS_BORDER;
            stDlg.dwExStyle = WS_EX_NONE;
            stDlg.x = 250;
            stDlg.y = 0;
            stDlg.w = 200;
            stDlg.h = 200;
            stDlg.caption = "4444";
            CreateMainWindowIndirectParam(&stDlg, hWndParent, pwdlogin_ime_next_proc, 0);
            break;

        case 110:
            SendNotifyMessage(hWndParent,MSG_CLOSE,0,0);
            break;


    }
    return ;
}
static LRESULT pwdlogin_ime_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
        case MSG_CREATE:
            {
                printf("33333 hwnd id is : %p\n",hWnd);
                HWND pwdlogin_ime_hWnd = CreateWindowEx(CTRL_BUTTON,
                        "333333333",
                        WS_CHILD | WS_VISIBLE ,  
                        WS_EX_NONE,
                        102,
                        50, 100, 105, 40, hWnd, 0);
                HWND pwdlogin_ime_hWnd2 = CreateWindowEx(CTRL_BUTTON,
                        "=======",
                        WS_CHILD | WS_VISIBLE ,  
                        WS_EX_NONE,
                        110,
                        0, 0, 105, 40, hWnd, 0);
                SetNotificationCallback(pwdlogin_ime_hWnd2, (NOTIFPROC)pwdlogin_ime_deal);
                SetNotificationCallback(pwdlogin_ime_hWnd, (NOTIFPROC)pwdlogin_ime_deal);
                break;
            }
        case MSG_CLOSE:
            printf("MSG_CLOSE\n");
            EndDialog(hWnd,0);
            break ;

    }

    return DefaultDialogProc(hWnd, message, wParam, lParam);
}
static void pwdlogin_window_deal(HWND hWnd, UINT dwId, WPARAM dwNc, LPARAM add_data)
{
    HWND hWndParent = GetParent(hWnd);
    DLGTEMPLATE stDlg;
    switch (dwId)
    {
        case 101:

            memset(&stDlg, 0, sizeof(stDlg));
            stDlg.dwStyle = WS_VISIBLE | WS_CAPTION | WS_BORDER;
            stDlg.dwExStyle = WS_EX_NONE;
            stDlg.x = 0;
            stDlg.y = 0;
            stDlg.w = 200;
            stDlg.h = 200;
            stDlg.caption = "333";
            DialogBoxIndirectParam(&stDlg, hWndParent, pwdlogin_ime_proc, 0);
            break;
        case 120:

            SendNotifyMessage(hWndParent,MSG_CLOSE,0,0);
            break;


    }
    return ;
}
static LRESULT pwdlogin_window_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
        case MSG_CREATE:
            {
                printf("2222 hwnd id is : %p\n",hWnd);
                HWND pwdlogin_window_hWnd = CreateWindowEx(CTRL_BUTTON,
                        "222222",
                        WS_CHILD | WS_VISIBLE ,  
                        WS_EX_NONE,
                        101,
                        50, 100, 105, 40, hWnd, 0);
                HWND pwdlogin_window_hWnd_1= CreateWindowEx(CTRL_BUTTON,
                        "close",
                        WS_CHILD | WS_VISIBLE ,  
                        WS_EX_NONE,
                        120,
                        100, 50, 80, 40, hWnd, 0);
                SetNotificationCallback(pwdlogin_window_hWnd, (NOTIFPROC)pwdlogin_window_deal);
                SetNotificationCallback(pwdlogin_window_hWnd_1, (NOTIFPROC)pwdlogin_window_deal);
                break;
            }
        case MSG_CLOSE:
            printf("2222 recived close msg\n");
            DestroyMainWindow (hWnd);
            MainWindowThreadCleanup (hWnd);
            break ;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}
static LRESULT HelloWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message) {
        case MSG_CREATE:
            printf("111111 hwnd id is : %p\n",hWnd);
            hWnd = CreateWindowEx(CTRL_BUTTON,
                    "11111111111",
                    WS_CHILD | WS_VISIBLE ,  
                    WS_EX_TRANSPARENT,
                    100,
                    50, 50, 105, 40, hWnd, 0);
            break;
        case MSG_COMMAND:
            if(100 == wParam)
            {	

                GetParent(hWnd);
                DLGTEMPLATE stDlg;
                memset(&stDlg, 0, sizeof(stDlg));
                stDlg.dwStyle = WS_VISIBLE | WS_CAPTION | WS_BORDER;
                stDlg.dwExStyle = WS_EX_NONE;
                stDlg.x = 0;
                stDlg.y = 0;
                stDlg.w = 200;
                stDlg.h = 200;
                stDlg.caption = "22";
                CreateMainWindowIndirectParam(&stDlg, hWnd, pwdlogin_window_proc, 0);
            }
            break;
        case MSG_CLOSE:
            // DestroyMainWindow (hWnd);
            //PostQuitMessage (hWnd);
            break;
    }
    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

int MiniGUIMain (int argc, const char* argv[])
{
    MSG Msg;

    MAINWINCREATE CreateInfo;


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
    printf("    HWND hMainWnd id is : %p \n",hMainWnd);
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
