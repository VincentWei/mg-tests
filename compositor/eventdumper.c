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
** testdri.c: A test program for MiniGUI dri NEWGAL engine.
**
** Copyright (C) 2019 FMSoft (http://www.fmsoft.cn).
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
** Or,
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
static char key_msg_text [512];
static char char_msg_text [512];
static char button_msg_text [512];
static char touch_msg_text [512];
static char gesture_swipe_msg_text [512];
static char gesture_pinch_msg_text [512];
static char idle_msg_text [512];

static RECT welcome_rc;
static RECT mouse_msg_rc;
static RECT timer_msg_rc;
static RECT key_msg_rc;
static RECT char_msg_rc;
static RECT button_msg_rc;
static RECT touch_msg_rc;
static RECT gesture_swipe_msg_rc;
static RECT gesture_pinch_msg_rc;
static RECT idle_msg_rc;

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

    SetRect (&key_msg_rc,
            10, timer_msg_rc.bottom + 5,
            g_rcScr.right - 10, timer_msg_rc.bottom + 5 + 20);
    key_msg_text[0] = 0;

    SetRect (&char_msg_rc,
            10, key_msg_rc.bottom + 5,
            g_rcScr.right - 10, key_msg_rc.bottom + 5 + 20);
    char_msg_text[0] = 0;

    SetRect (&button_msg_rc,
            10, char_msg_rc.bottom + 5,
            g_rcScr.right - 10, char_msg_rc.bottom + 5 + 20);
    button_msg_text[0] = 0;

    SetRect (&touch_msg_rc,
            10, button_msg_rc.bottom + 5,
            g_rcScr.right - 10, button_msg_rc.bottom + 5 + 20);
    touch_msg_text[0] = 0;

    SetRect (&gesture_swipe_msg_rc,
            10, touch_msg_rc.bottom + 5,
            g_rcScr.right - 10, touch_msg_rc.bottom + 5 + 20);
    gesture_swipe_msg_text[0] = 0;

    SetRect (&gesture_pinch_msg_rc,
            10, gesture_swipe_msg_rc.bottom + 5,
            g_rcScr.right - 10, gesture_swipe_msg_rc.bottom + 5 + 20);
    gesture_pinch_msg_text[0] = 0;

    SetRect (&idle_msg_rc,
            10, gesture_pinch_msg_rc.bottom + 5,
            g_rcScr.right - 10, gesture_pinch_msg_rc.bottom + 5 + 20);
    idle_msg_text[0] = 0;
}

static void on_mouse_message(HWND hwnd, UINT message, int x, int y, DWORD flags)
{
    static const char* format = "%s: x (%d), y (%d), flags (0x%08x)";

    sprintf(mouse_msg_text, format, Message2Str(message), x, y, flags);
    _MG_PRINTF("%s\n", mouse_msg_text);

    InvalidateRect(hwnd, &mouse_msg_rc, TRUE);
}

static void on_timer_message(HWND hwnd, UINT message, LINT id, DWORD tick_count)
{
    static const char* format = "%s: id (%ld), tick count (%lu)";

    sprintf (timer_msg_text, format, Message2Str(message), id, tick_count);
    _MG_PRINTF("%s\n", timer_msg_text);

    InvalidateRect(hwnd, &timer_msg_rc, TRUE);
}

static void on_key_message(HWND hwnd, UINT message, int scancode, DWORD flags)
{
    static const char* format = "%s: scancode (%ld), flags (0x%08x)";

    sprintf(key_msg_text, format, Message2Str(message), scancode, flags);
    _MG_PRINTF("%s\n", key_msg_text);

    InvalidateRect(hwnd, &key_msg_rc, TRUE);
}

static void on_char_message(HWND hwnd, UINT message, const unsigned char* mchar, DWORD flags)
{
    static const char* format = "%s: char (%s), flags (0x%08x)";

    sprintf(char_msg_text, format, Message2Str(message), mchar, flags);
    _MG_PRINTF("%s\n", char_msg_text);

    InvalidateRect(hwnd, &char_msg_rc, TRUE);
}

static void on_button_message(HWND hwnd, UINT message, unsigned int btncode, unsigned int nr_down_btns)
{
    static const char* format = "%s: button (%u), total number pressed (%u)";

    sprintf(button_msg_text, format, Message2Str(message), btncode, nr_down_btns);
    _MG_PRINTF("%s\n", button_msg_text);

    InvalidateRect(hwnd, &button_msg_rc, TRUE);
}

static void on_touch_message(HWND hwnd, UINT message, int x, int y, int slot, unsigned int seat_slot)
{
    static const char* format_pos = "%s: x (%d), y (%d)";
    static const char* format_frame = "%s: slot (%d), seat_slot (%u)";
    static const char* format_others = "%s";

    if (message == MSG_EXIN_TOUCH_DOWN || message == MSG_EXIN_TOUCH_MOTION) {
        sprintf(touch_msg_text, format_pos, Message2Str(message), x, y);
    }
    else if (message == MSG_EXIN_TOUCH_FRAME) {
        sprintf(touch_msg_text, format_frame, Message2Str(message), slot, seat_slot);
    }
    else {
        sprintf(touch_msg_text, format_others, Message2Str(message));
    }

    _MG_PRINTF("%s\n", touch_msg_text);

    InvalidateRect(hwnd, &touch_msg_rc, TRUE);
}

static void on_gesture_swipe_message(HWND hwnd, UINT message, int nr_figs, int dx, int dy, BOOL is_cancelled)
{
    static const char* format_begin = "%s: number figures (%d)";
    static const char* format_update = "%s: number figures (%d), dx (%d), dy (%d)";
    static const char* format_end = "%s: number figures (%d), is cancelled (%s)";

    if (message == MSG_EXIN_GESTURE_SWIPE_BEGIN) {
        sprintf(gesture_swipe_msg_text, format_begin, Message2Str(message), nr_figs);
    }
    else if (message == MSG_EXIN_GESTURE_SWIPE_UPDATE) {
        sprintf(gesture_swipe_msg_text, format_update, Message2Str(message), nr_figs, dx, dy);
    }
    else {
        sprintf(gesture_swipe_msg_text, format_end, Message2Str(message),
                nr_figs, is_cancelled ? "TRUE" : "FALSE");
    }

    _MG_PRINTF("%s\n", gesture_swipe_msg_text);

    InvalidateRect(hwnd, &gesture_swipe_msg_rc, TRUE);
}

static void on_gesture_pinch_message(HWND hwnd, UINT message,
        int nr_figs, unsigned scale, int da, int dx, int dy, BOOL is_cancelled)
{
    static const char* format_begin = "%s: number figures (%d), scale (%u)";
    static const char* format_update = "%s: scale (%u), da (%d), dx (%d), dy (%d)";
    static const char* format_end = "%s: number figures (%d), scale (%u), is cancelled (%s)";

    if (message == MSG_EXIN_GESTURE_PINCH_BEGIN) {
        sprintf(gesture_pinch_msg_text, format_begin, Message2Str(message), nr_figs, scale);
    }
    else if (message == MSG_EXIN_GESTURE_PINCH_UPDATE) {
        sprintf(gesture_pinch_msg_text, format_update, Message2Str(message), scale, da, dx, dy);
    }
    else {
        sprintf(gesture_pinch_msg_text, format_end, Message2Str(message),
                nr_figs, scale, is_cancelled ? "TRUE" : "FALSE");
    }

    _MG_PRINTF("%s\n", gesture_pinch_msg_text);

    InvalidateRect(hwnd, &gesture_pinch_msg_rc, TRUE);
}

static void on_idle_message(HWND hwnd, UINT message)
{
    static const char* format = "%s: tick count (%lu)";

    sprintf(idle_msg_text, format, Message2Str(message), GetTickCount());
    _MG_PRINTF("%s\n", idle_msg_text);

    InvalidateRect(hwnd, &idle_msg_rc, TRUE);
}

HWND create_flying_window (HWND hosting);
void move_flying_window (HWND hwnd);

static HWND flying_window;
static LRESULT EventDumperProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        make_welcome_text ();
        SetTimer (hwnd, 100, 100);
        break;

    case MSG_TIMER: {
        int timer_id = (int)wParam;
        DWORD tick_count = (DWORD)lParam;
        static DWORD moved_times;

        moved_times++;
        on_timer_message(hwnd, message, timer_id, tick_count);
        move_flying_window(flying_window);
        if (moved_times >= 2000) {
            KillTimer (hwnd, 100);
            SendNotifyMessage(flying_window, MSG_CLOSE, 0, 0);
            SendNotifyMessage(hwnd, MSG_CLOSE, 0, 0);
        }
        break;
    }

    case MSG_FIRSTMOUSEMSG ... MSG_LASTMOUSEMSG: {
        DWORD key_flags = (DWORD)wParam;
        int x_pos = LOSWORD (lParam);
        int y_pos = HISWORD (lParam);
        on_mouse_message(hwnd, message, x_pos, y_pos, key_flags);
        break;
    }

    case MSG_FIRSTKEYMSG ... MSG_LASTKEYMSG: {
        DWORD flags = (DWORD)lParam;
        if (message == MSG_CHAR) {
            unsigned char mchar[5];
            mchar[0] = FIRSTBYTE(wParam);
            mchar[1] = SECONDBYTE(wParam);
            mchar[2] = THIRDBYTE(wParam);
            mchar[3] = FOURTHBYTE(wParam);
            mchar[4] = 0;
            on_char_message(hwnd, message, mchar, flags);
        }
        else if (message == MSG_SYSCHAR) {
            unsigned char mchar[2];
            mchar[0] = (unsigned char)wParam;
            mchar[1] = 0;
            on_char_message(hwnd, message, mchar, flags);
        }
        else if (message == MSG_KEYSYM) {
            static unsigned char symbols[8];
            int index = HIBYTE (wParam);
            int keysym = LOBYTE (wParam);

            if (index >= 0 && index < 8)
                symbols[index] = (unsigned char)keysym;
            else {
                _WRN_PRINTF("Got a bad MSG_KEYSYM message");
            }

            if (keysym == 0)
                on_char_message(hwnd, message, symbols, flags);
        }
        else if (message == MSG_UTF8CHAR) {
            unsigned char ch_utf8 [7];
            ch_utf8 [0] = FIRSTBYTE(wParam);
            ch_utf8 [1] = SECONDBYTE(wParam);
            ch_utf8 [2] = THIRDBYTE(wParam);
            ch_utf8 [3] = FOURTHBYTE(wParam);
            ch_utf8 [4] = FIRSTBYTE(lParam);
            ch_utf8 [5] = SECONDBYTE(lParam);
            ch_utf8 [6] = 0;

            on_char_message(hwnd, message, ch_utf8, flags);
        }
        else {
            int scancode = (int)wParam;
            on_key_message(hwnd, message, scancode, flags);
        }

        break;
    }

    case MSG_EXIN_BUTTONDOWN:
    case MSG_EXIN_BUTTONUP: {
        unsigned int btncode = (unsigned int)wParam;
        unsigned int nr_pressed = (unsigned int)lParam;
        on_button_message(hwnd, message, btncode, nr_pressed);
        break;
    }

    case MSG_EXIN_TOUCH_DOWN:
    case MSG_EXIN_TOUCH_MOTION: {
        on_touch_message(hwnd, message, LOSWORD(lParam), LOSWORD(lParam), 0, 0);
        break;
    }

    case MSG_EXIN_TOUCH_FRAME: {
        int slot = (int)wParam;
        unsigned seat_slot = (unsigned)lParam;
        on_touch_message(hwnd, message, 0, 0, slot, seat_slot);
        break;
    }

    case MSG_EXIN_GESTURE_SWIPE_BEGIN: {
        int nr_figs = (int)wParam;
        on_gesture_swipe_message(hwnd, message, nr_figs, 0, 0, FALSE);
        break;
    }

    case MSG_EXIN_GESTURE_SWIPE_UPDATE: {
        int nr_figs = (int)wParam;
        int dx = LOSWORD(lParam);
        int dy = HISWORD(lParam);
        on_gesture_swipe_message(hwnd, message, nr_figs, dx, dy, FALSE);
        break;
    }

    case MSG_EXIN_GESTURE_SWIPE_END: {
        int nr_figs = (int)wParam;
        BOOL is_cancelled = (BOOL)lParam;
        on_gesture_swipe_message(hwnd, message, nr_figs, 0, 0, is_cancelled);
        break;
    }

    case MSG_EXIN_GESTURE_PINCH_BEGIN: {
        int nr_figs = (int)wParam;
        unsigned int scale = (unsigned int)lParam;
        on_gesture_pinch_message(hwnd, message, nr_figs, scale, 0, 0, 0, FALSE);
        break;
    }

    case MSG_EXIN_GESTURE_PINCH_UPDATE: {
        unsigned int scale = LOWORD(wParam);
        int da = HISWORD(wParam);
        int dx = LOSWORD(lParam);
        int dy = HISWORD(lParam);
        on_gesture_pinch_message(hwnd, message, 0, scale, da, dx, dy, FALSE);
        break;
    }

    case MSG_EXIN_GESTURE_PINCH_END: {
        int nr_figs = (int)LOSWORD(wParam);
        BOOL is_cancelled = (BOOL)HISWORD(wParam);
        unsigned int scale = (unsigned int)lParam;
        on_gesture_pinch_message(hwnd, message, nr_figs, scale, 0, 0, 0, is_cancelled);
        break;
    }

    case MSG_IDLE: {
        on_idle_message(hwnd, message);
    }

    case MSG_PAINT: {
        HDC hdc;

        hdc = BeginPaint (hwnd);
        DrawText (hdc, welcome_text, -1, &welcome_rc, DT_LEFT | DT_WORDBREAK);
        DrawText (hdc, mouse_msg_text, -1, &mouse_msg_rc, DT_LEFT | DT_WORDBREAK);
        DrawText (hdc, timer_msg_text, -1, &timer_msg_rc, DT_LEFT | DT_WORDBREAK);
        DrawText (hdc, key_msg_text, -1, &key_msg_rc, DT_LEFT | DT_WORDBREAK);
        DrawText (hdc, char_msg_text, -1, &char_msg_rc, DT_LEFT | DT_WORDBREAK);
        DrawText (hdc, button_msg_text, -1, &button_msg_rc, DT_LEFT | DT_WORDBREAK);
        DrawText (hdc, touch_msg_text, -1, &touch_msg_rc, DT_LEFT | DT_WORDBREAK);
        DrawText (hdc, gesture_swipe_msg_text, -1, &gesture_swipe_msg_rc, DT_LEFT | DT_WORDBREAK);
        DrawText (hdc, gesture_pinch_msg_text, -1, &gesture_pinch_msg_rc, DT_LEFT | DT_WORDBREAK);
        DrawText (hdc, idle_msg_text, -1, &idle_msg_rc, DT_LEFT | DT_WORDBREAK);
        EndPaint (hwnd, hdc);
        return 0;
    }

    case MSG_CLOSE:
        DestroyMainWindow (hwnd);
        PostQuitMessage (hwnd);
        return 0;
    }

    return DefaultMainWinProc(hwnd, message, wParam, lParam);
}

/* change value of this variable to disable i915 driver */
int dridriver_enabled = 1;

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
    CreateInfo.spCaption = "Event Dumper";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = EventDumperProc;
    CreateInfo.lx = g_rcScr.right - 400;
    CreateInfo.ty = g_rcScr.bottom - 300;
    CreateInfo.rx = g_rcScr.right;
    CreateInfo.by = g_rcScr.bottom;
    CreateInfo.iBkColor = COLOR_lightwhite;
    CreateInfo.dwAddData = 0;
    CreateInfo.hHosting = HWND_DESKTOP;

    hMainWnd = CreateMainWindow (&CreateInfo);
    if (hMainWnd == HWND_INVALID)
        return -1;

    flying_window = create_flying_window (hMainWnd);
    if (flying_window == HWND_INVALID) {
        DestroyMainWindow (hMainWnd);
        MainWindowThreadCleanup (hMainWnd);
        return -2;
    }

    ShowWindow(hMainWnd, SW_SHOWNORMAL);
    SetActiveWindow(hMainWnd);
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

