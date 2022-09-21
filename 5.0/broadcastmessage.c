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
**  Test code for issue #89 (https://github.com/VincentWei/MiniGUI/issues/89)
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

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include <stdio.h>
#include <string.h>

HWND hMainWnd;
static void pwdlogin_ime_next_deal(HWND hWnd, UINT dwId, WPARAM dwNc, LPARAM add_data)
{
    HWND hWndParent = GetParent(hWnd);
    switch (dwId)
    {
        case 104:
            do
            { printf("close hwnd id is : %p\n",hWndParent);
                PostMessage(hWndParent, MSG_CLOSE, 0, 0);
            }while((hWndParent = GetHosting(hWndParent))!= hMainWnd);
            break;

        case 105:
            printf("105\n\n");
            BroadcastMessage(MSG_CLOSE, 0xFF, 0);
            break;

    }
    return ;
}

static LRESULT pwdlogin_ime_next_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
        case MSG_CREATE:
            {
                printf("44444444 hwnd id is : %p\n",hWnd);
                HWND pwdlogin_ime_next_hWnd = CreateWindowEx(CTRL_BUTTON,
                        "44444444444",
                        WS_CHILD | WS_VISIBLE ,  
                        WS_EX_NONE,
                        104,
                        50, 100, 105, 40, hWnd, 0);
                SetNotificationCallback(pwdlogin_ime_next_hWnd, (NOTIFPROC)pwdlogin_ime_next_deal);
                HWND pwdlogin_ime_next_111hWnd = CreateWindowEx(CTRL_BUTTON,
                        "5555555",
                        WS_CHILD | WS_VISIBLE ,  
                        WS_EX_NONE,
                        105,
                        50, 50, 40, 40, hWnd, 0);
                SetNotificationCallback(pwdlogin_ime_next_111hWnd, (NOTIFPROC)pwdlogin_ime_next_deal);
                break;
            }

        case MSG_CLOSE:
            DestroyMainWindow (hWnd);
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
            stDlg.x = 0;
            stDlg.y = 0;
            stDlg.w = 200;
            stDlg.h = 200;
            stDlg.caption = "Win3";
            CreateMainWindowIndirectParam(&stDlg, hWndParent, pwdlogin_ime_next_proc, 0);
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
                SetNotificationCallback(pwdlogin_ime_hWnd, (NOTIFPROC)pwdlogin_ime_deal);
                break;
            }
        case MSG_CLOSE:
            DestroyMainWindow (hWnd);
            break ;

    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
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
            stDlg.caption = "Win2";
            CreateMainWindowIndirectParam(&stDlg, hWndParent, pwdlogin_ime_proc, 0);
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
                SetNotificationCallback(pwdlogin_window_hWnd, (NOTIFPROC)pwdlogin_window_deal);
                break;
            }
        case MSG_CLOSE:
            DestroyMainWindow (hWnd);
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

                DLGTEMPLATE stDlg;
                memset(&stDlg, 0, sizeof(stDlg));
                stDlg.dwStyle = WS_VISIBLE | WS_CAPTION | WS_BORDER;
                stDlg.dwExStyle = WS_EX_NONE;
                stDlg.x = 0;
                stDlg.y = 0;
                stDlg.w = 200;
                stDlg.h = 200;
                stDlg.caption = "Win1";
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

