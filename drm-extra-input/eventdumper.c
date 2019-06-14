/*
** helloworld.c: Sample program for MiniGUI Programming Guide
**      The first MiniGUI application.
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

#if (_MINIGUI_VERSION_CODE >= _VERSION_CODE(4,0,0)) \
        && defined(_MGHAVE_MSG_STRING)

static char welcome_text [512];
static char mouse_msg_text [512];
static char timer_msg_text [512];

static RECT welcome_rc;
static RECT mouse_msg_rc;
static RECT timer_msg_rc;

static void make_welcome_text (void)
{
    const char* format =
        "Welcome to the world of MiniGUI.\n"
        "If you can see this text, MiniGUI %d.%d.%d is running on this target.";

    sprintf (welcome_text, format,
        MINIGUI_MAJOR_VERSION, MINIGUI_MINOR_VERSION, MINIGUI_MICRO_VERSION);

    SetRect (&welcome_rc,
            10, 10,
            g_rcScr.right - 10, 10 + 40);

    SetRect (&mouse_msg_rc,
            10, welcome_rc.bottom + 5,
            g_rcScr.right - 10, welcome_rc.bottom + 5 + 20);
    mouse_msg_text[0] = 0;

    SetRect (&timer_msg_rc,
            10, mouse_msg_rc.bottom + 5,
            g_rcScr.right - 10, mouse_msg_rc.bottom + 5 + 20);
    timer_msg_text[0] = 0;
}

static void on_mouse_message(HWND hwnd, UINT message, int x, int y, DWORD flags)
{
    static const char* format = "%s: x (%d), y (%d), flags (0x%08x)";

    sprintf (mouse_msg_text, format, Message2Str(message), x, y);
    InvalidateRect(hwnd, &mouse_msg_rc, TRUE);
}

static void on_timer_message(HWND hwnd, UINT message, LINT id, DWORD tick_count)
{
    static const char* format = "%s: id (%ld), tick count (%lu)";

    sprintf (timer_msg_text, format, Message2Str(message), id, tick_count);
    InvalidateRect(hwnd, &timer_msg_rc, TRUE);
}

static LRESULT EventDumperProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        make_welcome_text ();
        SetTimer (hWnd, 100, 200);
        break;

    case MSG_TIMER: {
        int timer_id = (int)wParam;
        DWORD tick_count = (DWORD)lParam;
        on_timer_message(hWnd, message, timer_id, tick_count);
        break;
    }

    case MSG_FIRSTMOUSEMSG ... MSG_LASTMOUSEMSG: {
        DWORD key_flags = (DWORD)wParam;
        int x_pos = LOSWORD (lParam);
        int y_pos = HISWORD (lParam);
        on_mouse_message(hWnd, message, x_pos, y_pos, key_flags);
        break;
    }

    case MSG_PAINT: {
        HDC hdc;

        hdc = BeginPaint (hWnd);
        DrawText (hdc, welcome_text, -1, &welcome_rc, DT_LEFT | DT_WORDBREAK);
        DrawText (hdc, mouse_msg_text, -1, &mouse_msg_rc, DT_LEFT | DT_WORDBREAK);
        DrawText (hdc, timer_msg_text, -1, &timer_msg_rc, DT_LEFT | DT_WORDBREAK);
        EndPaint (hWnd, hdc);
        return 0;
    }

    case MSG_CLOSE:
        KillTimer (hWnd, 100);
        DestroyMainWindow (hWnd);
        PostQuitMessage (hWnd);
        return 0;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

int MiniGUIMain (int argc, const char* argv[])
{
    MSG Msg;
    HWND hMainWnd;
    MAINWINCREATE CreateInfo;

#ifdef _MGRM_PROCESSES
    JoinLayer(NAME_DEF_LAYER , "eventdumper" , 0 , 0);
#endif

    CreateInfo.dwStyle = WS_VISIBLE | WS_BORDER | WS_CAPTION;
    CreateInfo.dwExStyle = WS_EX_NONE;
    CreateInfo.spCaption = "Input Event Dumper";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = EventDumperProc;
    CreateInfo.lx = 0;
    CreateInfo.ty = 0;
    CreateInfo.rx = g_rcScr.right;
    CreateInfo.by = g_rcScr.bottom;
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

#ifdef _MGRM_THREADS
#include <minigui/dti.c>
#endif

#else
#error "To compile this program, please use MiniGUI 4.0.x and configure MiniGUI with --enable-msgstr"
#endif

