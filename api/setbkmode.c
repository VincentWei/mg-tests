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
**      SetBkMode
**      LoadBitmap
**      FillBoxWithBitmap
**      TextOut
**      DestroyMainWindow
**      MainWindowCleanup
**      PostQuitMessage
**      PostMessage
**
** Copyright (C) 2019 FMSoft (http://www.fmsoft.cn).
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

BOOL passed = TRUE;
static BITMAP bmp;

static LRESULT HelloWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        LoadBitmap(HDC_SCREEN, &bmp, "res/gnu.png");
        bmp.bmType = BMP_TYPE_COLORKEY;
        bmp.bmColorKey = 0;
        _MG_PRINTF("Type: %x, colorKey: %x\n", bmp.bmType, bmp.bmColorKey);
        break;

    case MSG_PAINT: {
        HDC hdc = BeginPaint (hWnd);
        SetBkColor(hdc, COLOR_red);
        SetBkMode(hdc, BM_TRANSPARENT);
        FillBoxWithBitmap(hdc, 0, 0, 0, 0, &bmp);
        FillBoxWithBitmap(hdc, 50, -(int)bmp.bmHeight/2, 0, 0, &bmp);
        FillBoxWithBitmap(hdc, 100, -bmp.bmHeight, 0, 0, &bmp);
        TextOut(hdc, 0, 100, "   Transparent text");

        gal_pixel px = GetPixel(hdc, 2, 105);
        _MG_PRINTF("Pixel value: %x vs bk pixel: %x\n", px, COLOR_lightwhite);
        if (px != COLOR_black)
            passed = FALSE;

        if (passed) {
            SetBkMode(hdc, BM_OPAQUE);
            SetBkColor(hdc, COLOR_green);
            FillBoxWithBitmap(hdc, 200, 0, 0, 0, &bmp);
            FillBoxWithBitmap(hdc, 250, -(int)bmp.bmHeight/2, 0, 0, &bmp);
            FillBoxWithBitmap(hdc, 300, -bmp.bmHeight, 0, 0, &bmp);
            TextOut(hdc, 0, 100, "   Black text in green background");

            gal_pixel px = GetPixel(hdc, 2, 105);
            _MG_PRINTF("Pixel value: %x vs bk pixel: %x\n", px, COLOR_lightwhite);
            if (px != COLOR_green)
                passed = FALSE;
        }

        EndPaint (hWnd, hdc);
        return 0;
    }

    case MSG_IDLE:
        PostMessage(hWnd, MSG_CLOSE, 0, 0);
        break;

    case MSG_CLOSE:
        UnloadBitmap(&bmp);
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

    _MG_PRINTF("Starting test SetBkMode()...\n");

    JoinLayer(NAME_DEF_LAYER , "fillboxwithbitmap" , 0 , 0);

    CreateInfo.dwStyle =
        WS_VISIBLE | WS_BORDER | WS_CAPTION;
    CreateInfo.dwExStyle = WS_EX_NONE;
    CreateInfo.spCaption = "SetBkMode";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = HelloWinProc;
    CreateInfo.lx = 0;
    CreateInfo.ty = 0;
    CreateInfo.rx = g_rcScr.right;
    CreateInfo.by = g_rcScr.bottom;
    CreateInfo.iBkColor = COLOR_black;
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

    if (passed) {
        _MG_PRINTF ("Test for SetBkMode() passed!\n");
        exit(0);
        return 0;
    }

    _MG_PRINTF ("Test for SetBkMode() failed!\n");
    exit(1);
    return 1;
}

