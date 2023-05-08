///////////////////////////////////////////////////////////////////////////////
//
//                        IMPORTANT LEGAL NOTICE
//
// The following open source license statement does not apply to any
// entity in the Exception List published by FMSoft.
//
// For more information, please visit:
//
// https://www.fmsoft.cn/exception-list
//
//////////////////////////////////////////////////////////////////////////////
/*
**  This test program is for Issue #116.
**
**  The following APIs are covered:
**
**      CreateMainWindow
**      SetTimer
**      IsTimerInstalled
**      KillTimer
**      DestroyMainWindow
**      MainWindowCleanup
**      PostQuitMessage
**      PostMessage
**
** Copyright (C) 2023 FMSoft (http://www.fmsoft.cn).
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdio.h>
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include <time.h>
#include <sys/time.h>

#define MSG_TIMER_START (0x900)

static void mytime(const char *prefix)
{
    struct timeval tTimeVal;
    gettimeofday(&tTimeVal, NULL);
    struct tm *tTM = localtime(&tTimeVal.tv_sec);
    printf("%s%04d-%02d-%02d %02d:%02d:%02d.%03ld.%03ld\n", prefix,
            tTM->tm_year + 1900, tTM->tm_mon + 1, tTM->tm_mday,
            tTM->tm_hour, tTM->tm_min, tTM->tm_sec,
            tTimeVal.tv_usec / 1000, tTimeVal.tv_usec % 1000);
}

static void input_back_button_deal(HWND hWnd, int dwMessage, WPARAM dwParam, LPARAM lParam)
{
    if (BN_CLICKED == dwParam) {
        PostMessage(GetParent(hWnd), MSG_CLOSE,0,0);
    }
}

static LRESULT gui_input_window_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND button;
    switch (message)
    {
        case MSG_CREATE:

            button = CreateWindowEx(CTRL_BUTTON,
                    "2",
                    WS_VISIBLE,
                    0,
                    100,
                    0, 
                    0,
                    100,
                    50,
                    hWnd,0);
            SetNotificationCallback(button, (NOTIFPROC)input_back_button_deal);
            break;

        case MSG_IDLE:
        {
            static int first = 0;
            if (first == 0) {
                first++;
                NotifyParent(button, 0, BN_CLICKED);
            }
            break;
        }

        case MSG_CLOSE:
            mytime("MSG_CLOSE:");
            SendMessage(GetHosting(hWnd),MSG_TIMER_START,0,0);
            EndDialog(hWnd,0);
            break;

        default:
            break;
    }

    return DefaultDialogProc (hWnd, message, wParam, lParam);
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
    if (BN_CLICKED == dwParam) {
        input_wnd_start(GetParent(hWnd));
    }
}

static int timer_interval = 3;
static time_t timer_installed, timer_expired;

static LRESULT HelloWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND button;
    HWND hwnd1;
    switch (message) {
        case MSG_CREATE:
            button = CreateWindowEx(CTRL_BUTTON,
                    "1",
                    WS_VISIBLE,
                    0,
                    0,
                    0, 0,
                    100, 50,
                    hWnd,0);
            SetNotificationCallback(button, (NOTIFPROC)input_button_deal);
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

        case MSG_IDLE:
        {
            static int first = 0;
            if (first == 0) {
                first++;
                mytime("MSG_IDLE:");
                NotifyParent(button, 0, BN_CLICKED);
            }
            break;
        }

        case MSG_TIMER_START: {
            mytime("MSG_TIMER_START:");

            time_t start = time(NULL), now;
            do {
                for (int i = 0; i <= 2000000000; i++) {
                    // consume some time.
                }

                now = time(NULL);
                if (now >= (start + 3)) {
                    break;
                }

            } while (1);

            if (IsTimerInstalled(hWnd, 11)) {
                KillTimer(hWnd, 11);
            }
            mytime("INSTALL THE TIMER:");
            timer_installed = time(NULL);
            SetTimer(hWnd, 11, timer_interval * 100);
            break;
        }

        case MSG_TIMER:
            mytime("MSG_TIMER:");
            timer_expired = time(NULL);
            PostMessage(hWnd, MSG_CLOSE, 0, 0);
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

    _MG_PRINTF("Starting test %s...\n", argv[0]);

    if (argc > 1) {
        timer_interval = atoi(argv[1]);
        if (timer_interval < 2)
            timer_interval = 2;
    }

    JoinLayer(NAME_DEF_LAYER , "timer" , 0 , 0);

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
    if ((timer_expired - timer_installed) >= timer_interval) {
        _MG_PRINTF("Success\n");
        exit(EXIT_SUCCESS);
    }
    else {
        _WRN_PRINTF("Failed\n");
        exit(EXIT_FAILURE);
        return 1;
    }

    return 0;
}

