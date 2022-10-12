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
**  This test program set region of a main window by calling SetWindowRegion.
**
**  The following APIs are covered:
**
**      CreateMainWindow
**      DestroyMainWindow
**      MainWindowCleanup
**      PostQuitMessage
**      PostMessage
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

static LRESULT HelloWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case MSG_IDLE: {
            static int count;

            count++;
            if (count == 10)
                PostMessage(hWnd, MSG_CLOSE, 0, 0);
            break;
        }

        case MSG_PAINT: {
            HDC hdc = BeginPaint(hWnd);
            SetMemDCAlpha(hdc, 0, 0);
            SetBrushColor(hdc, 0);
            FillBox(hdc, 0, 0, 100, 100);
            EndPaint(hWnd, hdc);

            gal_pixel px = GetPixel(HDC_SCREEN, 50, 50);
            _MG_PRINTF("Pixel: 0x%x\n", px);
            assert(px == 0);
            return 0;
        }

        case MSG_CLOSE:
            DestroyMainWindow(hWnd);
            PostQuitMessage(hWnd);
            return 0;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

int MiniGUIMain (int argc, const char* argv[])
{
    MSG Msg;
    HWND hMainWnd;
    MAINWINCREATE CreateInfo;

    _MG_PRINTF("Starting test WS_EX_AUTOSECONDARYDC...\n");

    JoinLayer(NAME_DEF_LAYER , "autosecondarydc" , 0 , 0);

    CreateInfo.dwStyle = WS_VISIBLE;
    CreateInfo.dwExStyle = WS_EX_AUTOSECONDARYDC;
    CreateInfo.spCaption = "WS_EX_AUTOSECONDARYDC";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = HelloWinProc;
    CreateInfo.lx = 0;
    CreateInfo.ty = 0;
    CreateInfo.rx = g_rcScr.right;
    CreateInfo.by = g_rcScr.bottom;
    CreateInfo.iBkColor = COLOR_lightwhite;
    CreateInfo.dwAddData = 0;
    CreateInfo.hHosting = HWND_DESKTOP;

    hMainWnd = CreateMainWindow (&CreateInfo);

    if (hMainWnd == HWND_INVALID) {
        exit(1);
        return -1;
    }

    ShowWindow (hMainWnd, SW_SHOWNORMAL);
    while (GetMessage(&Msg, hMainWnd)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    MainWindowCleanup (hMainWnd);

    _MG_PRINTF ("Test for WS_EX_AUTOSECONDARYDC passed!\n");
    exit(0);

    return 0;
}

