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
**  Test code of timer for MiniGUI 5.0.0
**
**  This test program continually sleeps and call GetTickCount, then calculate
**  the error giving by the sleep time and the GetTickCount.
**
**  The following APIs are covered:
**
**      GetTickCount
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

#define USEC_10MS   10000UL

int test_main_entry (int nr_times, int max_sleep_us)
{
    int count = nr_times;
    int nr_errors = 0;
    int max_error = 0;

    do {
        DWORD last_ticks, current_ticks;
        DWORD expected_ticks;
        DWORD sleep_us = random () % max_sleep_us;
        DWORD error;

        last_ticks = GetTickCount ();
        _MG_PRINTF ("sleep %lu us (left times: %d)\n", sleep_us, count);
        usleep (sleep_us);
        current_ticks = GetTickCount ();

        expected_ticks = last_ticks + sleep_us/USEC_10MS;
        if (expected_ticks > current_ticks) {
            error = expected_ticks - current_ticks;
            _MG_PRINTF ("expected_ticks > current_ticks: error: %lu\n", error);
        }
        else {
            error = current_ticks - expected_ticks;
            _MG_PRINTF ("expected_ticks <= current_ticks: error: %lu\n", error);
        }

        if (error > max_error)
            max_error = error;

        if (error > 0)
            nr_errors++;

        if (error > 2) {
            _WRN_PRINTF ("Too large error: %lu\n", error);
            assert (0);
            break;
        }

    } while (--count > 0);

    _MG_PRINTF ("percent with errors: %d%% (%d/%d)\n", (nr_errors*100)/nr_times, nr_errors, nr_times);
    return max_error;
}

#define DEF_NR_LOOPS    3
#define DEF_NR_TIMES    10

int MiniGUIMain (int argc, const char* argv[])
{
    int nr_loops = DEF_NR_LOOPS;    // total loops
    int nr_times = DEF_NR_TIMES;    // times per loop
    int max_error = 0;

    JoinLayer (NAME_DEF_LAYER , "gettickcount" , 0 , 0);

    srandom (time(NULL));

    if (argc > 1 && strcmp(argv[1], "auto") == 0) {
        // use the defaults
    }
    else {
        if (argc > 1)
            nr_loops = atoi (argv[1]);
        if (nr_loops < 0)
            nr_loops = DEF_NR_LOOPS;

        if (argc > 2)
            nr_times = atoi (argv[2]);
        if (nr_times < 0)
            nr_times = DEF_NR_TIMES;
    }

    _MG_PRINTF ("size of size_t: %lu\n", sizeof (size_t));

    for (int i = 0; i < nr_loops; i++) {
        int max_sleep_us = random() % 0x00FFFFFFUL;
        int error;
        if (max_sleep_us == 0)
            max_sleep_us = 1;

        _WRN_PRINTF ("Starting loop %d.\n", i);

        error = test_main_entry (nr_times, max_sleep_us);
        if (error > max_error)
            max_error = error;

        _WRN_PRINTF ("End of loop %d\n\n", i);
    }

    _MG_PRINTF ("Test for GetTickCount passed; max error (%d), total times: %d\n",
            max_error, nr_loops * nr_times);
    exit(0);
    return 0;
}

