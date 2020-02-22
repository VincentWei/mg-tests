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
**      IsTimerInstalled
**      HaveFreeTimer
**      SetWindowAdditionalData
**      GetWindowAdditionalData
**      SetWindowAdditionalData2
**      GetWindowAdditionalData2
**      PostQuitMessage
**      DestroyMainWindow
**      GetWindowInfo
**      MainWindowCleanup
**      CreateVirtualWindow
**      DestroyVirtualWindow
**      VirtualWindowCleanup
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

    if (ticks_error > 5) {
        _WRN_PRINTF ("Too large error: %lu\n", ticks_error);
    }

    timer->ticks_expected = ticks_current + timer->interv;
    if (ticks_error > timer->ticks_error) {
        timer->ticks_error = ticks_error;
    }

    _MG_PRINTF ("Timer fired for window (%s): id (%ld), interval (%lu), "
            "error (%lu)\n",
            GetWindowCaption (hwnd), timer->id, timer->interv, ticks_error);

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
    int nr_timers = 0;
    struct _test_info* info;

    info = calloc (1, sizeof (*info));
    if (info == NULL)
        return FALSE;

    for (int i = 0; i < TABLESIZE (info->timers); i++) {
        info->timers[i].id = (LINT)(&info->timers[i]);
        info->timers[i].interv = random() % 3000;   // max interval: 60s
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

        info->timers[i].wait_for_times = random() % 5;   // max times: 9
        if (info->timers[i].wait_for_times == 0)
            info->timers[i].wait_for_times = 1;

        if (SetTimerEx (hwnd, info->timers[i].id,
                info->timers[i].interv, info->timers[i].timer_proc)) {
            DWORD ticks_current = GetTickCount ();

            info->timers[i].ticks_expected = ticks_current +
                info->timers[i].interv;

            info->nr_timers++;
        }
        else {
            info->timers[i].id = 0;
            assert (i >= NR_BITS_DWORD);
        }
    }

    assert (!HaveFreeTimer());

    for (int i = 0; i < TABLESIZE (info->timers); i++) {
        if (info->timers[i].id) {
            if (IsTimerInstalled (hwnd, info->timers[i].id))
                nr_timers++;
        }
    }

    assert (nr_timers == info->nr_timers);

    SetWindowAdditionalData (hwnd, (DWORD)info);
    return (info->nr_timers == NR_BITS_DWORD);
}

static int dump_timer_info (HWND hwnd)
{
    struct _test_info *info =
        (struct _test_info *)GetWindowAdditionalData (hwnd);

    if (info->nr_timers > 0) {
        _MG_PRINTF ("Installed timers of window (%s):\n",
                GetWindowCaption (hwnd));

        for (int i = 0; i < TABLESIZE (info->timers); i++) {
            if (info->timers[i].id) {
                _MG_PRINTF ("Timer #%d: id (%ld), interval (%lu), "
                        "ticks_expected (%lu), wait_for_times (%d)\n",
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

    return info->nr_timers;
}

static void check_timers (HWND hwnd, DWORD ticks)
{
    int nr_timers = 0;

    struct _test_info *info =
        (struct _test_info *)GetWindowAdditionalData (hwnd);

    _MG_PRINTF("The number of active timers for window (%s): %d, at ticks (%lu)\n",
            GetWindowCaption (hwnd), info->nr_timers, ticks);

    for (int i = 0; i < TABLESIZE (info->timers); i++) {
        if (info->timers[i].id) {
            if (IsTimerInstalled (hwnd, info->timers[i].id))
                nr_timers++;
        }
    }

    assert (nr_timers == info->nr_timers);

    if (info->nr_timers == 0 &&
            // we use dwAddData2 to check whether the message thread
            // for the virtual window has exited.
            GetWindowAdditionalData2 (hwnd) == (DWORD)-1) {
        PostQuitMessage (hwnd);
    }
}

static void clean_timers (HWND hwnd)
{
    struct _test_info *info =
        (struct _test_info *)GetWindowAdditionalData (hwnd);

    assert (info);
    free (info);

    _MG_PRINTF ("Test data of window (%s) freed\n", GetWindowCaption (hwnd));

    SetWindowAdditionalData (hwnd, 0);
    /* all timers will be killed by DestroyMainWindow() */
}

enum {
    CMD_VIRTWND_FAILED = 0,
    CMD_VIRTWND_CREATED,
    CMD_VIRTWND_DESTROYED,
    CMD_VIRTWND_QUITED,
};

static void on_command (HWND hWnd, WPARAM cmd_id, LPARAM arg)
{
    switch (cmd_id) {
        case CMD_VIRTWND_FAILED:
            _ERR_PRINTF ("Failed to create the virtual window\n");
            break;

        case CMD_VIRTWND_CREATED:
            _WRN_PRINTF ("The virtual window has been created: %p\n",
                    (void*)arg);
            SetWindowAdditionalData2 (hWnd, arg);
            break;

        case CMD_VIRTWND_DESTROYED:
            _WRN_PRINTF ("The virtual window has been destroyed: %p\n",
                    (void*)arg);
            SetWindowAdditionalData2 (hWnd, 0);
            break;

        case CMD_VIRTWND_QUITED:
            _WRN_PRINTF ("The message thread for the virtual window exited: %p\n",
                   (void*)arg);
            SetWindowAdditionalData2 (hWnd, (DWORD)-1);
            break;

        default:
            _WRN_PRINTF ("Unknown command identifier: %d\n", (int)cmd_id);
            break;
    }
}

static LRESULT
TestWinProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        if (init_timers (hWnd)) {
            SetWindowAdditionalData2 (hWnd, (DWORD)-1);
            return 0;
        }
        return -1;

    case MSG_COMMAND:
        on_command (hWnd, wParam, lParam);
        break;
        
    case MSG_IDLE:
        check_timers (hWnd, (DWORD)wParam);
        break;

    case MSG_TIMER:
        if (!my_timer_proc (hWnd, (LINT)wParam, (DWORD)lParam)) {
            KillTimer (hWnd, (LINT)wParam);
            assert (HaveFreeTimer ());
        }
        break;
        
    case MSG_DESTROY:
        clean_timers (hWnd);
        return 0;
    }

    return DefaultWindowProc (hWnd, message, wParam, lParam);
}

#ifdef _MGHAVE_VIRTUAL_WINDOW
static void* test_timer_in_message_thread (void* arg)
{
    MSG Msg;
    HWND hMainWnd = (HWND)arg;
    HWND hVirtWnd = CreateVirtualWindow (HWND_DESKTOP,
            "Virtual Window for timer test", 0, TestWinProc, 0);

    if (hVirtWnd == HWND_INVALID) {
        SendNotifyMessage (hMainWnd, MSG_COMMAND, CMD_VIRTWND_FAILED, 0);
        return NULL;
    }

    SendNotifyMessage (hMainWnd, MSG_COMMAND, CMD_VIRTWND_CREATED,
            (LPARAM)hVirtWnd);

    dump_timer_info (hVirtWnd);

    while (GetMessage (&Msg, hVirtWnd)) {
        DispatchMessage (&Msg);
    }

    if (dump_timer_info (hVirtWnd) > 0) {
        _ERR_PRINTF ("The message loop quited unexpectedly\n");
    }

    DestroyVirtualWindow (hVirtWnd);

    SendNotifyMessage (hMainWnd, MSG_COMMAND, CMD_VIRTWND_DESTROYED,
            (LPARAM)hVirtWnd);

    VirtualWindowCleanup (hVirtWnd);

    SendNotifyMessage (hMainWnd, MSG_COMMAND, CMD_VIRTWND_QUITED, 0);
    return NULL;
}
#endif  /* defined _MGHAVE_VIRTUAL_WINDOW */

static int test_timer_in_gui_thread (void)
{
    MSG Msg;
    HWND hMainWnd;
    MAINWINCREATE CreateInfo;

    CreateInfo.dwStyle = 
        WS_VISIBLE | WS_BORDER | WS_CAPTION;
    CreateInfo.dwExStyle = WS_EX_NONE;
    CreateInfo.spCaption = "Main window for timer test";
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

#ifdef _MGHAVE_VIRTUAL_WINDOW
    pthread_t th;

    if (CreateThreadForMessaging (&th, NULL, test_timer_in_message_thread,
            (void*)hMainWnd, TRUE, 0)) {

        _ERR_PRINTF ("Failed to create message thread\n");
        return -1;
    }
#endif  /* defined _MGHAVE_VIRTUAL_WINDOW */

    dump_timer_info (hMainWnd);

    ShowWindow (hMainWnd, SW_SHOWNORMAL);
    while (GetMessage (&Msg, hMainWnd)) {
        TranslateMessage (&Msg);
        DispatchMessage (&Msg);
    }

    if (dump_timer_info (hMainWnd) > 0) {
        _ERR_PRINTF ("The message loop quited unexpectedly\n");
    }

    DestroyMainWindow (hMainWnd);

    /* The dwAddData of this window has been set to zero in clean_timers.
       We use GetWindowInfo to check the dwAddData of this window,
       because the window is to be deleted, GetWindowAdditionalData will
       always return 0. */
    const WINDOWINFO *win_info = GetWindowInfo (hMainWnd);
    assert (win_info->dwAddData == 0);

    MainWindowCleanup (hMainWnd);
    return 0;
}

int MiniGUIMain (int argc, const char* argv[])
{
    int retval;

    JoinLayer (NAME_DEF_LAYER , "timer" , 0 , 0);

    srandom (time(NULL));

    for (int i = 0; i < 3; i++) {
        retval = test_timer_in_gui_thread ();
        if (retval)
            exit (retval);
    }

    _MG_PRINTF ("Test for timer passed!\n");
    return 0;
}

