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
** wallpaper-dynamic.c: a daynamic wallpaper renderer.
**
** Copyright (C) 2019 ~ 2020 FMSoft (http://www.fmsoft.cn).
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

static BOOL not_alloc_bmp_buff (void* context, BITMAP* bmp)
{
    return FALSE;
}

static int load_wallpaper (HDC hdc, const char* filename, double scale)
{
    BITMAP bmp;
    MG_RWops* area = NULL;
    int ret = 0;

    if (!(area = MGUI_RWFromFile (filename, "rb"))) {
        _ERR_PRINTF("Bad wallpaper file\n");
        ret = -1;
        goto ret;
    }

    // Get the bitmap width and height by calling LoadBitmapEx2
    LoadBitmapEx2 (HDC_SCREEN, &bmp, area, "jpg", not_alloc_bmp_buff, NULL);

    if (bmp.bmWidth > 0 && bmp.bmHeight > 0) {
        int x, y, w, h;
        static int wp_w = 0, wp_h = 0;

        if (wp_w == 0) {
            wp_w = (int)GetGDCapability (hdc, GDCAP_HPIXEL);
            wp_h = (int)GetGDCapability (hdc, GDCAP_VPIXEL);
        }

        w = (int) (bmp.bmWidth * scale);
        h = (int) (bmp.bmHeight * scale);

        x = (wp_w - w) >> 1;
        y = (wp_h - h) >> 1;

        MGUI_RWseek (area, SEEK_SET, 0);
        _DBG_PRINTF("calling StretchPaintImageEx with x(%d), y(%d), w(%d), h(%d)\n",
                x, y, w, h);
        StretchPaintImageEx (hdc, x, y, w, h, area, "jpg");
        SyncUpdateDC (hdc);
    }
    else {
        _ERR_PRINTF("Failed to get the size of wallpaper bitmap\n");
        ret = -2;
        goto ret;
    }

ret:
    if (area)
        MGUI_RWclose (area);

    return ret;
}

#define WALLPAPER_FILE          "res/wallpaper-0.jpg"

int MiniGUIMain (int argc, const char* argv[])
{
    double scale = 1.0;
    double step = 0.1;
    MSG msg;
    RECT rc_scr = GetScreenRect();
    DWORD old_tick_count;

    _MG_PRINTF("Screen rect: %d, %d, %d, %d\n",
            rc_scr.left, rc_scr.top,
            rc_scr.right, rc_scr.bottom);

    _MG_PRINTF("Wallpaper pattern size: %d, %d\n",
            GetGDCapability (HDC_SCREEN, GDCAP_HPIXEL),
            GetGDCapability (HDC_SCREEN, GDCAP_VPIXEL));

    JoinLayer(NAME_DEF_LAYER , "wallpaper" , 0 , 0);

    if (load_wallpaper (HDC_SCREEN, WALLPAPER_FILE, scale) < 0)
        exit (1);

    old_tick_count = GetTickCount ();

    /* this is a trick in order that GetMessage can return fast */
    SetTimer (HWND_DESKTOP, (LINT)&old_tick_count, 2);

    while (GetMessage (&msg, HWND_DESKTOP)) {
        DWORD curr_tick_count = GetTickCount ();
        if (curr_tick_count > old_tick_count + 2) {
            scale += step;
            if (scale > 2.0) {
                step = -0.1;
            }
            if (scale < 1.0) {
                step = 0.1;
            }

            _DBG_PRINTF("It is time to load another wallpaper.\n");
            if (load_wallpaper (HDC_SCREEN, WALLPAPER_FILE, scale) < 0)
                exit (1);
            old_tick_count = curr_tick_count;
        }

        DispatchMessage (&msg);
    }

    return 0;
}

#ifdef _MGRM_THREADS
#include <minigui/dti.c>
#endif

