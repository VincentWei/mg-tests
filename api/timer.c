#include <stdio.h>
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include <time.h>
#include <sys/time.h>

#define MSG_TIMER_START (0x900)

static void mytime(void)
{
    struct timeval tTimeVal;
    gettimeofday(&tTimeVal, NULL);
    struct tm *tTM = localtime(&tTimeVal.tv_sec);
    printf("%04d-%02d-%02d %02d:%02d:%02d.%03d.%03d\n\n",
            tTM->tm_year + 1900, tTM->tm_mon + 1, tTM->tm_mday,
            tTM->tm_hour, tTM->tm_min, tTM->tm_sec,
            tTimeVal.tv_usec / 1000, tTimeVal.tv_usec % 1000);
}

void input_back_button_deal(HWND hWnd, int dwMessage, WPARAM dwParam, LPARAM lParam)
{
    if (BN_CLICKED != dwParam)
    {
        return;
    }

    SendMessage(GetParent(hWnd),MSG_CLOSE,0,0);
    return;
}

static LRESULT gui_input_window_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hWnd2;
    switch (message)
    {
        case MSG_CREATE:

            hWnd2 = CreateWindowEx(CTRL_BUTTON,
                    "2",
                    WS_VISIBLE,
                    0,
                    100,
                    0, 
                    0,
                    100,
                    50,
                    hWnd,0);
            SetNotificationCallback(hWnd2, (NOTIFPROC)input_back_button_deal);
            break;
        case MSG_CLOSE:
            SendMessage(GetHosting(hWnd),MSG_TIMER_START,0,0);
            EndDialog(hWnd,0);
            break;
        default:
            return DefaultDialogProc (hWnd, message, wParam, lParam);
            break;
    }

    return 0;
}

void input_wnd_start(HWND hwnd)
{
    DLGTEMPLATE stDlg = {0};
    stDlg.dwStyle = WS_VISIBLE;
    stDlg.dwExStyle = WS_EX_NOCLOSEBOX | WS_EX_AUTOSECONDARYDC | WS_EX_TROUNDCNS | WS_EX_BROUNDCNS;
    stDlg.x = 100;
    stDlg.y = 100;
    stDlg.w = 200;
    stDlg.h = 200;
    stDlg.caption = "22222";
    DialogBoxIndirectParam(&stDlg, hwnd, gui_input_window_proc, 0);
}

void input_button_deal(HWND hWnd, int dwMessage, WPARAM dwParam, LPARAM lParam)
{
    if (BN_CLICKED != dwParam)
    {
        return;
    }
    input_wnd_start(GetParent(hWnd));
    return ;
}

static LRESULT HelloWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hwnd1;
    switch (message) {
        case MSG_CREATE:
            hwnd1 = CreateWindowEx(CTRL_BUTTON,
                    "1",
                    WS_VISIBLE,
                    0,
                    0,
                    0,
                    0,
                    100,
                    50,
                    hWnd,0);
            SetNotificationCallback(hwnd1, (NOTIFPROC)input_button_deal);
            hwnd1 = CreateWindowEx(CTRL_STATIC,
                    "2222",
                    WS_VISIBLE,
                    10,
                    0,
                    0,
                    60,
                    100,
                    50,
                    hWnd,0);
            ShowWindow(hwnd1,SW_HIDE);
            break;
        case MSG_TIMER_START:

            mytime();
            printf("rec msg!!!\n\n\n");
            for (int i = 0; i <= 2000000000;i++)
            {}
            //sleep(500);
            printf("out if deal!!!\n\n\n");
            mytime();
            printf("set time!!!\n\n\n");
            if (IsTimerInstalled(hWnd, 11))
            {
                KillTimer(hWnd, 11);
            }
            SetTimer(hWnd, 11, 200);
            break;

        case MSG_TIMER:
            mytime();
            printf("timer: %d!!!\n\n\n", (int)wParam);
            break;

        case MSG_DESTROY:
            DestroyAllControls (hWnd);
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
    JoinLayer(NAME_DEF_LAYER , "mycontrol" , 0 , 0);
#endif

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


