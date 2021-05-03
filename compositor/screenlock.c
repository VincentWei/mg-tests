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
** The dockerbar
**
** Copyright (C) 2003 ~ 2021 FMSoft (http://www.fmsoft.cn).
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
#include <unistd.h>
#include <time.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#define _ID_TIME_STATIC     110

#define _MARGIN             2
#define _HEIGHT_CTRL        24
#define _WIDTH_TIME         52
#define _WIDTH_APPS         250
#define _ID_TIMER           100
#define _MAX_WIDTH_LAYER_BOX    80
#define _MIN_WIDTH_LAYER_BOX    20

static char* mk_time (char* buff)
{
    time_t t;
    struct tm *tm;

    time (&t);
    tm = localtime (&t);
    sprintf (buff, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);

    return buff;
}

static LRESULT MyMainWinProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    char buff [20];

    switch (message) {
    case MSG_CREATE:
        CreateWindow (CTRL_STATIC, mk_time (buff), WS_CHILD | WS_BORDER | WS_VISIBLE | SS_CENTER, 
                    _ID_TIME_STATIC, (g_rcScr.right - _WIDTH_TIME) / 2, (g_rcScr.bottom - _HEIGHT_CTRL) / 2,
                    _WIDTH_TIME, _HEIGHT_CTRL, hWnd, 0);

        SetTimer (hWnd, _ID_TIMER, 100);
        break;

    case MSG_TIMER:
        SetDlgItemText (hWnd, _ID_TIME_STATIC, mk_time (buff));
        return 0;
        
    case MSG_CLOSE:
        KillTimer (hWnd, _ID_TIMER);
        DestroyAllControls (hWnd);
        DestroyMainWindow (hWnd);
        PostQuitMessage (hWnd);
        return 0;
    }

    return DefaultMainWinProc (hWnd, message, wParam, lParam);
}

int MiniGUIMain (int argc, const char* argv[])
{
    MAINWINCREATE CreateInfo;
    HWND hMainWnd;
    MSG Msg;

#ifdef _MGRM_PROCESSES
    JoinLayer (NAME_DEF_LAYER, "screenlock", 0, 0);
#endif

    CreateInfo.dwStyle = WS_VISIBLE;
    CreateInfo.dwExStyle = WS_EX_TOOLWINDOW | WS_EX_WINTYPE_SCREENLOCK;
    CreateInfo.spCaption = "Screen Lock";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor (0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = MyMainWinProc;
    CreateInfo.lx = g_rcScr.left; 
    CreateInfo.ty = g_rcScr.top;
    CreateInfo.rx = g_rcScr.right;
    CreateInfo.by = g_rcScr.bottom;
    CreateInfo.iBkColor =
        GetWindowElementPixelEx (HWND_NULL, HDC_SCREEN, WE_MAINC_THREED_BODY); 
    CreateInfo.dwAddData = 0;
    CreateInfo.hHosting = HWND_DESKTOP;

    hMainWnd = CreateMainWindowEx2 (&CreateInfo, 0L, NULL, NULL,
            ST_PIXEL_ARGB8888,
            MakeRGBA (SysPixelColor[IDX_COLOR_darkgray].r,
                SysPixelColor[IDX_COLOR_darkgray].g,
                SysPixelColor[IDX_COLOR_darkgray].b,
                0xA0),
            CT_ALPHAPIXEL, COLOR_BLEND_SP_LIGHTEN);

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

