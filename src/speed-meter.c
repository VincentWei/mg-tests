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
** speed-meter.c: Test the speed to update screen.
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

static RECT rc_scr;

static void test_fillbox(void)
{
    struct timespec ts_start;
    clock_gettime(CLOCK_REALTIME, &ts_start);

    for (int j = 0; j < 1000; j++) {
        for (int i = 0; i < 16; i++) {
            SetBrushColor(HDC_SCREEN, SysPixelIndex[i]);
            FillBox(HDC_SCREEN, 0, 0, rc_scr.right, rc_scr.bottom);
            SyncUpdateDC(HDC_SCREEN);
        }
    }

    double elapsed = mgt_get_elapsed_seconds(&ts_start, NULL);
    _MG_PRINTF("Elapsed time: %f (seconds); full screen fills per second: %f\n",
            elapsed, 16000/elapsed);

}

static inline void test_bitblt(void)
{
    int w = RECTW(rc_scr);
    int h = RECTH(rc_scr);

    HDC memdc = CreateCompatibleDCEx(HDC_SCREEN, w, h);

    struct timespec ts_start;
    clock_gettime(CLOCK_REALTIME, &ts_start);

    for (int j = 0; j < 1000; j++) {
        for (int i = 0; i < 16; i++) {
            SetBrushColor(memdc, SysPixelIndex[i]);
            FillBox(memdc, 0, 0, rc_scr.right, rc_scr.bottom);
            BitBlt(memdc, 0, 0, w, h, HDC_SCREEN, 0, 0, 0);
            SyncUpdateDC(HDC_SCREEN);
        }
    }

    double elapsed = mgt_get_elapsed_seconds(&ts_start, NULL);
    _MG_PRINTF("Elapsed time: %f (seconds); full screen blits per second: %f\n",
            elapsed, 16000/elapsed);

    DeleteMemDC(memdc);
}

int MiniGUIMain (int argc, const char* argv[])
{
    rc_scr = GetScreenRect();

    _MG_PRINTF("Screen rect: %d, %d, %d, %d\n",
            rc_scr.left, rc_scr.top,
            rc_scr.right, rc_scr.bottom);

#ifdef _MGRM_PROCESSES
    JoinLayer(NAME_DEF_LAYER , "speed-meter" , 0 , 0);
#endif

    test_fillbox();
    test_bitblt();

    return 0;
}

#ifdef _MGRM_PROCESSS
#include <minigui/dti.c>
#endif

