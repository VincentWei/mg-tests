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
**  Test code for issue #87 (https://github.com/VincentWei/MiniGUI/issues/87)
**
**  This test program creates a dialog box with hosted main windows.
**
**  The following APIs are covered:
**
**      CreateMainWindow
**      CreateMainWindowIndirectParam
**      DestroyMainWindow
**      MainWindowCleanup
**      DialogBoxIndirectParam
**      EndDialogBox
**      GetFirstHosted
**      GetHosting
**      PostMessage
**      SendNotifyMessage
**      PostQuitMessage
**      MSG_IDLE
**      MSG_CREATE
**
** Copyright (C) 2022 FMSoft (http://www.fmsoft.cn).
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

static LRESULT
fourthWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        _MG_PRINTF("A main window created: %s\n", GetWindowCaption(hWnd));
         CreateWindowEx(CTRL_BUTTON,
                "1st in 4th",
                WS_CHILD | WS_VISIBLE ,  
                WS_EX_NONE,
                104,
                150, 0, 40, 40, hWnd, 0);
        break;

    case MSG_CLOSE:
        _MG_PRINTF("Destroying a main window: %s\n", GetWindowCaption(hWnd));
        DestroyMainWindow (hWnd);
        MainWindowCleanup (hWnd);
        break ;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

static LRESULT
thirdWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        _MG_PRINTF("A main window created: %s\n", GetWindowCaption(hWnd));

        CreateWindowEx(CTRL_BUTTON,
                "1st in 3rd",
                WS_CHILD | WS_VISIBLE,
                WS_EX_NONE,
                102,
                50, 100, 105, 40, hWnd, 0);
        CreateWindowEx(CTRL_BUTTON,
                "2nd in 3rd",
                WS_CHILD | WS_VISIBLE,
                WS_EX_NONE,
                110,
                0, 0, 105, 40, hWnd, 0);
        break;

    case MSG_IDLE:
        if (GetFirstHosted(hWnd) == HWND_NULL) {
            DLGTEMPLATE stDlg;

            memset(&stDlg, 0, sizeof(stDlg));
            stDlg.dwStyle = WS_VISIBLE | WS_CAPTION | WS_BORDER;
            stDlg.dwExStyle = WS_EX_NONE;
            stDlg.x = 250;
            stDlg.y = 0;
            stDlg.w = 200;
            stDlg.h = 200;
            stDlg.caption = "The Fourth Main Window";
            CreateMainWindowIndirectParam(&stDlg, hWnd, fourthWinProc, 0);
        }
        else {
            static int count;
            count++;
            if (count == 10)
                SendNotifyMessage(hWnd, MSG_CLOSE, 0, 0);
        }
        break;

    case MSG_CLOSE:
        _MG_PRINTF("Destroying a main window: %s\n", GetWindowCaption(hWnd));
        PostMessage(GetHosting(hWnd), MSG_CLOSE, 0, 0);
        EndDialog(hWnd, 0);
        break;
    }

    return DefaultDialogProc(hWnd, message, wParam, lParam);
}

static LRESULT
secondWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        _MG_PRINTF("A main window created: %s\n", GetWindowCaption(hWnd));
        CreateWindowEx(CTRL_BUTTON,
                "the 1st in 2nd",
                WS_CHILD | WS_VISIBLE,
                WS_EX_NONE,
                101,
                50, 100, 105, 40, hWnd, 0);
        CreateWindowEx(CTRL_BUTTON,
                "the 2nd in 2nd",
                WS_CHILD | WS_VISIBLE,
                WS_EX_NONE,
                120,
                100, 50, 80, 40, hWnd, 0);
        break;

    case MSG_IDLE:
        if (GetFirstHosted(hWnd) == HWND_NULL) {
            DLGTEMPLATE stDlg;

            memset(&stDlg, 0, sizeof(stDlg));
            stDlg.dwStyle = WS_VISIBLE | WS_CAPTION | WS_BORDER;
            stDlg.dwExStyle = WS_EX_NONE;
            stDlg.x = 0;
            stDlg.y = 0;
            stDlg.w = 200;
            stDlg.h = 200;
            stDlg.caption = "The Third Main Window";
            DialogBoxIndirectParam(&stDlg, hWnd, thirdWinProc, 0);
        }
        break;

    case MSG_CLOSE:
        _MG_PRINTF("Destroying a main window: %s\n", GetWindowCaption(hWnd));
        PostMessage(GetHosting(hWnd), MSG_CLOSE, 0, 0);
        DestroyMainWindow(hWnd);
        MainWindowCleanup(hWnd);
        break ;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

static LRESULT
firstWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        _MG_PRINTF("A main window created: %s\n", GetWindowCaption(hWnd));
        CreateWindowEx(CTRL_BUTTON,
                "The 1st button",
                WS_CHILD | WS_VISIBLE,
                WS_EX_TRANSPARENT,
                100,
                50, 50, 105, 40, hWnd, 0);
        break;

    case MSG_IDLE:
        if (GetFirstHosted(hWnd) == HWND_NULL) {
            DLGTEMPLATE stDlg;
            memset(&stDlg, 0, sizeof(stDlg));
            stDlg.dwStyle = WS_VISIBLE | WS_CAPTION | WS_BORDER;
            stDlg.dwExStyle = WS_EX_NONE;
            stDlg.x = 0;
            stDlg.y = 0;
            stDlg.w = 200;
            stDlg.h = 200;
            stDlg.caption = "The Second Main Window";
            CreateMainWindowIndirectParam(&stDlg, hWnd, secondWinProc, 0);
        }
        break;

    case MSG_CLOSE:
        _MG_PRINTF("Destroying a main window: %s\n", GetWindowCaption(hWnd));
        DestroyMainWindow (hWnd);
        PostQuitMessage (hWnd);
        break;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

int MiniGUIMain (int argc, const char* argv[])
{
    MSG Msg;
    MAINWINCREATE CreateInfo;

    _MG_PRINTF("Starting to test EndDialog()...\n");

    JoinLayer(NAME_DEF_LAYER , "enddialog", 0 , 0);

    CreateInfo.dwStyle = WS_VISIBLE | WS_CAPTION;
    CreateInfo.dwExStyle = WS_EX_NONE;
    CreateInfo.spCaption = "The First Main Window";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = firstWinProc;
    CreateInfo.lx = 0;
    CreateInfo.ty = 0;
    CreateInfo.rx = 480;
    CreateInfo.by = 272;
    CreateInfo.iBkColor = COLOR_lightwhite;
    CreateInfo.dwAddData = 0;
    CreateInfo.hHosting = HWND_DESKTOP;

    hMainWnd = CreateMainWindow (&CreateInfo);
    if (hMainWnd == HWND_INVALID) {
        exit(1);
        return 1;
    }

    srandom(time(NULL));
    ShowWindow(hMainWnd, SW_SHOWNORMAL);

    while (GetMessage(&Msg, hMainWnd)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    MainWindowCleanup (hMainWnd);

    _MG_PRINTF ("Test for EndDialog() passed!\n");
    exit(0);

    return 0;
}

