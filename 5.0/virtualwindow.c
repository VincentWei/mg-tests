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
**  Test code of virtual window for MiniGUI 5.0.
**
**  This test program creates some main windows and virtual windows in the main
**  thread, and creates some virtual windows in some message threads.
**  Every main window or virtual window will create a general POSIX thread
**  as a worker to perform a time-consuming operation.
**
**  All threads use MiniGUI's messaging functions to send notifications by
**  calling NotifyWindow or send a synchronous message to other windows/threads
**  by calling SendMessage.
**
**  The following APIs are covered:
**
**      CreateThreadForMessaging
**      CreateMainWindowEx2
**      DefaultWindowProc
**      DestroyMainWindow
**      MainWindowCleanup
**      CreateVirtualWindow
**      DestroyVirtualWindow
**      VirtualWindowCleanup
**      GetRootWindow
**      GetFirstHosted
**      GetNextHosted
**      GetHostedById
**      GetWindowInfo
**      PostQuitMessage
**      NotifyWindow
**      SendMessage
**      SetNotificationCallback
**      GetWindowId
**      SetWindowId
**      GetWindowAdditionalData
**      SetWindowAdditionalData
**      GetWindowAdditionalData2
**      SetWindowAdditionalData2
**      MSG_IDLE
**      MSG_CREATE
**      MSG_DESTROY
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

#ifdef _MGHAVE_VIRTUAL_WINDOW

#define MAX_DEPTH_HOSTED        8
#define MAX_BREADTH_HOSTED      8

enum {
    NC_GTH_NOOP = 0,
    NC_GTH_RUNNING,
    NC_GTH_QUITING,
    NC_MTH_RUNNING,
    NC_MTH_ROOT_CEATED,
    NC_MTH_MAINWIN_CEATED,
    NC_MTH_VIRTWIN_CEATED,
    NC_MTH_QUITING,
};

/* these message will be sent by a general thread to
   it's owner synchronously. */
#define MSG_GTH_DONE    (MSG_USER + 1)
#define MSG_GTH_QUIT    (MSG_USER + 2)

/* these message will be sent by a message thread to
   the main window in the main thread synchronously. */
#define MSG_MTH_READY   (MSG_USER + 3)
#define MSG_MTH_QUIT    (MSG_USER + 4)

static void* general_entry (void* arg)
{
    char buff [256];
    HWND owner = (HWND)arg;
    pthread_t self = pthread_self ();
    pthread_detach (self);

    NotifyWindow (owner, (LINT)owner, NC_GTH_RUNNING, (DWORD)&self);

    sleep (random() % 10);

    snprintf (buff, sizeof (buff),
            "Payload from general thread (%p): my owner is %s",
            &self, GetWindowCaption (owner));

    char* payload = strdup (buff);
    SendMessage (owner, MSG_GTH_DONE, (WPARAM)payload, (LPARAM)&self);
    free (payload);

    NotifyWindow (owner, (LINT)owner, NC_GTH_QUITING, (DWORD)&self);

    SendMessage (owner, MSG_GTH_QUIT, (WPARAM)0, (LPARAM)&self);
    return NULL;
}

/* create a general threads */
static int start_general_thread (HWND owner)
{
    pthread_t th;
    if (pthread_create (&th, NULL, general_entry, (void*)owner)) {
        _ERR_PRINTF ("Failed to create the general thread\n");
        return -1;
    }

    return 0;
}

struct test_info {
    HWND main_main_wnd;
    HWND root_wnd;

    int max_depth_hosted;
    int max_breadth_hosted;

    int nr_main_wins;
    int nr_virt_wins;

    int nr_thread_wins;

    int nr_mths;
};

static HWND create_test_main_window (struct test_info* info, HWND hosting);
static HWND create_test_virtual_window (struct test_info* info, HWND hosting);

static void create_hosted_windows (HWND hwnd)
{
    struct test_info *info = (struct test_info*)GetWindowAdditionalData (hwnd);
    DWORD add_data2 = GetWindowAdditionalData2 (hwnd);
    WORD depth_hosted = HIWORD (add_data2);
    pthread_t self = pthread_self ();

    _DBG_PRINTF ("current hosting depth: %d\n", depth_hosted);
    if (depth_hosted > info->max_depth_hosted) {
        _WRN_PRINTF ("Reached the maximal hosting depth: %d\n", depth_hosted);
        return;
    }

    int max_hosted = info->max_breadth_hosted;
    if (info->max_breadth_hosted > 0)
        max_hosted = random() % info->max_breadth_hosted;
    else
        max_hosted = info->max_breadth_hosted;

    if (max_hosted <= 0) max_hosted = 1;

    for (int i = 0; i < max_hosted; i++) {
        HWND hosted_wnd;
#ifdef _MGRM_THREADS
        if ((random() % 2)) {
            hosted_wnd = create_test_main_window (info, hwnd);
        }
        else {
            hosted_wnd = create_test_virtual_window (info, hwnd);
        }
#else   /* defined _MGRM_THREADS */
        hosted_wnd = create_test_main_window (info, hwnd);
        if (GetRootWindow() != HWND_DESKTOP) {
            /* for non thread modes, hosted main window must be invalid */
            assert (hosted_wnd == HWND_INVALID);
        }

        if (hosted_wnd == HWND_INVALID) {
            hosted_wnd = create_test_virtual_window (info, hwnd);
        }
#endif  /* not defined _MGRM_THREADS */

        if (hosted_wnd != HWND_INVALID) {
            char spaces [256];
            spaces [0] = '\0';

            for (int i = 0; i < depth_hosted; i++) {
                strcat (spaces, "   ");
            }

            if (IsMainWindow (hosted_wnd)) {
                _MG_PRINTF ("%sCREATED A MAIN WINDOW at depth (%d) in thread (%p): %p\n",
                        spaces, depth_hosted, &self, hosted_wnd);
            }
            else if (IsVirtualWindow (hosted_wnd)) {
                _MG_PRINTF ("%sCREATED A VIRT WINDOW at depth (%d in thread (%p)): %p\n",
                        spaces, depth_hosted, &self, hosted_wnd);
            }
            else {
                 assert (0);
            }
        }
        else {
            _ERR_PRINTF ("FAILED to create a hosted window\n");
        }
    }
}

struct _travel_context {
    int nr_wins;
    int depth;
    HWND hosting;
};

static HWND travel_win_tree_bfs (struct _travel_context *ctxt)
{
    HWND hosting = ctxt->hosting;
    HWND hosted = GetFirstHosted (hosting);

    while (hosted) {
        const char* spaces = "    ";
        const WINDOWINFO* win_info;

        for (int i = 0; i < ctxt->depth; i++) {
            _MG_PRINTF ("%s", spaces);
        }

        win_info = GetWindowInfo (hosted);
        _MG_PRINTF ("Got a window in depth (%d): %p (%ld, %s)\n",
                ctxt->depth, hosted, win_info->id, win_info->spCaption);

        ctxt->nr_wins++;

        ctxt->hosting = hosted;
        ctxt->depth++;
        travel_win_tree_bfs (ctxt);
        ctxt->depth--;

        hosted = GetNextHosted (hosting, hosted);
    }

    return NULL;
}

static void test_get_hosted_by_id (HWND root_wnd, struct test_info *info)
{
    HWND found_main = GetHostedById (root_wnd,
            info->max_depth_hosted,
            WIN_SEARCH_METHOD_BFS | WIN_SEARCH_FILTER_MAIN);

    HWND found_virt = GetHostedById (root_wnd,
            info->max_depth_hosted,
            WIN_SEARCH_METHOD_DFS | WIN_SEARCH_FILTER_VIRT);

    /* GetHostedById never returns the desktop */
    assert (found_main != HWND_DESKTOP);
    assert (found_main != HWND_DESKTOP);

    if (found_main != HWND_NULL || found_virt != HWND_NULL) {
        /* different filters must return different results */
        assert (found_main != found_virt);
    }

    if (info->nr_thread_wins > 0 && root_wnd == HWND_DESKTOP) {
        HWND found_wnd = GetHostedById (root_wnd,
                0,
                WIN_SEARCH_METHOD_DFS |
                WIN_SEARCH_FILTER_VIRT |
                WIN_SEARCH_FILTER_MAIN);

        /* the window identified by 0 is always the root window
           in the gui thread */
        assert (found_wnd == info->root_wnd);
    }
}

static void print_hosting_tree (HWND hwnd)
{
    HWND root_wnd = GetRootWindow ();

    if (root_wnd != HWND_INVALID && root_wnd != HWND_NULL) {
        struct test_info *info;
        info = (struct test_info*)GetWindowAdditionalData (hwnd);

        struct _travel_context ctxt = { 0, 0, root_wnd };
        travel_win_tree_bfs (&ctxt);
        assert (ctxt.nr_wins == info->nr_thread_wins);

        test_get_hosted_by_id (root_wnd, info);
    }
    else {
        _ERR_PRINTF ("try to travel a non message thread\n");
    }
}

static void my_notif_proc (HWND hwnd, LINT id, int nc, DWORD add_data)
{
    struct test_info *info;
    info = (struct test_info *)GetWindowAdditionalData (hwnd);

    assert (info);
    assert (id == (LINT)hwnd);

    _DBG_PRINTF ("Got a notification for window (%p, %s): nc(%d)\n",
                hwnd, GetWindowCaption (hwnd), nc);

    switch (nc) {
        case NC_GTH_RUNNING:
            create_hosted_windows (hwnd);
            break;

        case NC_GTH_QUITING:
            _MG_PRINTF ("The general thread of window (%p) is quiting\n",
                    hwnd);
            break;

        case NC_MTH_ROOT_CEATED:
            assert (info->main_main_wnd == hwnd);
            _DBG_PRINTF ("A root window for a new message thread created: %p\n",
                    (HWND)add_data);
            break;

        case NC_MTH_MAINWIN_CEATED:
            assert (info->main_main_wnd == hwnd);
            info->nr_main_wins++;
            _DBG_PRINTF ("A new main window (%p) created, total (%d)\n",
                    (HWND)add_data, info->nr_main_wins);
            print_hosting_tree (hwnd);
            break;

        case NC_MTH_VIRTWIN_CEATED:
            assert (info->main_main_wnd == hwnd);
            info->nr_virt_wins++;
            _DBG_PRINTF ("A new virtual window (%p) created, total (%d)\n",
                    (HWND)add_data, info->nr_virt_wins);
            print_hosting_tree (hwnd);
            break;

        case NC_MTH_QUITING:
            assert (info->main_main_wnd == hwnd);
            _DBG_PRINTF ("A message thread (%p) is quiting\n",
                    (void*)add_data);
            break;

        default:
            _WRN_PRINTF ("unhandled notification code: %d\n", nc);
            break;
    }
}

static int on_create (HWND hwnd)
{
    assert (GetWindowAdditionalData (hwnd));

    /* start a general thread for every window */
    if (start_general_thread (hwnd))
        return -1;

    return 0;
}

static LRESULT
test_win_proc (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case MSG_CREATE:
        SetNotificationCallback (hwnd, my_notif_proc);
        return on_create (hwnd);

    case MSG_IDLE:
        if (GetWindowId (hwnd) == -1) {
            struct test_info* info;
            info = (struct test_info*)GetWindowAdditionalData (hwnd);
            DWORD add_data2 = GetWindowAdditionalData2 (hwnd);

            if (info == NULL) {
                const WINDOWINFO* win_info = GetWindowInfo(hwnd);

                _MG_PRINTF ("A bad window (%p, %0lx): %s...\n",
                        hwnd, win_info->id, win_info->spCaption);
                break;
            }

            if (info->root_wnd != hwnd) {
                _MG_PRINTF ("It's time to destroy the HOSTED window (%p, %0lx): %s...\n",
                        hwnd, add_data2, GetWindowCaption (hwnd));

                // retry to destroy self.
                if (IsVirtualWindow (hwnd)) {
                    if (DestroyVirtualWindow (hwnd)) {
                        VirtualWindowCleanup (hwnd);
                        _WRN_PRINTF ("A HOSTED window destroyed (%p, %0lx)\n",
                                hwnd, add_data2);
                        info->nr_thread_wins--;
                    }
                }
                else if (IsMainWindow (hwnd)) {
                    if (DestroyMainWindow (hwnd)) {
                        _WRN_PRINTF ("A HOSTED window destroyed (%p, %0lx)\n",
                                hwnd, add_data2);
                        MainWindowCleanup (hwnd);
                        info->nr_thread_wins--;
                    }
                }
                else {
                    assert (0);
                }
            }
            else {
                _MG_PRINTF ("It's time to quit the message loop (%p, %0lx): nr_wins (%d)\n",
                        hwnd, add_data2, info->nr_thread_wins);

                if (info->nr_mths == 0 && info->nr_thread_wins == 0) {
                    // all hosted windows are destroyed.
                    PostQuitMessage (hwnd);
                }
            }
        }
        break;

    case MSG_GTH_DONE:
        _MG_PRINTF ("general thread (%p) done: payload (%s)\n",
                (void*)lparam, (char*)wparam);
        break;

    case MSG_GTH_QUIT:
        /* when the general thread quits, set the identifier to -1 */
        SetWindowId (hwnd, -1);
        break;

    case MSG_MTH_READY: {
        /* when a mesage thread is ready */
        struct test_info* info;
        info = (struct test_info*)GetWindowAdditionalData (hwnd);
        info->nr_mths++;
        break;
    }

    case MSG_MTH_QUIT: {
        /* when a mesage thread is quiting */
        struct test_info* info;
        info = (struct test_info*)GetWindowAdditionalData (hwnd);
        info->nr_mths--;
        break;
    }

    case MSG_DESTROY:
        if (GetWindowId (hwnd) != -1) {
            // if the identifier is not -1, do not destroy me */
            return -1;
        }
        return 0;
    }

    return DefaultWindowProc (hwnd, message, wparam, lparam);
}

static HWND
create_test_main_window (struct test_info* info, HWND hosting)
{
    MAINWINCREATE create_info;

    create_info.dwStyle = 
        WS_VISIBLE | WS_BORDER | WS_CAPTION;
    create_info.dwExStyle = WS_EX_NONE;
    create_info.spCaption = "A Main window";
    create_info.hMenu = 0;
    create_info.hCursor = GetSystemCursor(0);
    create_info.hIcon = 0;
    create_info.MainWindowProc = test_win_proc;
    create_info.lx = 0;
    create_info.ty = 0;
    create_info.rx = 100;
    create_info.by = 100;
    create_info.iBkColor = PIXEL_lightwhite;
    create_info.dwAddData = (DWORD)info;
    create_info.hHosting = hosting;

    WORD depth_hosted = 0;
    WORD nr_wins_hosted = 0;
    if (hosting != HWND_NULL) {
        DWORD add_data2;

        add_data2 = GetWindowAdditionalData2 (hosting);
        depth_hosted = HIWORD (add_data2);
        nr_wins_hosted = LOWORD (add_data2);

        nr_wins_hosted++;
        add_data2 = MAKELONG (nr_wins_hosted, depth_hosted);
        SetWindowAdditionalData2 (hosting, add_data2);

        depth_hosted++;
        nr_wins_hosted = 0;
    }

    DWORD add_data2 = MAKELONG (nr_wins_hosted, depth_hosted);

    /* we use hosting depth as the identifier of the window,
       for testing GetHostedbyId easily */
    HWND new_wnd = CreateMainWindowEx2 (&create_info, depth_hosted, NULL, NULL,
            0, 0, 0, 0);
    if (info->root_wnd && new_wnd != HWND_INVALID) {
        NotifyWindow (info->main_main_wnd, (LINT)info->main_main_wnd,
            NC_MTH_MAINWIN_CEATED, (DWORD)new_wnd);
        SetWindowAdditionalData2 (new_wnd, add_data2);
        _DBG_PRINTF ("CREATED A WINDOW (MAIN) at depth (%d): %p\n", depth_hosted--, new_wnd);

        info->nr_thread_wins++;
    }

    return new_wnd;
}

static HWND
create_test_virtual_window (struct test_info* info, HWND hosting)
{
    WORD depth_hosted = 0;
    WORD nr_wins_hosted = 0;

    if (hosting != HWND_NULL) {
        DWORD add_data2;

        add_data2 = GetWindowAdditionalData2 (hosting);
        depth_hosted = HIWORD (add_data2);
        nr_wins_hosted = LOWORD (add_data2);

        nr_wins_hosted++;
        add_data2 = MAKELONG (nr_wins_hosted, depth_hosted);
        SetWindowAdditionalData2 (hosting, add_data2);

        depth_hosted++;
        nr_wins_hosted = 0;
    }

    DWORD add_data2 = MAKELONG (nr_wins_hosted, depth_hosted);

    /* we use the depth hosted as the identifier of a window */
    HWND new_wnd = CreateVirtualWindow (hosting, test_win_proc,
            "A Virtual Window", depth_hosted, (DWORD)info);

    if (info->root_wnd && new_wnd != HWND_INVALID) {
        NotifyWindow (info->main_main_wnd, (LINT)info->main_main_wnd,
            NC_MTH_VIRTWIN_CEATED, (DWORD)new_wnd);
        SetWindowAdditionalData2 (new_wnd, add_data2);
        _DBG_PRINTF ("CREATED A WINDOW (VIRTUAL) at depth (%d): %p\n", depth_hosted--, new_wnd);

        info->nr_thread_wins++;
    }

    return new_wnd;
}

static void* test_entry (void* arg)
{
    MSG msg;
    struct test_info info = { (HWND)arg, HWND_NULL };
    pthread_t self = pthread_self ();

    info.max_depth_hosted = random() % MAX_DEPTH_HOSTED;
    info.max_breadth_hosted = random() % MAX_BREADTH_HOSTED;
    info.nr_main_wins = 0;
    info.nr_virt_wins = 0;

    _WRN_PRINTF ("MESSAGE THREAD (%p): max_depth_hosted (%d), max_breadth_hosted (%d)\n",
            &self, info.max_depth_hosted, info.max_breadth_hosted);

    NotifyWindow (info.main_main_wnd, (LINT)info.main_main_wnd,
            NC_MTH_RUNNING, (DWORD)&self);

#ifdef _MGRM_THREADS
    info.root_wnd = create_test_main_window (&info, HWND_NULL);
    if (info.root_wnd == HWND_INVALID) {
        info.root_wnd = create_test_virtual_window (&info, HWND_NULL);
    }
#else
    info.root_wnd = create_test_main_window (&info, HWND_NULL);
    assert (info.root_wnd == HWND_INVALID);

    info.root_wnd = create_test_virtual_window (&info, HWND_NULL);
#endif

    if (info.root_wnd == HWND_INVALID) {
        NotifyWindow (info.main_main_wnd, (LINT)info.main_main_wnd,
                NC_MTH_QUITING, (DWORD)&self);
        _ERR_PRINTF ("FAILED to create root window of message threads\n");
        return NULL;
    }

    SendMessage (info.main_main_wnd, MSG_MTH_READY, 0, (LPARAM)&self);

    if (IsMainWindow (info.root_wnd)) {
        _MG_PRINTF ("CREATED THE ROOT MAIN WINDOW at depth (0) in thread (%p): %p\n",
                &self, info.root_wnd);
    }
    else if (IsVirtualWindow (info.root_wnd)) {
        _MG_PRINTF ("CREATED THE ROOT VIRT WINDOW at depth (0) in thread (%p): %p\n",
                &self, info.root_wnd);
    }
    else {
         assert (0);
    }

    info.nr_thread_wins = 0;
    info.nr_mths = 0;

    /* we use the dwAddData2 to record the current hosted depth
       and the number of windows in current hosted depth. */
    SetWindowAdditionalData2 (info.root_wnd, 0);
    NotifyWindow (info.main_main_wnd, (LINT)info.main_main_wnd,
            NC_MTH_ROOT_CEATED, (DWORD)info.root_wnd);

    while (GetMessage (&msg, info.root_wnd)) {
        DispatchMessage (&msg);
    }

    _WRN_PRINTF ("MESSAGE THREAD (%p) is quiting\n", &self);

    if (IsMainWindow (info.root_wnd)) {
        DestroyMainWindow (info.root_wnd);
        MainWindowCleanup (info.root_wnd);
    }
    else if (IsVirtualWindow (info.root_wnd)) {
        DestroyVirtualWindow (info.root_wnd);
        VirtualWindowCleanup (info.root_wnd);
    }
    else {
         assert (0);
    }

    NotifyWindow (info.main_main_wnd, (LINT)info.main_main_wnd,
            NC_MTH_QUITING, (DWORD)&self);

    SendMessage (info.main_main_wnd, MSG_MTH_QUIT, 0, (LPARAM)&self);
    return NULL;
}

static int test_main_entry (int nr_threads)
{
    struct test_info info;

    /* create a main window in main thread */
    info.main_main_wnd = HWND_NULL;
    info.root_wnd = HWND_NULL;
    info.max_depth_hosted = random() % MAX_DEPTH_HOSTED;
    info.max_breadth_hosted = random() % MAX_BREADTH_HOSTED;

    _WRN_PRINTF ("MAIN THREAD: max_depth_hosted (%d), max_breadth_hosted (%d)\n",
            info.max_depth_hosted, info.max_breadth_hosted);

    /* we use dwAddData to record the global test info */
    info.main_main_wnd = create_test_main_window (&info, HWND_NULL);
    if (info.main_main_wnd == HWND_INVALID) {
        _ERR_PRINTF ("FAILED to create the main window in main thread\n");
        return -1;
    }

    info.root_wnd = info.main_main_wnd;
    info.nr_main_wins = 1;
    info.nr_virt_wins = 0;
    info.nr_thread_wins = 0;
    info.nr_mths = 0;

    /* we use dwAddData2 to record the hosted depth */
    SetWindowAdditionalData2 (info.main_main_wnd, 0);

    /* create message threads */
    for (int i = 0; i < nr_threads; i++) {
        pthread_t th;
        if (CreateThreadForMessaging (&th, NULL, test_entry,
                    (void*)info.main_main_wnd, TRUE, 16)) {
            _ERR_PRINTF ("FAILED to create message thread: %d\n", i);
            return -1;
        }
    }

    /* enter message loop */
    MSG msg;
    ShowWindow (info.main_main_wnd, SW_SHOWNORMAL);
    while (GetMessage (&msg, info.main_main_wnd)) {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }

    _WRN_PRINTF ("MAIN THREAD is quiting\n");

    DestroyMainWindow (info.main_main_wnd);
    MainWindowCleanup (info.main_main_wnd);

    _MG_PRINTF ("Test for virtual window passed!\n");
    return 0;
}

int MiniGUIMain (int argc, const char* argv[])
{
    int nr_loops = 10;
    int nr_threads = 4;

    JoinLayer (NAME_DEF_LAYER , "virtual window" , 0 , 0);

    srandom (time(NULL));

    if (argc > 1)
        nr_loops = atoi (argv[1]);
    if (nr_loops < 0)
        nr_loops = 10;

    if (argc > 2)
        nr_threads = atoi (argv[2]);
    if (nr_threads < 0)
        nr_threads = 4;

    for (int i = 0; i < nr_loops; i++) {
        _WRN_PRINTF ("Starting loop %d.\n", i);
        if (test_main_entry (nr_threads))
            return -1;
        _WRN_PRINTF ("==================================\n\n");
    }

    return 0;
}

#else   /* defined _MGHAVE_VIRTUAL_WINDOW */

int MiniGUIMain (int argc, const char* argv[])
{
    _WRN_PRINTF ("Please enable virtual window.\n");
    return 0;
}

#endif  /* not defined _MGHAVE_VIRTUAL_WINDOW */

