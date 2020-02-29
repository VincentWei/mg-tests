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
**  Under MiniGUI-Threads runtime mode, this program will create 7 GUI threads
**  for all z-order levels and create main windows in different levels in
**  theses threads.
**
**  Under MiniGUI-Processes runtime mode, this program will
**  forks 6 children processes for all z-order levels other than
**  WS_EX_WINTYPE_GLOBAL. The children processes create main windows in
**  different levels.
**
**  Note that this program cannot give correct result under compositing schema.
**
**  The following APIs are covered:
**
**      CreateThreadForMainWindow
**      CreateMainWindow
**      DestroyMainWindow
**      MainWindowCleanup
**      CreateVirtualWindow
**      DestroyVirtualWindow
**      VirtualWindowCleanup
**      PostQuitMessage
**      DefaultMainWinProc
**      DefaultVirtualWinProc
**      ShowWindow
**      MoveWindow
**      GetWindowAdditionalData
**      SetWindowAdditionalData
**      GetWindowAdditionalData2
**      SetWindowAdditionalData2
**      SendMessage
**      NotifyWindow
**      GetPixel
**      SetTimer
**      GetNextMainWindow
**      GetWindowRect
**      WS_EX_WINTYPE_TOOLTIP
**      WS_EX_WINTYPE_GLOBAL
**      WS_EX_WINTYPE_SCREENLOCK
**      WS_EX_WINTYPE_DOCKER
**      WS_EX_WINTYPE_HIGHER
**      WS_EX_WINTYPE_NORMAL
**      WS_EX_WINTYPE_LAUNCHER
**      MSG_PAINT
**      MSG_TIMER
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

#include "list.h"

/* constants defined by MiniGUI Core */
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
    WIN_LEVEL_MIN = WIN_LEVEL_TOOLTIP,
    WIN_LEVEL_MAX = WIN_LEVEL_LAUNCHER,
};

#define IS_WIN_LEVEL_VALID(level)   \
    ((level) >= WIN_LEVEL_MIN && (level) <= WIN_LEVEL_MAX)

/* user-defined messages */
enum {
    /* Messages sent to main main window from a message thread. */
    MSG_MTH_READY = MSG_USER,
    MSG_MTH_QUIT,

    /* Messages sent to main main window from a test window. */
    MSG_GETWINLEVEL,
    MSG_TESTWINCREATED,
    MSG_TESTWINSHOWN,
    MSG_TESTWINMOVED,
    MSG_TESTWINDESTROYED,

    /* Messages sent to test main window from main main window. */
    MSG_ZORDER_OP_DESTROY, 
    MSG_ZORDER_OP_SHOW,
    MSG_ZORDER_OP_HIDE,
    MSG_ZORDER_OP_RAISE,
    MSG_ZORDER_OP_MOVE,
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
    struct list_head    list;
    HWND                hwnd;
    DWORD               color_bkgnd;
    BOOL                visible;
    int                 level_expected;
    int                 level_got;
    RECT                rc_window;
    RECT                rc_exp;
} win_info_t;

static struct window_template {
    DWORD       type_style;
    DWORD       color_bkgnd;
    DWORD       color_delta;
    RECT        rc_window;
    SIZE        size_delta;
    int         nr_allowed;
    int         nr_created;
    const char *type_name;
    const char *caption;

    struct list_head list_wins;
#if 0
    win_info_t *windows;
#endif
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

static const win_info_t *get_window_at_point (int x, int y)
{
    int level;

    for (level = WIN_LEVEL_MIN; level <= WIN_LEVEL_MAX; level++) {
        struct list_head *info;

        list_for_each (info, &window_templates[level].list_wins) {
            win_info_t *my_win_info = (win_info_t*)info;
            if (my_win_info->visible &&
                    PtInRect (&my_win_info->rc_window, x, y)) {
                return my_win_info;
            }
        }
    }

    return NULL;
}

static void print_windows_by_pixel (gal_pixel pixel)
{
    for (int level = WIN_LEVEL_MIN; level <= WIN_LEVEL_MAX; level++) {
        struct list_head *info;

        list_for_each (info, &window_templates[level].list_wins) {
            win_info_t *win_info = (win_info_t *)info;
            gal_pixel pixel_window = DWORD2Pixel (HDC_SCREEN,
                    win_info->color_bkgnd);

            if (pixel_window == pixel) {
                _ERR_PRINTF ("pixel (%08x) on screen "
                    "is background 0x%08lx of window (%s), "
                    "rect (%d, %d, %d, %d), visibility: %s\n",
                    pixel, win_info->color_bkgnd,
                    GetWindowCaption (win_info->hwnd),
                    win_info->rc_window.left,
                    win_info->rc_window.top,
                    win_info->rc_window.right,
                    win_info->rc_window.bottom,
                    win_info->visible ? "TRUE" : "FALSE");
            }
        }
    }
}

static int print_windows_in_level (int level)
{
    int nr = 0;
    struct list_head *info;

    list_for_each (info, &window_templates[level].list_wins) {
        win_info_t *win_info = (win_info_t *)info;

        _ERR_PRINTF ("window (%s) in level (%d): background (0x%08lx), "
            "rect (%d, %d, %d, %d), visibility: %s\n",
            GetWindowCaption (win_info->hwnd), level,
            win_info->color_bkgnd,
            win_info->rc_window.left,
            win_info->rc_window.top,
            win_info->rc_window.right,
            win_info->rc_window.bottom,
            win_info->visible ? "TRUE" : "FALSE");

        nr++;
    }

    SendMessage (HWND_DESKTOP, MSG_DUMPZORDER, 0, 0);

    return nr;
}

static int check_zorder_by_apis (void)
{
    HWND tm_api = GetNextMainWindow (HWND_NULL);    // tm: topmost
    HWND bm_api = HWND_NULL;                        // bm: bottommost
    HWND next = HWND_NULL;
    
    _WRN_PRINTF ("the topmost window got by APIs: %p (%s)\n",
            tm_api, GetWindowCaption (tm_api));

    next = tm_api;
    while (next) {
        bm_api = next;
        next = GetNextMainWindow (next);
    }

    _WRN_PRINTF ("the bottommost window got by APIs: %p (%s)\n",
            bm_api, GetWindowCaption (bm_api));

    HWND tm_me = HWND_NULL, bm_me = HWND_NULL;

    for (int level = WIN_LEVEL_MIN; level <= WIN_LEVEL_MAX; level++) {
        struct list_head *info;

        list_for_each (info, &window_templates[level].list_wins) {
            win_info_t *win_info = (win_info_t *)info;

            if (tm_me == HWND_NULL)
                tm_me = win_info->hwnd;
            bm_me = win_info->hwnd;
        }
    }

    _WRN_PRINTF ("the topmost window got from my zorder: %p (%s)\n",
            tm_me, GetWindowCaption (tm_me));

    _WRN_PRINTF ("the bottommost window got from my zorder: %p (%s)\n",
            bm_me, GetWindowCaption (bm_me));

    assert (tm_api == tm_me);
    assert (bm_api == bm_me);
    return 0;
}

static int check_zorder_by_pixels (void)
{
    int nr = 0;
    int x, y;
    gal_pixel pixel_desktop = GetWindowElementPixelEx (HWND_DESKTOP,
            HDC_SCREEN, WE_BGC_DESKTOP);

    /* check some pixels on the diagonal line */
    for (x = 0, y = 0; x < g_rcScr.right && y < g_rcScr.bottom; x += 3, y += 3) {
        const win_info_t* win_info;
        gal_pixel pixel_screen = GetPixel (HDC_SCREEN, x, y);

        win_info = get_window_at_point (x, y);
        if (win_info) {
            gal_pixel pixel_window = DWORD2Pixel (HDC_SCREEN,
                    win_info->color_bkgnd);

            if (pixel_screen != pixel_window) {
                _ERR_PRINTF ("pixel (%d, %d) on screen: %08x\n",
                        x, y, pixel_screen);
                _ERR_PRINTF ("pixel (%d, %d) on screen "
                        "do not match background 0x%08lx of window (%s), "
                        "rect (%d, %d, %d, %d)\n",
                        x, y, win_info->color_bkgnd,
                        GetWindowCaption (win_info->hwnd),
                        win_info->rc_window.left, win_info->rc_window.top,
                        win_info->rc_window.right, win_info->rc_window.bottom);

                if (pixel_screen == pixel_desktop) {
                    _ERR_PRINTF ("pixel (%d, %d) on screen "
                            "is background pixel of desktop\n", x, y);
                }
                else {
                    print_windows_by_pixel (pixel_screen);
                }

                print_windows_in_level (win_info->level_got);

                assert (0);
                nr++;
            }
        }
        else {
            if (pixel_screen != pixel_desktop) {
                _ERR_PRINTF ("pixel (%d, %d) on screen: %08x\n",
                        x, y, pixel_screen);
                _ERR_PRINTF ("pixel (%d, %d) on screen "
                        "do not match background of desktop\n",
                        x, y);

                print_windows_by_pixel (pixel_screen);
                print_windows_in_level (win_info->level_got);

                assert (0);
                nr++;
            }
        }
    }

    return nr;
}

static int add_new_window_in_level (int level, const win_info_t* win_info)
{
    struct list_head *info;
    win_info_t *new_win_info;

    list_for_each (info, &window_templates[level].list_wins) {
        win_info_t *my_win_info = (win_info_t*)info;
        if (my_win_info->hwnd == win_info->hwnd) {
            _ERR_PRINTF ("window (%p) existed in level (%d)\n",
                    win_info->hwnd, level);
            assert (0);
            return -1;
        }
    }

    if ((new_win_info = mg_slice_new (win_info_t)) == NULL) {
        _WRN_PRINTF ("failed to allocate memory for window info\n");
        return -1;
    }

    memcpy (new_win_info, win_info, sizeof (win_info_t));
    list_add (&new_win_info->list, &window_templates[level].list_wins);

    window_templates[level].nr_created++;
    return 0;
}


GET_ENTRY_BY_INDEX(get_win_info_by_idx, win_info_t, list)

static int mark_window_as_visible_in_level (int level, HWND hwnd, BOOL show_hide)
{
    struct list_head *info, *tmp;
    win_info_t *found = NULL;

    list_for_each_safe (info, tmp, &window_templates[level].list_wins) {
        win_info_t *win_info = (win_info_t *)info;
        if (win_info->hwnd == hwnd) {
            found = win_info;
            break;
        }
    }

    if (found == NULL) {
        _ERR_PRINTF ("window (%p) not found in level (%d)\n", hwnd, level);
        assert (0);
        return -1;
    }

    found->visible = show_hide;
    return 0;
}

static int raise_window_to_top_in_level (int level, HWND hwnd)
{
    int idx = 0;
    struct list_head *info;
    win_info_t *found = NULL;

    list_for_each (info, &window_templates[level].list_wins) {
        win_info_t *win_info = (win_info_t *)info;
        if (win_info->hwnd == hwnd) {
            found = win_info;
            break;
        }
        idx++;
    }

    if (found == NULL) {
        _ERR_PRINTF ("window (%p) not found in level (%d)\n", hwnd, level);
        assert (0);
        return -1;
    }

    _DBG_PRINTF ("window #%d (%s) is visible (%d: %d)\n",
            idx, GetWindowCaption (found->hwnd),
            found->visible, IsWindowVisible (found->hwnd));

    found->visible = TRUE;
    assert (IsWindowVisible (found->hwnd));

    list_del (&found->list);
    list_add (&found->list, &window_templates[level].list_wins);
    return 0;
}

static int change_window_rect_in_level (int level, HWND hwnd)
{
    struct list_head *info;
    win_info_t *found = NULL;

    list_for_each (info, &window_templates[level].list_wins) {
        win_info_t *win_info = (win_info_t *)info;
        if (win_info->hwnd == hwnd) {
            found = win_info;
            break;
        }
    }

    if (found == NULL) {
        _ERR_PRINTF ("window (%p) not found in level (%d)\n", hwnd, level);
        assert (0);
        return -1;
    }

    found->rc_window = found->rc_exp;
    return 0;
}

static int remove_window_in_level (int level, HWND hwnd)
{
    struct list_head *info;
    win_info_t *found = NULL;

    list_for_each (info, &window_templates[level].list_wins) {
        win_info_t *win_info = (win_info_t *)info;
        if (win_info->hwnd == hwnd) {
            found = win_info;
            break;
        }
    }

    if (found == NULL) {
        _ERR_PRINTF ("window (%p) not found in level (%d)\n", hwnd, level);
        assert (0);
        return -1;
    }

    list_del (&found->list);
    mg_slice_delete (win_info_t, found);

    window_templates[level].nr_created--;
    return 0;
}

static int free_all_windows_in_level (int level)
{
    int nr = 0;
    struct list_head *info, *tmp;

    list_for_each_safe (info, tmp, &window_templates[level].list_wins) {
        win_info_t *win_info = (win_info_t *)info;
        list_del (&win_info->list);
        mg_slice_delete (win_info_t, win_info);
        nr++;
    }

    window_templates[level].nr_created = 0;
    return nr;
}

typedef struct test_info {
    HWND main_main_wnd; // the main window in the main thread
    HWND root_wnd;      // the root window in a GUI thread

    int win_level;      // window level for this thread.
    int nr_thread_wins; // the number of main windows in this thread.
    int nr_threads;     // the number of active GUI threads.
    int nr_zops;        // the number of pending zorder operations.
} test_info_t;

static void on_test_win_created (test_info_t* info, win_info_t* win_info)
{
    assert (IS_WIN_LEVEL_VALID (win_info->level_got));

    _MG_PRINTF ("A main window created (%s) in level (%d), visible (%s : %s)\n",
            GetWindowCaption (win_info->hwnd), win_info->level_got,
            win_info->visible?"YSE":"NO",
            IsWindowVisible (win_info->hwnd)?"YES":"NO");

    if (win_info->visible)
        info->nr_zops++;

    add_new_window_in_level (win_info->level_got, win_info);
}

static void on_test_win_shown (int show_cmd, HWND hwnd)
{
    int level = (int)GetWindowAdditionalData2 (hwnd);

    assert (IS_WIN_LEVEL_VALID (level));

    switch (show_cmd) {
        case SW_HIDE:
            _MG_PRINTF ("The main window (%s) was hidden in level (%d)\n",
                    GetWindowCaption (hwnd), level);
            mark_window_as_visible_in_level (level, hwnd, FALSE);
            break;

        case SW_SHOW:
            _MG_PRINTF ("The main window (%s) was shown in level (%d)\n",
                    GetWindowCaption (hwnd), level);
            mark_window_as_visible_in_level (level, hwnd, TRUE);
            break;

        case SW_SHOWNORMAL:
            _MG_PRINTF ("The main window (%s) was raised in level (%d)\n",
                    GetWindowCaption (hwnd), level);
            raise_window_to_top_in_level (level, hwnd);
            break;

        default:
            assert (0);
            break;
    }
}

static void on_test_win_moved (HWND hwnd)
{
    int level = (int)GetWindowAdditionalData2 (hwnd);

    assert (IS_WIN_LEVEL_VALID (level));

    _MG_PRINTF ("The main window (%s) was moved in level (%d)\n",
            GetWindowCaption (hwnd), level);
    if (change_window_rect_in_level (level, hwnd)) {
        assert (0);
    }
}

static void on_test_win_destroyed (test_info_t* info, HWND hwnd)
{
    int level = (int)GetWindowAdditionalData2 (hwnd);

    assert (IS_WIN_LEVEL_VALID (level));

    _MG_PRINTF ("The main window (%s) is being destroyed in level (%d)\n",
            GetWindowCaption (hwnd), level);

    remove_window_in_level (level, hwnd);
}

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

static void set_new_window_rect (win_info_t *win_info, int level)
{
    win_info->rc_exp = win_info->rc_window;

    int offx = window_templates[level].size_delta.cx * (random () % 5 - 10);
    int offy = window_templates[level].size_delta.cy * (random () % 5 - 10);

    OffsetRect (&win_info->rc_exp, offx, offy);
}

static void do_zorder_operation (test_info_t* info)
{
    int level;
    int wnd_idx;
    win_info_t *win_info;

    level = random() % (WIN_LEVEL_MAX + 1);
    if (window_templates[level].nr_created == 0) {
        _WRN_PRINTF ("all windows in level (%d) are destroyed.\n", level);
        for (int i = WIN_LEVEL_MIN; i <= WIN_LEVEL_MAX; i++) {
            if (window_templates[i].nr_created > 0) {
                level = i;
                break;
            }
        }
    }

    if (window_templates[level].nr_created == 0) {
        _WRN_PRINTF ("all windows in all levels are destroyed.\n");
        return;
    }

    wnd_idx = random() % window_templates[level].nr_created;

    win_info = get_win_info_by_idx (&window_templates[level].list_wins, wnd_idx);
    if (win_info) {
        switch (random () % 4) {    /* 3 for no moving */
        case 0:
            if (win_info->hwnd != info->main_main_wnd) {
                _MG_PRINTF ("we are destroying window (%s) in level (%d), "
                        "%d windows left\n",
                        GetWindowCaption (win_info->hwnd), level,
                        window_templates[level].nr_created);
                // destroy the target window
                SendNotifyMessage (win_info->hwnd, MSG_ZORDER_OP_DESTROY, 0, 0);
                info->nr_zops++;
            }
            break;

        case 1:
            if (win_info->visible) {
                _MG_PRINTF ("we are hiding window (%s) in level (%d)\n",
                        GetWindowCaption (win_info->hwnd), level);
                assert (IsWindowVisible (win_info->hwnd));
                SendNotifyMessage (win_info->hwnd, MSG_ZORDER_OP_HIDE, 0, 0);
                info->nr_zops++;
            }
            else {
                _MG_PRINTF ("we are showing window (%s) in level (%d)\n",
                        GetWindowCaption (win_info->hwnd), level);
                assert (!IsWindowVisible (win_info->hwnd));
                SendNotifyMessage (win_info->hwnd, MSG_ZORDER_OP_SHOW, 0, 0);
                info->nr_zops++;
            }
            break;

        case 2:
            _MG_PRINTF ("we are showing and raising window (%s) in level (%d)\n",
                    GetWindowCaption (win_info->hwnd), level);
            SendNotifyMessage (win_info->hwnd, MSG_ZORDER_OP_RAISE, 0, 0);
            info->nr_zops++;
            break;

        case 3:
            _MG_PRINTF ("we are moving window (%s) in level (%d)\n",
                    GetWindowCaption (win_info->hwnd), level);
            set_new_window_rect (win_info, level);
            SendNotifyMessage (win_info->hwnd, MSG_ZORDER_OP_MOVE,
                    MAKELONG (win_info->rc_exp.left, win_info->rc_exp.top),
                    MAKELONG (win_info->rc_exp.right, win_info->rc_exp.bottom));
            info->nr_zops++;
            break;

        default:
            assert (0);
            break;
        }
    }
}

static LRESULT
test_main_win_proc (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case MSG_CREATE:
        return 0;

    case MSG_ERASEBKGND:
        _DBG_PRINTF ("window (%s) got MSG_ERASEBKGND message\n",
                GetWindowCaption (hwnd));
        break;

    case MSG_SHOWWINDOW: {
        test_info_t *info;
        info = (test_info_t *)GetWindowAdditionalData (hwnd);

        assert (info && info->main_main_wnd != HWND_NULL);

        SendMessage (info->main_main_wnd,
                MSG_TESTWINSHOWN, wparam, (LPARAM)hwnd);
        break;
    }

    case MSG_MOVEWINDOW: {
        test_info_t *info;
        info = (test_info_t *)GetWindowAdditionalData (hwnd);

        assert (info && info->main_main_wnd != HWND_NULL);

        SendMessage (info->main_main_wnd,
                MSG_TESTWINMOVED, 0, (LPARAM)hwnd);
        break;
    }

    case MSG_GETWINLEVEL: {
        static int win_level = WIN_LEVEL_MIN;

        if (win_level > WIN_LEVEL_MAX) {
            win_level = WIN_LEVEL_MIN;
        }

        return win_level++;
    }

    case MSG_MTH_READY: {
        /* when a GUI thread is ready */
        test_info_t *info;
        info = (test_info_t *)GetWindowAdditionalData (hwnd);

        assert (info && hwnd == info->main_main_wnd);
        info->nr_threads++;
        break;
    }

    case MSG_MTH_QUIT: {
        /* when a GUI thread is quiting */
        test_info_t *info;
        info = (test_info_t *)GetWindowAdditionalData (hwnd);

        assert (info && hwnd == info->main_main_wnd);
        info->nr_threads--;
        break;
    }

    case MSG_TESTWINCREATED: {
        test_info_t *info;
        info = (test_info_t *)GetWindowAdditionalData (hwnd);

        assert (info && hwnd == info->main_main_wnd);
        on_test_win_created (info, (win_info_t *)lparam);
        break;
    }

    case MSG_TESTWINSHOWN: {
        test_info_t *info;
        info = (test_info_t *)GetWindowAdditionalData (hwnd);

        assert (info && hwnd == info->main_main_wnd);
        on_test_win_shown ((int)wparam, (HWND)lparam);
        info->nr_zops--;
        break;
    }

    case MSG_TESTWINMOVED: {
        test_info_t *info;
        info = (test_info_t *)GetWindowAdditionalData (hwnd);

        assert (info && hwnd == info->main_main_wnd);
        on_test_win_moved ((HWND)lparam);
        info->nr_zops--;
        break;
    }

    case MSG_TESTWINDESTROYED: {
        test_info_t *info;
        info = (test_info_t *)GetWindowAdditionalData (hwnd);

        assert (info && hwnd == info->main_main_wnd);
        on_test_win_destroyed ((test_info_t *)wparam, (HWND)lparam);
        info->nr_zops--;
        break;
    }

    case MSG_ZORDER_OP_DESTROY: {
        test_info_t *info;
        info = (test_info_t *)GetWindowAdditionalData (hwnd);

        if (info->root_wnd != hwnd) {
            SendMessage (info->main_main_wnd, MSG_TESTWINDESTROYED,
                    (WPARAM)info, (LPARAM)hwnd);
            // for non root window, destroy it here
            DestroyMainWindow (hwnd);
            MainWindowCleanup (hwnd);
        }
        break;
    }

    case MSG_ZORDER_OP_SHOW:
        ShowWindow (hwnd, SW_SHOW);
        break;

    case MSG_ZORDER_OP_HIDE:
        ShowWindow (hwnd, SW_HIDE);
        break;

    case MSG_ZORDER_OP_RAISE:
        ShowWindow (hwnd, SW_SHOWNORMAL);
        break;

    case MSG_ZORDER_OP_MOVE: {
        int lx = LOSWORD (wparam);
        int ty = HISWORD (wparam);
        int rx = LOSWORD (lparam);
        int by = HISWORD (lparam);

        MoveWindow (hwnd, lx, ty, (rx - lx), (by - ty), TRUE);
        break;
    }

    case MSG_TIMER: {
        test_info_t *info;
        info = (test_info_t *)GetWindowAdditionalData (hwnd);

        if (info->main_main_wnd == hwnd) {
            if (info->nr_threads == 0 && info->nr_thread_wins == 0) {
                _MG_PRINTF ("It's time to quit message loop of main thread\n");
                PostQuitMessage (hwnd);
            }
            else {
                // check zorder when the main main window is idle
                // and the number of pending zorder operation is 0.
                if (info->nr_zops == 0) {
                    check_zorder_by_apis ();
                    check_zorder_by_pixels ();
                    do_zorder_operation (info);
                }
            }
        }

        break;
    }

    case MSG_DESTROY: {
        test_info_t *info;
        info = (test_info_t *)GetWindowAdditionalData (hwnd);

        info->nr_thread_wins--;
        if (info->nr_thread_wins == 0)
            PostQuitMessage (hwnd);
        return 0;
    }

    default:
        break;
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

    /* always create main windows visible initially, in order that
       the z-order maintained by this test program and
       the z-order maintained by MiniGUI is identical at the start. */
    if (1 || random() % 2) {
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

        win_info.level_got = -1;
        switch (GetWindowExStyle (win_info.hwnd) & WS_EX_WINTYPE_MASK) {
            case WS_EX_WINTYPE_TOOLTIP:
                win_info.level_got = WIN_LEVEL_TOOLTIP;
                break;

            case WS_EX_WINTYPE_GLOBAL:
                win_info.level_got = WIN_LEVEL_GLOBAL;
                break;

            case WS_EX_WINTYPE_SCREENLOCK:
                win_info.level_got = WIN_LEVEL_SCREENLOCK;
                break;

            case WS_EX_WINTYPE_DOCKER:
                win_info.level_got = WIN_LEVEL_DOCKER;
                break;

            case WS_EX_WINTYPE_HIGHER:
                win_info.level_got = WIN_LEVEL_HIGHER;
                break;

            case WS_EX_WINTYPE_NORMAL:
                win_info.level_got = WIN_LEVEL_NORMAL;
                break;

            case WS_EX_WINTYPE_LAUNCHER:
                win_info.level_got = WIN_LEVEL_LAUNCHER;
                break;

            default:
                _WRN_PRINTF ("bad window type\n");
                assert (0);
                return HWND_INVALID;
        }

        // we use dwAddData2 to record the level got
        SetWindowAdditionalData2 (win_info.hwnd, (DWORD)win_info.level_got);

        _MG_PRINTF ("A main window created (%s) type (%s) visible (%s)\n",
                GetWindowCaption (win_info.hwnd),
                window_templates[win_info.level_got].type_name,
                win_info.visible?"YES":"NO");

        if (win_info.level_got != win_info.level_expected) {
            _WRN_PRINTF ("window (%s) type changed (%s -> %s)\n",
                    GetWindowCaption (win_info.hwnd),
                    window_templates[win_info.level_expected].type_name,
                    window_templates[win_info.level_got].type_name);
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

    if (!IS_WIN_LEVEL_VALID (info.win_level)) {
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

    SendMessage (info.main_main_wnd, MSG_MTH_READY, 0, (LPARAM)&self);

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

    SendMessage (info.main_main_wnd, MSG_MTH_QUIT, 0, (LPARAM)&self);
    return NULL;
}

#endif /* defined _MGRM_THREADS */

static int test_main_entry (void)
{
    test_info_t info = { HWND_NULL, HWND_NULL, WIN_LEVEL_NORMAL, 0 };

    /* initialize window list for all levels */
    for (int level = WIN_LEVEL_MIN; level <= WIN_LEVEL_MAX; level++) {
        window_templates[level].list_wins.next =
            &window_templates[level].list_wins;

        window_templates[level].list_wins.prev =
            &window_templates[level].list_wins;
#if 0
        window_templates[info.win_level].windows =
            calloc (window_templates[info.win_level].nr_allowed,
                    sizeof (win_info_t));
#endif
    }

    /* we use dwAddData to record the global test info */
    if (create_test_main_window (&info, HWND_NULL, 0) == HWND_INVALID) {
        _ERR_PRINTF ("FAILED to create the main window in main thread\n");
        return -1;
    }

    SetNotificationCallback (info.main_main_wnd, main_main_wnd_notif_proc);
    SetTimer (info.main_main_wnd, 100, 10);  // we check zorder per 100ms

    /* reset nr_thread_wins for main thread */
    info.nr_thread_wins = 0;

#ifdef _MGRM_THREADS
    /* create message threads */
    for (int level = WIN_LEVEL_MIN; level <= WIN_LEVEL_MAX; level++) {
        pthread_t th;
        if (CreateThreadForMainWindow (&th, NULL, test_thread_entry,
                    (void*)info.main_main_wnd)) {
            _ERR_PRINTF ("FAILED to create gui thread: %d\n", level);
            return -1;
        }
    }
#else   /* defined _MGRM_THREADS */
    for (int level = WIN_LEVEL_MIN; level <= WIN_LEVEL_MAX; level++) {
        info.win_level = level;

        int nr_tries = window_templates[info.win_level].nr_allowed + 1;
        for (int i = 0; i < nr_tries; i++) {
            if (create_test_main_window (&info, info.main_main_wnd, i)
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
    info.nr_zops++;
    while (GetMessage (&msg, info.main_main_wnd)) {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }

    DestroyMainWindow (info.main_main_wnd);
    MainWindowCleanup (info.main_main_wnd);

    /* free window list for all levels */
    int nr_left = 0;
    for (int level = WIN_LEVEL_MIN; level <= WIN_LEVEL_MAX; level++) {
        nr_left += free_all_windows_in_level (level);

#if 0
        free (window_templates[level].windows);
        window_templates[level].windows = NULL;
#endif
    }

    if (nr_left > 1) {
        _WRN_PRINTF ("There are some window information not freed "
                "besides the main main window\n");
    }
    assert (nr_left == 1);

    return 0;
}

int MiniGUIMain (int argc, const char* argv[])
{
    int nr_loops = 3;

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

