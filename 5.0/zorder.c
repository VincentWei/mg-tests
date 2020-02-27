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
**  Test code of z-order for MiniGUI 5.0.
**
**  This test program creates some main windows in different z-order levels.
**  Every main window will have a distinct background color and size.
**  The program will check the correctness of a z-order operation by reading
**  some pixel values of specific pixels on the screen.
**
**  Under MiniGUI-Threads runtime mode, this program will create 6 GUI threads
*   for all z-order levels other than WS_EX_WINTYPE_NORMAL, and create main
**  windows in different levels in theses threads.
**
**  Under MiniGUI-Processes runtime mode, this program runs as the server and
**  forks 6 children processes for all z-order levels other than
**  WS_EX_WINTYPE_GLOBAL. The children processes create main windows in
**  different levels.
**
**  Note that this program cannot give correct result under compositing schema.
**
**  The following APIs are covered:
**
**      CreateThreadForMainWindow
**      CreateMainWindowEx2
**      DestroyMainWindow
**      MainWindowCleanup
**      SetActiveWindow
**      GetActiveWindow
**      GetPixel
**      DefaultMainWinProc
**      DefaultVirtualWinProc
**      ShowWindow
**      WS_EX_WINTYPE_TOOLTIP
**      WS_EX_WINTYPE_GLOBAL
**      WS_EX_WINTYPE_SCREENLOCK
**      WS_EX_WINTYPE_DOCKER
**      WS_EX_WINTYPE_HIGHER
**      WS_EX_WINTYPE_NORMAL
**      WS_EX_WINTYPE_LAUNCHER
**      WS_ALWAYSTOP
**      MSG_PAINT
**      MSG_IDLE
**      MSG_CREATE
**      MSG_DESTROY
**      HDC_SCREEN
**
** Copyright (C) 2020 FMSoft (http://www.fmsoft.cn).
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
#include <unistd.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#ifndef _MGSCHEMA_COMPOSITING

/* constants defined by MiniGUI */
#define DEF_NR_TOOLTIPS             8
#define DEF_NR_GLOBALS              15
#define DEF_NR_SCREENLOCKS          8
#define DEF_NR_DOCKERS              8
#define DEF_NR_HIGHERS              16
#define DEF_NR_NORMALS              128
#define DEF_NR_LAUNCHERS            8

/* window z-order levels */
enum {
    WIN_LEVEL_TOOLTIP = 0,
    WIN_LEVEL_GLOBAL,
    WIN_LEVEL_SCREENLOCK,
    WIN_LEVEL_DOCKER,
    WIN_LEVEL_HIGHER,
    WIN_LEVEL_NORMAL,
    WIN_LEVEL_LAUNCHER,
    WIN_LEVEL_MAX = WIN_LEVEL_LAUNCHER,
};

/* user-defined messages */
enum {
    MSG_GETWINLEVEL = MSG_USER,
    MSG_TESTWINCREATED,
};

/* notification identifiers */
enum {
    NTID_THREAD_STATUS = 0,
};

/* notification codes */
enum {
    NC_ERR_WINLEVEL = 0,
    NC_ERR_ROOTWND,
    NC_ERR_TESTWND,
    NC_RUN_QUITING,
};

typedef struct win_info {
    HWND    hwnd;
    DWORD   color_bkgnd;
    BOOL    visible;
    BOOL    topmost;
    int     level_expected;
    RECT    rc_window;
} win_info_t;

struct window_template {
    DWORD       type_style;
    DWORD       color_bkgnd;
    DWORD       color_delta;
    RECT        rc_window;
    SIZE        size_delta;
    int         nr_allowed;
    int         nr_created;
    const char *type_name;
    const char *caption;
    win_info_t *windows;
} window_templates [] = {
    { WS_EX_WINTYPE_TOOLTIP,    0xFFFFFF00, 0x00000010,
        { 0, 0, 100, 100 }, { 13, 13 }, DEF_NR_TOOLTIPS,    0,
        "WS_EX_WINTYPE_TOOLTIP",
        "A tooltip window #%d" },

    { WS_EX_WINTYPE_GLOBAL,     0xFFFF00FF, 0x00001000,
        { 0, 0, 200, 200 }, { 17, 17 }, DEF_NR_GLOBALS,     0,
        "WS_EX_WINTYPE_GLOBAL",
        "A global window #%d" },

    { WS_EX_WINTYPE_SCREENLOCK, 0xFF00FFFF, 0x00100000,
        { 0, 0, 300, 300 }, { 7, 7 },   DEF_NR_SCREENLOCKS, 0,
        "WS_EX_WINTYPE_SCREENLOCK",
        "A screenlock window #%d" },

    { WS_EX_WINTYPE_DOCKER,     0xFFFF0000, 0x00001010,
        { 0, 0, 400, 400 }, { 11, 11 }, DEF_NR_DOCKERS,     0,
        "WS_EX_WINTYPE_DOCKER",
        "A docker window #%d" },

    { WS_EX_WINTYPE_HIGHER,     0xFF000000, 0x00030303,
        { 0, 0, 500, 500 }, { 5, 5 },   DEF_NR_HIGHERS,     0,
        "WS_EX_WINTYPE_HIGHER",
        "A higher window #%d" },

    { WS_EX_WINTYPE_NORMAL,     0xFF000000, 0x00010101,
        { 0, 0, 600, 600 }, { 3, 3 },   DEF_NR_NORMALS,     0,
        "WS_EX_WINTYPE_NORMAL",
        "A normal window #%d" },

    { WS_EX_WINTYPE_LAUNCHER,   0xFF00FF00, 0x00100010,
        { 0, 0, 700, 700 }, { 19, 19 }, DEF_NR_LAUNCHERS,   0,
        "WS_EX_WINTYPE_LAUNCHER",
        "A launcher window #%d" },
};

typedef struct test_info {
    HWND main_main_wnd; // the main window in main thread
    HWND root_wnd;      // the root window in the gui thread

    int win_level;      // window level for the thread.
    int nr_thread_wins; // the number of main windows in a thread.
} test_info_t;

static void main_main_wnd_notif_proc (HWND hwnd, LINT id, int nc, DWORD add_data)
{
    test_info_t *info;
    info = (test_info_t *)GetWindowAdditionalData (hwnd);

    assert (info);
    switch (nc) {
        case NC_ERR_WINLEVEL:
            break;

        case NC_ERR_ROOTWND:
            break;

        case NC_ERR_TESTWND:
            break;

        case NC_RUN_QUITING:
            break;

        default:
            _WRN_PRINTF ("unhandled notification code: %d\n", nc);
            break;
    }
}

static void on_test_win_created (HWND hwnd, test_info_t* info,
        win_info_t* win_info)
{
    int level_got = -1;

    assert (info->main_main_wnd == hwnd);

    switch (GetWindowExStyle (win_info->hwnd) & WS_EX_WINTYPE_MASK) {
        case WS_EX_WINTYPE_TOOLTIP:
            level_got = WIN_LEVEL_TOOLTIP;
            break;

        case WS_EX_WINTYPE_GLOBAL:
            level_got = WIN_LEVEL_GLOBAL;
            break;

        case WS_EX_WINTYPE_SCREENLOCK:
            level_got = WIN_LEVEL_SCREENLOCK;
            break;

        case WS_EX_WINTYPE_DOCKER:
            level_got = WIN_LEVEL_DOCKER;
            break;

        case WS_EX_WINTYPE_HIGHER:
            level_got = WIN_LEVEL_HIGHER;
            break;

        case WS_EX_WINTYPE_NORMAL:
            level_got = WIN_LEVEL_NORMAL;
            break;

        case WS_EX_WINTYPE_LAUNCHER:
            level_got = WIN_LEVEL_LAUNCHER;
            break;

        default:
            _WRN_PRINTF ("bad window type\n");
            assert (0);
            return;
    }

    _MG_PRINTF ("A main window created (%s) type (%s)\n",
            GetWindowCaption (win_info->hwnd),
            window_templates[level_got].type_name);

    if (level_got != win_info->level_expected) {
        _WRN_PRINTF ("window (%s) type changed (%s -> %s)\n",
                GetWindowCaption (win_info->hwnd),
                window_templates[win_info->level_expected].type_name,
                window_templates[level_got].type_name);
    }
}

static LRESULT
test_main_win_proc (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case MSG_CREATE:
        SetNotificationCallback (hwnd, main_main_wnd_notif_proc);
        return 0;

    case MSG_GETWINLEVEL: {
#ifndef _MGRM_THREADS
        assert (0);
#else
        static int win_level = WIN_LEVEL_TOOLTIP;
        return win_level++;
#endif
        break;
    }

    case MSG_TESTWINCREATED:
        on_test_win_created (hwnd, (test_info_t *)wparam, (win_info_t *)lparam);
        break;

    case MSG_IDLE:
        _DBG_PRINTF ("got a MSG_IDLE for window: %p\n", hwnd);
        break;

    case MSG_DESTROY:
        return 0;
    }

    return DefaultMainWinProc (hwnd, message, wparam, lparam);
}

static HWND
create_test_main_window (test_info_t* info, HWND hosting, int number)
{
    char            caption[64];
    MAINWINCREATE   create_info;
    win_info_t      win_info;

    win_info.level_expected = info->win_level;
    if (random() % 2) {
        create_info.dwStyle = WS_VISIBLE;
        win_info.visible = TRUE;
    }
    else {
        create_info.dwStyle = WS_NONE;
        win_info.visible = FALSE;
    }
    create_info.dwExStyle = window_templates[info->win_level].type_style;

    sprintf (caption, window_templates[info->win_level].caption, number); 
    create_info.spCaption = caption;
    create_info.hMenu = 0;
    create_info.hCursor = GetSystemCursor(0);
    create_info.hIcon = 0;
    create_info.MainWindowProc = test_main_win_proc;

    win_info.rc_window = window_templates[info->win_level].rc_window;
    for (int i = 0; i < number; i++) {
        win_info.rc_window.right  +=
            window_templates[info->win_level].size_delta.cx;
        win_info.rc_window.bottom +=
            window_templates[info->win_level].size_delta.cy;
    }
    create_info.lx = win_info.rc_window.left;
    create_info.ty = win_info.rc_window.top;
    create_info.rx = win_info.rc_window.right;
    create_info.by = win_info.rc_window.bottom;

    win_info.color_bkgnd = window_templates[info->win_level].color_bkgnd;
    for (int i = 0; i < number; i++) {
        win_info.color_bkgnd += window_templates[info->win_level].color_delta;
    }
    create_info.iBkColor = DWORD2Pixel (HDC_SCREEN, win_info.color_bkgnd);

    create_info.dwAddData = (DWORD)info;
    create_info.hHosting = hosting;

    win_info.hwnd = CreateMainWindow (&create_info);
    if (win_info.hwnd != HWND_INVALID) {
        if (info->main_main_wnd == HWND_NULL) {
            // we are creating main window in main thread
            info->main_main_wnd = win_info.hwnd;
            info->root_wnd = win_info.hwnd;
        }

        SendMessage (info->main_main_wnd, MSG_TESTWINCREATED,
            (WPARAM)info, (LPARAM)&win_info);

        info->nr_thread_wins++;
    }

    return win_info.hwnd;
}

#ifdef _MGRM_THREADS

/* we only created a virtual window as the root window of a new GUI thread
   under MiniGUI-Threads runmode */
static LRESULT
test_root_win_proc (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case MSG_CREATE:
        return 0;

    case MSG_IDLE:
        break;

    case MSG_DESTROY:
        return 0;
    }

    return DefaultVirtualWinProc (hwnd, message, wparam, lparam);
}

static HWND
create_test_virtual_window (test_info_t* info, HWND hosting)
{
    return CreateVirtualWindow (hosting, test_root_win_proc,
            "A virtual window as root", 0, (DWORD)info);
}

static void* test_thread_entry (void* arg)
{
    MSG msg;
    test_info_t info = { (HWND)arg, HWND_NULL, 0, 0 };
    pthread_t self = pthread_self ();

    info.win_level = (int)SendMessage (info.main_main_wnd,
            MSG_GETWINLEVEL, 0, (DWORD)&self);

    if (info.win_level < 0 || info.win_level > WIN_LEVEL_MAX) {
        NotifyWindow (info.main_main_wnd,
                NTID_THREAD_STATUS, NC_ERR_WINLEVEL, (DWORD)&self);
        _ERR_PRINTF ("bad window level: %d\n", info.win_level);
        return NULL;
    }

    info.root_wnd = create_test_virtual_window (&info, HWND_NULL);

    if (info.root_wnd == HWND_INVALID) {
        NotifyWindow (info.main_main_wnd,
                NTID_THREAD_STATUS, NC_ERR_ROOTWND, (DWORD)&self);
        _ERR_PRINTF ("FAILED to create root window\n");
        return NULL;
    }

    int nr_tries = window_templates[info.win_level].nr_allowed + 1;
    for (int i = 0; i < nr_tries; i++) {
        if (create_test_main_window (&info, info.root_wnd, i) == HWND_INVALID) {
            NotifyWindow (info.main_main_wnd,
                    NTID_THREAD_STATUS, NC_ERR_TESTWND, (DWORD)&self);
        }
    }

    while (GetMessage (&msg, info.root_wnd)) {
        DispatchMessage (&msg);
    }

    DestroyVirtualWindow (info.root_wnd);
    VirtualWindowCleanup (info.root_wnd);

    NotifyWindow (info.main_main_wnd,
            NTID_THREAD_STATUS, NC_RUN_QUITING, (DWORD)&self);
    return NULL;
}

#endif /* defined _MGRM_THREADS */

static int test_main_entry (void)
{
    test_info_t info = { HWND_NULL, HWND_NULL, WIN_LEVEL_NORMAL, 0 };

    /* allocate window info for all levels */
    for (int level = WIN_LEVEL_TOOLTIP; level <= WIN_LEVEL_MAX; level++) {
        window_templates[info.win_level].windows =
            calloc (window_templates[info.win_level].nr_allowed,
                    sizeof (win_info_t));
    }

    /* we use dwAddData to record the global test info */
    if (create_test_main_window (&info, HWND_NULL, 0) == HWND_INVALID) {
        _ERR_PRINTF ("FAILED to create the main window in main thread\n");
        return -1;
    }

    /* reset nr_thread_wins for main thread */
    info.nr_thread_wins = 0;

#ifdef _MGRM_THREADS
    /* create message threads */
    for (int level = WIN_LEVEL_TOOLTIP; level <= WIN_LEVEL_MAX; level++) {
        pthread_t th;
        if (CreateThreadForMainWindow (&th, NULL, test_thread_entry,
                    (void*)info.main_main_wnd)) {
            _ERR_PRINTF ("FAILED to create gui thread: %d\n", level);
            return -1;
        }
    }
#else   /* defined _MGRM_THREADS */
    for (int level = WIN_LEVEL_TOOLTIP; level <= WIN_LEVEL_MAX; level++) {
        info.win_level = level;

        int nr_tries = window_templates[info.win_level].nr_allowed + 1;
        for (int i = 0; i < nr_tries; i++) {
            if (create_test_main_window (info, info.main_main_wnd, i)
                    == HWND_INVALID) {
                NotifyWindow (info.main_main_wnd,
                        NTID_THREAD_STATUS, NC_ERR_TESTWND, 0);
            }
        }
    }
#endif  /* not defined _MGRM_THREADS */

    /* enter message loop */
    MSG msg;
    ShowWindow (info.main_main_wnd, SW_SHOWNORMAL);
    while (GetMessage (&msg, info.main_main_wnd)) {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }

    DestroyMainWindow (info.main_main_wnd);
    MainWindowCleanup (info.main_main_wnd);

    /* allocate window info for all levels */
    for (int level = WIN_LEVEL_TOOLTIP; level <= WIN_LEVEL_MAX; level++) {
        free (window_templates[level].windows);
        window_templates[level].windows = NULL;
    }
    return 0;
}

int MiniGUIMain (int argc, const char* argv[])
{
    int nr_loops = 10;

    JoinLayer (NAME_DEF_LAYER , "zorder" , 0 , 0);

    srandom (time(NULL));

    if (argc > 1)
        nr_loops = atoi (argv[1]);
    if (nr_loops < 0)
        nr_loops = 4;

    for (int i = 0; i < nr_loops; i++) {
        _WRN_PRINTF ("Starting loop %d.\n", i);
        if (test_main_entry ())
            return -1;
        _WRN_PRINTF ("==================================\n\n");
    }

    return 0;
}

#else   /* not defined _MGSCHEMA_COMPOSITING */

int MiniGUIMain (int argc, const char* argv[])
{
    _WRN_PRINTF ("This test program cannot run under compositing schema.\n");
    return 0;
}

#endif  /* defined _MGSCHEMA_COMPOSITING */

