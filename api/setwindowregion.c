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
**      GetWindowRect
**      InitFreeClipRectList
**      InitClipRgn
**      InitCircleRegion
**      SetWindowRegion
**      EmptyClipRgn
**      DestroyFreeClipRectList
**      MainWindowCleanup
**      PostQuitMessage
**      PostMessage
**      SetTimer
**      KillTimer
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

#define HL_ST_CAP       "Hello World!"
#define HL_ST_NOMES     "No messageso far."
#define HL_ST_TIMER     "Timer expired, current tick count: %p."
#define HL_ST_LBD       "The left button pressed."
#define HL_ST_LBU       "The left button released."
#define HL_ST_RBD       "The right button pressed."
#define HL_ST_RBU       "The right button released."
#define HL_ST_SYS       "sys"
#define HL_ST_KEYD      "The %d %s key pressed %d times."
#define HL_ST_KEYLONG   "=======The %d key pressed over a long term"
#define HL_ST_KEYALWAY  "=======The %d key pressed always"
#define HL_ST_KEYU      "The %d key released"

static char welcome_text [512];
static char msg_text [256];
static RECT welcome_rc = {10, 100, 600, 400};
static RECT msg_rc = {10, 100, 600, 400};

static const char* syskey = "";

static int last_key = -1;
static int last_key_count = 0;

static void make_welcome_text (void)
{
    const char* sys_charset = GetSysCharset (TRUE);
    const char* format;

    if (sys_charset == NULL)
        sys_charset = GetSysCharset (FALSE);

    SetRect (&welcome_rc,  10, 10, g_rcScr.right - 10, g_rcScr.bottom / 2 - 10);
    SetRect (&msg_rc, 10, welcome_rc.bottom + 10, g_rcScr.right - 10, g_rcScr.bottom - 20);

    if (strcmp (sys_charset, FONT_CHARSET_GB2312_0) == 0 
            || strcmp (sys_charset, FONT_CHARSET_GBK) == 0
            || strcmp (sys_charset, FONT_CHARSET_GB18030_0) == 0) {
        format = "欢迎来到 MiniGUI 的世界! 如果您能看到该文本, 则说明 MiniGUI Version %d.%d.%d 可在该硬件上运行!";
    }
    else if (strcmp (sys_charset, FONT_CHARSET_BIG5) == 0) {
        format = "欢迎来到 MiniGUI 的世界! 如果您能看到该文本, 则说明 MiniGUI Version %d.%d.%d 可在该硬件上运行!";
    }
    else {
        format = "Welcome to the world of MiniGUI. \nIf you can see this text, MiniGUI Version %d.%d.%d can run on this hardware board.";
    }

    sprintf (welcome_text, format, MINIGUI_MAJOR_VERSION, MINIGUI_MINOR_VERSION, MINIGUI_MICRO_VERSION);

    strcpy (msg_text, HL_ST_NOMES);
}

static LRESULT HelloWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;

    syskey = "";

    switch (message) {
        case MSG_CREATE:
            make_welcome_text ();
            SetTimer (hWnd, 100, 20);
            break;

        case MSG_TIMER: {
            static int count;

            count++;
            if (count == 10)
                PostMessage(hWnd, MSG_CLOSE, 0, 0);

            _MG_PRINTF("Updating window\n");
            sprintf (msg_text, HL_ST_TIMER, (PVOID)GetTickCount ());
            InvalidateRect (hWnd, &msg_rc, TRUE);
            break;
        }

        case MSG_LBUTTONDOWN:
            strcpy (msg_text, HL_ST_LBD);
            InvalidateRect (hWnd, &msg_rc, TRUE);
            break;

        case MSG_LBUTTONUP:
            strcpy (msg_text, HL_ST_LBU);
            InvalidateRect (hWnd, &msg_rc, TRUE);
            break;

        case MSG_RBUTTONDOWN:
            strcpy (msg_text, HL_ST_RBD);
            InvalidateRect (hWnd, &msg_rc, TRUE);
            break;

        case MSG_RBUTTONUP:
            strcpy (msg_text, HL_ST_RBU);
            InvalidateRect (hWnd, &msg_rc, TRUE);
            break;

        case MSG_PAINT:
            hdc = BeginPaint (hWnd);
            DrawText (hdc, welcome_text, -1, &welcome_rc, DT_LEFT | DT_WORDBREAK);
            DrawText (hdc, msg_text, -1, &msg_rc, DT_LEFT | DT_WORDBREAK);
            EndPaint (hWnd, hdc);
            return 0;

        case MSG_SYSKEYDOWN:
            syskey = HL_ST_SYS;
        case MSG_KEYDOWN:
            if (last_key == wParam)
                last_key_count++;
            else {
                last_key = wParam;
                last_key_count = 1;
            }
            sprintf (msg_text, HL_ST_KEYD, 
                            (int)wParam, syskey, last_key_count);
            InvalidateRect (hWnd, &msg_rc, TRUE);
            return 0;

        case MSG_KEYLONGPRESS:
            sprintf (msg_text, HL_ST_KEYLONG, (int)wParam);
            InvalidateRect (hWnd, &msg_rc, TRUE);
            break;

        case MSG_KEYALWAYSPRESS:
            sprintf (msg_text, HL_ST_KEYALWAY, (int)wParam);
            InvalidateRect (hWnd, &msg_rc, TRUE);
            break;

        case MSG_KEYUP:
            sprintf (msg_text, HL_ST_KEYU, (int)wParam);
            InvalidateRect (hWnd, &msg_rc, TRUE);
            return 0;

        case MSG_CLOSE:
            KillTimer (hWnd, 100);
            DestroyMainWindow (hWnd);
            PostQuitMessage (hWnd);
            return 0;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

static void set_circle_region (HWND hwnd)
{
    RECT rc;
    BLOCKHEAP cliprc_heap;
    CLIPRGN circle_rgn;

    GetWindowRect(hwnd, &rc);

    InitFreeClipRectList(&cliprc_heap, 50);

    InitClipRgn(&circle_rgn, &cliprc_heap);
    InitCircleRegion (&circle_rgn, 100, 100, 100);

    if (!SetWindowRegion(hwnd, &circle_rgn)) {
        printf("Error calling SetWindowRegion.\n");
    }

    EmptyClipRgn (&circle_rgn);
    DestroyFreeClipRectList (&cliprc_heap);
}

int MiniGUIMain (int argc, const char* argv[])
{
    MSG Msg;
    HWND hMainWnd;
    MAINWINCREATE CreateInfo;

    _MG_PRINTF("Starting test SetWindowRegion()...\n");

    RECT rc_scr = GetScreenRect();
    _MG_PRINTF("Screen rect: %d, %d, %d, %d\n",
            rc_scr.left, rc_scr.top,
            rc_scr.right, rc_scr.bottom);

    JoinLayer(NAME_DEF_LAYER , "setwindowregion" , 0 , 0);

    CreateInfo.dwStyle = 
        WS_VISIBLE | WS_BORDER | WS_CAPTION;
    CreateInfo.dwExStyle = WS_EX_NONE;
    CreateInfo.spCaption = HL_ST_CAP;
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

    set_circle_region (hMainWnd);

    while (GetMessage(&Msg, hMainWnd)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    MainWindowCleanup (hMainWnd);

    _MG_PRINTF ("Test for SetWindowRegion() passed!\n");
    exit(0);

    return 0;
}

