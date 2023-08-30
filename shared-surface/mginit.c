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
** Copyright (C) 2003 ~ 2020 FMSoft (http://www.fmsoft.cn).
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
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <minigui/common.h>

#ifdef _MGSCHEMA_COMPOSITING

#include "global.h"

static int nr_clients = 0;
static pid_t pid_producer_named = 0;
static pid_t pid_producer_clwin = 0;
static pid_t pid_consumer = 0;

static pid_t exec_app(const char* file_name, const char* app_name,
        const char *extra_arg)
{
    pid_t pid = 0;

    if ((pid = vfork()) > 0) {
        _MG_PRINTF("new child, pid: %d.\n", pid);
    }
    else if (pid == 0) {
        execl(file_name, app_name, extra_arg, NULL);
        perror("execl");
        _exit(1);
    }
    else {
        perror("vfork");
    }

    return pid;
}

static int on_producer_named_ready(int cli, int clifd, void* buff, size_t len)
{
    int ret_value = 0;
    if (strcmp(buff, SHARED_SURFACE_READY)) {
        _ERR_PRINTF("FAILED: `%s` expected but got `%s`\n",
                SHARED_SURFACE_READY, (char *)buff);
        exit(EXIT_FAILURE);
    }

    RegisterRequestHandler(REQID_PRODUCER_NAMED_READY, NULL);

    pid_producer_clwin = exec_app("./producer-clwin", "producer-clwin", NULL);
    return ServerSendReply(clifd, &ret_value, sizeof(int));
}

static int on_producer_clwin_ready(int cli, int clifd, void* buff, size_t len)
{
    char arg[128];
    int ret_value = 0;

    struct producer_clwin_info *info = (struct producer_clwin_info *)buff;
    int ret = snprintf(arg, sizeof(arg), CONSUMER_ARG_PATTERN, SHARED_SURFACE_NAME,
            info->cli, info->hwnd);
    if (ret <= 0 || (size_t)ret >= sizeof(arg)) {
        _ERR_PRINTF("TOO_SMALL: buffer for argument\n");
        exit(EXIT_FAILURE);
    }

    RegisterRequestHandler(REQID_PRODUCER_CLWIN_READY, NULL);
    pid_consumer = exec_app("./consumer", "consumer", arg);
    return ServerSendReply(clifd, &ret_value, sizeof(int));
}

static void on_new_del_client (int op, int cli)
{
    if (op == LCO_NEW_CLIENT) {
        nr_clients ++;
        _MG_PRINTF ("A new client: %d.\n", mgClients[cli].pid);
    }
    else if (op == LCO_DEL_CLIENT) {
        _MG_PRINTF ("A client left: %d.\n", mgClients[cli].pid);
        nr_clients --;
        if (nr_clients == 0) {
            _MG_PRINTF ("There is no any client.\n");
        }
        else if (nr_clients < 0) {
            _ERR_PRINTF ("Serious error: nr_clients less than zero.\n");
        }
    }
    else
        _ERR_PRINTF ("Serious error: incorrect operations.\n");
}

static unsigned int old_tick_count;

static inline void dump_key_messages (PMSG msg)
{
    if (msg->message == MSG_KEYDOWN || msg->message == MSG_KEYUP) {
        _WRN_PRINTF ("%s (%d) %s KS_REPEATED\n",
                (msg->message == MSG_KEYDOWN)?"MSG_KEYDOWN":"MSG_KEYUP",
                (int)msg->wParam,
                (msg->lParam & KS_REPEATED)?"with":"without");
    }
}

static int my_event_hook (PMSG msg)
{
    old_tick_count = GetTickCount ();

    dump_key_messages(msg);

    if (msg->message == MSG_KEYDOWN) {
        switch (msg->wParam) {
        case SCANCODE_ESCAPE:
            if (nr_clients == 1 && pid_producer_named) {
                kill(pid_producer_named, SIGINT);
                pid_producer_named = 0;
            }
            break;

        case SCANCODE_SPACE:
           break;
    }
    }

    return HOOK_GOON;
}

static void child_wait(int sig)
{
    int pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status))
            _MG_PRINTF("--pid=%d--status=%x--rc=%d---\n",
                    pid, status, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            _MG_PRINTF("--pid=%d--signal=%d--\n", pid, WTERMSIG (status));
    }
}

int MiniGUIMain(int argc, const char* argv[])
{
    MSG msg;
    struct sigaction siga;

    siga.sa_handler = child_wait;
    siga.sa_flags  = 0;
    memset (&siga.sa_mask, 0, sizeof(sigset_t));
    sigaction(SIGCHLD, &siga, NULL);

    OnNewDelClient = on_new_del_client;

    if (!ServerStartup(0 , 0 , 0)) {
        _ERR_PRINTF("Can not start the server of MiniGUI-Processes: mginit.\n");
        return 1;
    }

    SetServerEventHook(my_event_hook);

    RegisterRequestHandler(REQID_PRODUCER_NAMED_READY, on_producer_named_ready);
    RegisterRequestHandler(REQID_PRODUCER_CLWIN_READY, on_producer_clwin_ready);
    pid_producer_named =
        exec_app("./producer-named", "producer-named", "wallpaper");

    old_tick_count = GetTickCount();
    while (GetMessage(&msg, HWND_DESKTOP)) {
        DispatchMessage(&msg);
    }

    return 0;
}

#else   /* defined _MGSCHEMA_COMPOSITING */

int main(int argc, const char* argv[])
{
    _WRN_PRINTF("This test program is the server for compositing schema. "
           "But your MiniGUI was not configured for compositing schema.\n");
    return EXIT_SUCCESS;
}

#endif  /* not defined _MGSCHEMA_COMPOSITING */

