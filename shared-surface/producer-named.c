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
** producer-named.c: a name-based shared surface producer.
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

#include <minigui/common.h>

#ifdef _MGSCHEMA_COMPOSITING
#include "global.h"

static BOOL not_alloc_bmp_buff (void* context, BITMAP* bmp)
{
    return FALSE;
}

static int paint_wallpaper(HDC hdc, const char* filename, double scale)
{
    BITMAP bmp;
    MG_RWops* area = NULL;
    int ret = 0;

    if (!(area = MGUI_RWFromFile(filename, "rb"))) {
        _ERR_PRINTF("Bad wallpaper file\n");
        ret = -1;
        goto ret;
    }

    // Get the bitmap width and height by calling LoadBitmapEx2
    LoadBitmapEx2 (hdc, &bmp, area, "jpg", not_alloc_bmp_buff, NULL);

    if (bmp.bmWidth > 0 && bmp.bmHeight > 0) {
        int x, y, w, h;
        static int wp_w = 0, wp_h = 0;

        if (wp_w == 0) {
            wp_w = (int)GetGDCapability(hdc, GDCAP_HPIXEL);
            wp_h = (int)GetGDCapability(hdc, GDCAP_VPIXEL);
        }

        w = (int)(bmp.bmWidth * scale);
        h = (int)(bmp.bmHeight * scale);

        x = (wp_w - w) >> 1;
        y = (wp_h - h) >> 1;

        MGUI_RWseek (area, SEEK_SET, 0);
        _DBG_PRINTF("calling StretchPaintImageEx with x(%d), y(%d), w(%d), h(%d)\n",
                x, y, w, h);
        StretchPaintImageEx(hdc, x, y, w, h, area, "jpg");
        SyncUpdateDC(hdc);
    }
    else {
        _ERR_PRINTF("Failed to get the size of wallpaper bitmap\n");
        ret = -2;
        goto ret;
    }

ret:
    if (area)
        MGUI_RWclose(area);

    return ret;
}

#define WALLPAPER_FILE          "res/wallpaper-0.jpg"

int MiniGUIMain (int argc, const char* argv[])
{
    double scale = 1.0;
    double step = 0.1;
    MSG msg;
    RECT rc_scr = GetScreenRect();

    _MG_PRINTF("Screen rect: %d, %d, %d, %d\n",
            rc_scr.left, rc_scr.top,
            rc_scr.right, rc_scr.bottom);

    JoinLayer(NAME_DEF_LAYER , "producer-surface" , 0 , 0);

    const char *name = NULL;
    if (argc > 1) {
        name = argv[1];
    }
    else {
        name = SHARED_SURFACE_NAME;
    }

    HSURF ssurf = CreateSharedSurface(NULL,
            name, MEMDC_FLAG_HWSURFACE,
            PRODUCER_NAMED_WIDTH,
            PRODUCER_NAMED_HEIGHT,
            32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    if (ssurf == NULL) {
        _ERR_PRINTF("Failed to create a shared surface by name: %s\n", name);
        exit(EXIT_FAILURE);
    }

    HDC memdc = CreateMemDCFromSurface(ssurf);
    if (memdc == HDC_INVALID) {
        _ERR_PRINTF("Failed to create memdc for the shared surface\n");
        exit(EXIT_FAILURE);
    }

    if (paint_wallpaper(memdc, WALLPAPER_FILE, scale) < 0) {
        _ERR_PRINTF("Failed to load wallpaper\n");
        exit(EXIT_FAILURE);
    }

    REQUEST req = {
        REQID_PRODUCER_NAMED_READY,
        SHARED_SURFACE_READY,
        sizeof(SHARED_SURFACE_READY)
    };

    int result;
    if (ClientRequest(&req, &result, sizeof(int))) {
        _ERR_PRINTF("BAD_REQUEST: %d\n", REQID_PRODUCER_NAMED_READY);
        exit(EXIT_FAILURE);
    }

    if (result) {
        _ERR_PRINTF("FAILED_REQUEST: %d\n", REQID_PRODUCER_NAMED_READY);
        exit(EXIT_FAILURE);
    }

    DWORD old_tick_count, org_tick_count;
    old_tick_count = org_tick_count = GetTickCount();

    /* this is a trick in order that GetMessage can return fast */
    SetTimer(HWND_DESKTOP, (LINT)&old_tick_count, 2);

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
            if (paint_wallpaper(memdc, WALLPAPER_FILE, scale) < 0)
                exit(EXIT_FAILURE);
            old_tick_count = curr_tick_count;
        }

        DispatchMessage (&msg);
    }

    _MG_PRINTF("Quitting...\n");

    DeleteMemDC(memdc);
    DestroySharedSurface(ssurf);

    SendMessage (HWND_DESKTOP, MSG_ENDSESSION, 0, 0);

    exit(EXIT_SUCCESS);
    return 0;
}

#else  /* defined _MGSCHEMA_COMPOSITING */

int main(int argc, const char* argv[])
{
    _WRN_PRINTF ("This test program is a client for compositing schema."
           "But your MiniGUI was not configured as compositing schema.\n");
    return EXIT_SUCCESS;
}

#endif  /* not defined _MGSCHEMA_COMPOSITING */

