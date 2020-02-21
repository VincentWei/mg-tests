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
**  Test code for timer for MiniGUI 5.0.0
**  The following APIs are covered:
**
**      CreateMainWindow
**      DefaultWindowProc
**      SetTimerEx
**      KillTimer
**      SetWindowAddtionalData
**      GetWindowAdditionalData
**      PostQuitMessage
**      DestroyMainWindow
**      MainWindowCleanup
**      MSG_TIMER
**      MSG_IDLE
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

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

struct _timer_info {
    LINT id;
    DWORD interv;
    TIMERPROC timer_proc;
    DWORD ticks_expected;
    DWORD ticks_error;
    int wait_for_times;
    int fired_times;
};

struct _test_info {
    int nr_timers;
    struct _timer_info timers [NR_BITS_DWORD + 8];
};

static BOOL my_timer_proc (HWND hwnd, LINT id, DWORD ticks)
{
    struct _test_info *info =
        (struct _test_info *)GetWindowAdditionalData (hwnd);
    struct _timer_info* timer = (struct _timer_info*)id;
    DWORD ticks_error = 0;
    DWORD ticks_current = GetTickCount ();

    if (ticks_current >= timer->ticks_expected)
        ticks_error = ticks_current - timer->ticks_expected;
    else {
        ticks_error = timer->ticks_expected - ticks_current;
    }

    timer->ticks_expected = ticks_current + timer->interv;
    if (ticks_error > timer->ticks_error) {
        timer->ticks_error = ticks_error;
    }
    else {
        assert (0);
    }

    _MG_PRINTF ("Timer fired: id (%ld), interval (%lu), error (%lu)\n",
            timer->id, timer->interv, ticks_error);

    timer->fired_times++;
    timer->wait_for_times--;
    if (timer->wait_for_times == 0) {

        info->nr_timers--;
        return FALSE;
    }

    return TRUE;
}

static BOOL init_timers (HWND hwnd)
{
    struct _test_info* info;

    info = calloc (1, sizeof (*info));
    if (info == NULL)
        return FALSE;

    srandom (time(NULL));
    for (int i = 0; i < TABLESIZE (info->timers); i++) {
        info->timers[i].id = (LINT)(&info->timers[i]);
        info->timers[i].interv = random() % 6000;   // max interval: 60s
        if (info->timers[i].interv == 0)
            info->timers[i].interv = 1;

        switch (info->timers[i].interv % 2) {
            case 1:
                info->timers[i].timer_proc = my_timer_proc;
                break;
            case 0:
            default:
                info->timers[i].timer_proc = NULL;
                break;
        }

        info->timers[i].wait_for_times = random() % 10;   // max times: 9
        if (info->timers[i].wait_for_times == 0)
            info->timers[i].wait_for_times = 1;

        if (SetTimerEx (hwnd, info->timers[i].id,
                info->timers[i].interv, info->timers[i].timer_proc)) {
            DWORD ticks_current = GetTickCount ();

            info->timers[i].ticks_expected = ticks_current +
                info->timers[i].interv;
            _WRN_PRINTF ("ticks_expected for timer #%d: %lu\n",
                    i, info->timers[i].ticks_expected);

            info->nr_timers++;
        }
        else {
            info->timers[i].id = 0;
            assert (i >= NR_BITS_DWORD);
        }
    }

    SetWindowAdditionalData (hwnd, (DWORD)info);
    return (info->nr_timers == NR_BITS_DWORD);
}

static void dump_timer_info (HWND hwnd)
{
    struct _test_info *info =
        (struct _test_info *)GetWindowAdditionalData (hwnd);

    if (info->nr_timers > 0) {
        _MG_PRINTF ("Installed timers of window (%s):\n", GetWindowCaption (hwnd));

        for (int i = 0; i < TABLESIZE (info->timers); i++) {
            if (info->timers[i].id) {
                _MG_PRINTF ("Timer #%d: id (%ld), interval (%lu), ticks_expected (%lu), wait_for_times (%d)\n",
                        i, info->timers[i].id, info->timers[i].interv,
                        info->timers[i].ticks_expected,
                        info->timers[i].wait_for_times);
            }
            else {
                _MG_PRINTF ("Timer #%d: not installed\n", i);
            }
        }
    }
    else {
        _MG_PRINTF ("Killed timers of window (%s):\n", GetWindowCaption (hwnd));

        for (int i = 0; i < TABLESIZE (info->timers); i++) {
            if (info->timers[i].id) {
                assert (info->timers[i].fired_times > 0);
                assert (info->timers[i].wait_for_times == 0);
                _MG_PRINTF ("Timer #%d: id (%ld), interval (%lu), max error (%lu), "
                        "fired times (%d)\n",
                        i, info->timers[i].id, info->timers[i].interv,
                        info->timers[i].ticks_error, info->timers[i].fired_times);
            }
            else {
                _MG_PRINTF ("Timer #%d: not installed\n", i);
            }
        }
    }
}

static void check_timers (HWND hwnd)
{
    struct _test_info *info =
        (struct _test_info *)GetWindowAdditionalData (hwnd);

    _WRN_PRINTF("The number of timers active currently (%lu): %d\n",
            GetTickCount (), info->nr_timers);

    if (info->nr_timers == 0)
        PostQuitMessage (hwnd);
}

static void clean_timers (HWND hwnd)
{
    struct _test_info *info =
        (struct _test_info *)GetWindowAdditionalData (hwnd);

    assert (info);
    free (info);

    /* all timers will be killed by DestroyMainWindow() */
}

static LRESULT
TestWinProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        if (init_timers (hWnd))
            return 0;
        return -1;

    case MSG_IDLE:
        check_timers (hWnd);
        break;

    case MSG_TIMER:
        if (!my_timer_proc (hWnd, (LINT)wParam, (DWORD)lParam)) {
            KillTimer (hWnd, (LINT)wParam);
        }
        break;
        
    case MSG_DESTROY:
        clean_timers (hWnd);
        return 0;
    }

    return DefaultWindowProc (hWnd, message, wParam, lParam);
}

static int test_timer_in_gui_thread (void)
{
    MSG Msg;
    HWND hMainWnd;
    MAINWINCREATE CreateInfo;

    CreateInfo.dwStyle = 
        WS_VISIBLE | WS_BORDER | WS_CAPTION;
    CreateInfo.dwExStyle = WS_EX_NONE;
    CreateInfo.spCaption = "timer tester";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = TestWinProc;
    CreateInfo.lx = 0;
    CreateInfo.ty = 0;
    CreateInfo.rx = 100;
    CreateInfo.by = 100;
    CreateInfo.iBkColor = PIXEL_lightwhite;
    CreateInfo.dwAddData = 0;
    CreateInfo.hHosting = HWND_DESKTOP;

    hMainWnd = CreateMainWindow (&CreateInfo);
    if (hMainWnd == HWND_INVALID)
        return -1;

    dump_timer_info (hMainWnd);

    ShowWindow (hMainWnd, SW_SHOWNORMAL);

    while (GetMessage(&Msg, hMainWnd)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    dump_timer_info (hMainWnd);

    DestroyMainWindow (hMainWnd);
    MainWindowCleanup (hMainWnd);
    return 0;
}

int MiniGUIMain (int argc, const char* argv[])
{
    int retval;

    JoinLayer (NAME_DEF_LAYER , "timer" , 0 , 0);

    for (int i = 0; i < 10; i++) {
        retval = test_timer_in_gui_thread ();
        if (retval)
            exit (retval);
    }

    return 0;
}

