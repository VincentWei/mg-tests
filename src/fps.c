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
** fps.c: Calculating FPS (frames per seconds).
**
** Copyright (C) 2023 FMSoft (http://www.fmsoft.cn).
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
#include <time.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

double mgt_get_elapsed_seconds(const struct timespec *ts_from,
        const struct timespec *ts_to)
{
    struct timespec ts_curr;
    time_t ds;
    long dns;

    if (ts_to == NULL) {
        clock_gettime(CLOCK_REALTIME, &ts_curr);
        ts_to = &ts_curr;
    }

    ds = ts_to->tv_sec - ts_from->tv_sec;
    dns = ts_to->tv_nsec - ts_from->tv_nsec;
    return ds + dns * 1.0E-9;
}

int MiniGUIMain (int argc, const char* argv[])
{
    RECT rc_scr = GetScreenRect();

    _MG_PRINTF("Screen rect: %d, %d, %d, %d\n",
            rc_scr.left, rc_scr.top,
            rc_scr.right, rc_scr.bottom);

#ifdef _MGRM_PROCESSES
    JoinLayer(NAME_DEF_LAYER , "fps" , 0 , 0);
#endif

    struct timespec ts_start;
    clock_gettime(CLOCK_REALTIME, &ts_start);

    for (int j = 0; j < 100; j++) {
        for (int i = 0; i < 16; i++) {
            SetBrushColor(HDC_SCREEN, SysPixelIndex[i]);
            FillBox(HDC_SCREEN, 0, 0, rc_scr.right, rc_scr.bottom);
            SyncUpdateDC(HDC_SCREEN);
        }
    }

    double elapsed = mgt_get_elapsed_seconds(&ts_start, NULL);
    _MG_PRINTF("Elapsed time: %f (seconds), fps: %f\n",
            elapsed, 1600/elapsed);

    return 0;
}

#ifdef _MGRM_PROCESSS
#include <minigui/dti.c>
#endif

