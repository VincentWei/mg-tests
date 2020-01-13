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
**
** wallpaper.c: wallpaper renderer.
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

static int load_wallpaper (HDC hdc, const char* filename)
{
    int x, y;
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
        int wp_w, wp_h;
        wp_w = (int)GetGDCapability (hdc, GDCAP_HPIXEL);
        wp_h = (int)GetGDCapability (hdc, GDCAP_VPIXEL);

        x = (wp_w - bmp.bmWidth) >> 1;
        y = (wp_h - bmp.bmHeight) >> 1;
    }
    else {
        _ERR_PRINTF("Failed to get the size of wallpaper bitmap\n");
        ret = -2;
        goto ret;
    }

    MGUI_RWseek (area, SEEK_SET, 0);
    PaintImageEx (HDC_SCREEN, x, y, area, "jpg");

ret:
    if (area)
        MGUI_RWclose (area);

    return ret;
}

static unsigned int old_tick_count;

#define WALLPAPER_FILE          "res/wallpaper-0.jpg"

int MiniGUIMain (int argc, const char* argv[])
{
    MSG msg;

    JoinLayer(NAME_DEF_LAYER , "wallpaper" , 0 , 0);

    if (load_wallpaper (HDC_SCREEN, WALLPAPER_FILE) < 0)
        exit (1);

    old_tick_count = GetTickCount ();
    while (GetMessage (&msg, HWND_DESKTOP)) {
        if (GetTickCount () > old_tick_count + 10000) {
            _WRN_PRINTF("It is time to load another wallpaper.");
        }

        DispatchMessage (&msg);
    }

    return 0;
}

#ifdef _MGRM_THREADS
#include <minigui/dti.c>
#endif

