///////////////////////////////////////////////////////////////////////////////
//
//                          IMPORTANT NOTICE
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
**  This test program is for Caret.
**
**  The following APIs are covered:
**
**      CreateMainWindow
**      GetTickCount
**      CreateCaret
**      ShowCaret
**      HideCaret
**      DestroyCaret
**      DestroyMainWindow
**      MainWindowCleanup
**      PostQuitMessage
**      PostMessage
**
** Copyright (C) 2025 FMSoft (http://www.fmsoft.cn).
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#define IDC_MYEDIT    100

/* a simple edit window */
static LRESULT MyeditWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int w = 0;
    SIZE Size;
    static char *pBuffer = NULL;
    static int pos = 0;
    HDC hdc;

    switch (message) {
        case MSG_CREATE:
            SetWindowFont(hWnd, GetSystemFont(SYSLOGFONT_DEFAULT));
            if (!CreateCaret (hWnd, NULL, 1, GetSysCharHeight())) {
                return -1;
            }
            pBuffer = (char *) calloc(1, 11);
            *pBuffer = 0;
            break;

        case MSG_SETFOCUS:
            hdc = GetClientDC(hWnd);
            w = GetTextExtent (hdc, pBuffer, pos, &Size);
            SetCaretPos(hWnd, w, 0);
            //SetCaretPos(hWnd, pos*GetSysCharWidth(), 0);
            ShowCaret(hWnd);
            ReleaseDC(hdc);
            break;

        case MSG_KILLFOCUS:
            HideCaret(hWnd);
            break;

        case MSG_CARETBLINK: {
            _MG_PRINTF("Got MSG_CARETBLINK message\n");
            static int count;
            count++;
            if (count == 5)
                PostMessage(GetParent(hWnd), MSG_CLOSE, 0, 0);
            break;
        }

        case MSG_PAINT:
            hdc = BeginPaint(hWnd);
            TextOut(hdc, 0, 0, pBuffer);
            EndPaint(hWnd, hdc);
            return 0;

        case MSG_DESTROY:
            DestroyCaret (hWnd);
            if (pBuffer)
                free(pBuffer);
            return 0;
    }

    return DefaultControlProc(hWnd, message, wParam, lParam);
}

BOOL RegisterMyedit(void)
{
    WNDCLASS WndClass;

    WndClass.spClassName = "myedit";
    WndClass.dwStyle     = 0;
    WndClass.dwExStyle   = 0;
    WndClass.hCursor     = GetSystemCursor(IDC_IBEAM);
#ifdef _MGSCHEMA_COMPOSITING
    WndClass.dwBkColor   = RGBA_lightwhite;
#else
    WndClass.iBkColor    = PIXEL_lightwhite;
#endif
    WndClass.WinProc     = MyeditWindowProc;

    return RegisterWindowClass (&WndClass);
}

/* main windoww proc */
static LRESULT CaretdemoWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hMyedit;

    switch (message) {
    case MSG_CREATE:
        RegisterMyedit();
        hMyedit = CreateWindow("myedit", "", WS_VISIBLE | WS_CHILD, IDC_MYEDIT, 
                30, 50, 100, 20, hWnd, 0);
        SetFocus(hMyedit);
        break;

    case MSG_CLOSE:
        DestroyAllControls (hWnd);
        DestroyMainWindow (hWnd);
        PostQuitMessage (hWnd);
        return 0;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

int MiniGUIMain (int args, const char* argv[])
{
    MSG Msg;
    HWND hMainWnd;
    MAINWINCREATE CreateInfo;

    _MG_PRINTF("Starting test %s...\n", argv[0]);

    JoinLayer(NAME_DEF_LAYER , "caret" , 0 , 0);

    CreateInfo.dwStyle = WS_VISIBLE | WS_BORDER | WS_CAPTION;
    CreateInfo.dwExStyle = WS_EX_NONE;
    CreateInfo.spCaption = "Caret demo";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = CaretdemoWinProc;
    CreateInfo.lx = 0;
    CreateInfo.ty = 0;
    CreateInfo.rx = 320;
    CreateInfo.by = 240;
    CreateInfo.iBkColor = COLOR_lightgray;
    CreateInfo.dwAddData = 0;
    CreateInfo.hHosting = HWND_DESKTOP;

    hMainWnd = CreateMainWindow (&CreateInfo);
    if (hMainWnd == HWND_INVALID) {
        exit(1);
        return -1;
    }

    ShowWindow(hMainWnd, SW_SHOWNORMAL);

    while (GetMessage(&Msg, hMainWnd)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    MainWindowThreadCleanup (hMainWnd);

    DWORD ticks = GetTickCount();
    if (ticks < 300UL) {
        _MG_PRINTF ("Test for caret passed; tick count: %lu!\n", ticks);
        exit(0);
    }
    else {
        _MG_PRINTF ("Test for caret failed: tick count: %lu!\n", ticks);
    }

    return 0;
}

