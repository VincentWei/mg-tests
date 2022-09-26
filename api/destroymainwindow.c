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
**  This test program creates a main windows with hosted main windows, in which
**  there are some controls. The bug may cause only the fourth main window
**  could get the `MSG_CLOSE` message which is posted by calling
**  `BroadcastMessage()`, because the buggy implementation of
**  `ThrowAwayMessages()` threw away all messages in the message queue.
**
**  The following APIs are covered:
**
**      CreateMainWindow
**      CreateMainWindowIndirectParam
**      DestroyMainWindow
**      MainWindowCleanup
**      GetFirstHosted
**      BroadcastMessage
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

#include <stdio.h>
#include <string.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

static LRESULT
fourthWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        _MG_PRINTF("A main window created: %s\n", GetWindowCaption(hWnd));
        CreateWindowEx(CTRL_BUTTON,
                "1st in 4th",
                WS_CHILD | WS_VISIBLE,
                WS_EX_NONE,
                104,
                50, 100, 105, 40, hWnd, 0);
        CreateWindowEx(CTRL_BUTTON,
                "2nd in 4th",
                WS_CHILD | WS_VISIBLE,
                WS_EX_NONE,
                105,
                50, 50, 40, 40, hWnd, 0);
        break;

    case MSG_IDLE:
        BroadcastMessage(MSG_CLOSE, 0xFF, 0);
        break;

    case MSG_CLOSE:
        _MG_PRINTF("Destroying a main window: %s\n", GetWindowCaption(hWnd));
        DestroyMainWindow(hWnd);
        MainWindowCleanup(hWnd);
        break ;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

static LRESULT thirdWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
            stDlg.caption = "The Fourth Main Window";
            CreateMainWindowIndirectParam(&stDlg, hWnd, fourthWinProc, 0);
        }
        break;

    case MSG_CLOSE:
        _MG_PRINTF("Destroying a main window: %s\n", GetWindowCaption(hWnd));
        DestroyMainWindow(hWnd);
        MainWindowCleanup(hWnd);
        break ;

    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

static LRESULT
secondWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        _MG_PRINTF("A main window created: %s\n", GetWindowCaption(hWnd));
        CreateWindowEx(CTRL_BUTTON,
                "1st in 2nd",
                WS_CHILD | WS_VISIBLE,
                WS_EX_NONE,
                101,
                50, 100, 105, 40, hWnd, 0);
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
            CreateMainWindowIndirectParam(&stDlg, hWnd, thirdWinProc, 0);
        }
        break;

    case MSG_CLOSE:
        _MG_PRINTF("Destroying a main window: %s\n", GetWindowCaption(hWnd));
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
                "1st in 1st",
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
        if (GetFirstHosted(hWnd) == HWND_NULL) {
            _MG_PRINTF("All hosted main windows were destroyed, it's time to quit\n");
            DestroyMainWindow(hWnd);
            PostQuitMessage(hWnd);
        }
        else {
            _MG_PRINTF("There is still hosted main windows\n");
        }
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

    HWND hMainWnd = CreateMainWindow(&CreateInfo);
    if (hMainWnd == HWND_INVALID) {
        exit(1);
        return 1;
    }

    ShowWindow(hMainWnd, SW_SHOWNORMAL);

    while (GetMessage(&Msg, hMainWnd)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    MainWindowCleanup(hMainWnd);

    exit(0);
    return 0;
}

