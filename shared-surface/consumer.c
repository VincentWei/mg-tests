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
** Copyright (C) 2003 ~ 2021 FMSoft (http://www.fmsoft.cn).
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

#undef NDEBUG

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define _DEBUG

#include <minigui/common.h>

#ifdef _MGSCHEMA_COMPOSITING
#include "global.h"

static HDC memdc_named, memdc_clwin;
static HSURF ssurf_named, ssurf_clwin;
static unsigned int dirty_age_named, dirty_age_clwin;

static void composite_shared_surfaces(HWND hwnd, HDC hdc)
{
    static BOOL alpha_upward = TRUE;

    HSURF surf;

    surf = GetSurfaceFromDC(memdc_named);
    assert(surf == ssurf_named);

    surf = GetSurfaceFromDC(memdc_clwin);
    assert(surf == ssurf_clwin);


    RECT rc;
    GetClientRect(hwnd, &rc);

    int dst_w = RECTW(rc);
    int dst_h = RECTH(rc);

    LockSharedSurface(ssurf_named, &dirty_age_named, NULL, NULL);
    StretchBlt(memdc_named, 0, 0, 0, 0, hdc, 0, 0, dst_w, dst_h, 0);
    UnlockSharedSurface(ssurf_named, TRUE);

    Uint8 alpha = GetTickCount() % 256;
    if (!alpha_upward)
        alpha = 255 - alpha;

    SetMemDCAlpha(memdc_clwin, MEMDC_FLAG_SRCALPHA, alpha);

    if (alpha == 255)
        alpha_upward = FALSE;
    else if (alpha == 0)
        alpha_upward = TRUE;

    SetMemDCColorKey(memdc_clwin, MEMDC_FLAG_SRCCOLORKEY,
            RGBA2Pixel(memdc_clwin, 0, 0, 0, 0xFF));

    int src_w = (int)GetGDCapability(memdc_clwin, GDCAP_HPIXEL);
    int src_h = (int)GetGDCapability(memdc_clwin, GDCAP_VPIXEL);

    assert(src_w == PRODUCER_CLWIN_WIDTH);
    assert(src_h == PRODUCER_CLWIN_HEIGHT);

    LockSharedSurface(ssurf_clwin, &dirty_age_clwin, NULL, NULL);
    BitBlt(memdc_clwin, 0, 0, 0, 0,
            hdc, (dst_w - src_w) / 2, (dst_h - src_h) / 2, 0);
    UnlockSharedSurface(ssurf_clwin, FALSE);
}

static LRESULT ConsumerWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;

    switch (message) {
        case MSG_CREATE:
            SetTimer(hWnd, 100, 2);
            break;

        case MSG_TIMER:
            InvalidateRect(hWnd, NULL, FALSE);
            break;

        case MSG_PAINT:
            hdc = BeginPaint(hWnd);
            composite_shared_surfaces(hWnd, hdc);
            EndPaint(hWnd, hdc);
            return 0;

        case MSG_CLOSE:
            KillTimer(hWnd, 100);
            DestroyMainWindow(hWnd);
            PostQuitMessage(hWnd);
            return 0;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

int MiniGUIMain(int argc, const char* argv[])
{
    MSG Msg;
    HWND hMainWnd;
    MAINWINCREATE CreateInfo;

    JoinLayer(NAME_TOPMOST_LAYER , "consumer" , 0 , 0);

    if (argc > 1) {
        char name[MAX_NAME + 1];
        int cli;
        HWND hwnd;

        if (sscanf(argv[1], CONSUMER_ARG_PATTERN, name, &cli, &hwnd) != 3) {
            _ERR_PRINTF("BAD_ARG: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }

        int fd_named, fd_clwin;
        size_t sz_named, sz_clwin;
        DWORD flags_named, flags_clwin;
        fd_named = GetSharedSurfaceFDByName(name, &sz_named, &flags_named);
        if (fd_named < 0) {
            _ERR_PRINTF("FAILED_CALL: GetSharedSurfaceFDByName(%s)\n", name);
            exit(EXIT_FAILURE);
        }

        _MG_PRINTF("Got a file desrciptor for the named sharee surface: %d\n", fd_named);

        ssurf_named = AttachToSharedSurface(NULL, fd_named, sz_named, flags_named);
        if (ssurf_named == NULL) {
            _ERR_PRINTF("FAILED_CALL: AttachToSharedSurface(%d)\n", fd_named);
            exit(EXIT_FAILURE);
        }

        const char *my_name;
        SIZE sz;
        int pitch;
        size_t map_size;
        off_t pixels_off;

        my_name = GetSharedSurfaceInfo(ssurf_named, NULL, &sz,
                &pitch, &map_size, &pixels_off);
        _MG_PRINTF("pitch of %p: %d; width: %d\n", ssurf_named, pitch, sz.cx);
        assert(strcmp(my_name, name) == 0);
        assert(sz.cx == PRODUCER_NAMED_WIDTH);
        assert(sz.cy == PRODUCER_NAMED_HEIGHT);
        assert(pitch >= (PRODUCER_NAMED_WIDTH * 4));
        assert(map_size > (PRODUCER_NAMED_WIDTH * 4 * PRODUCER_NAMED_HEIGHT));
        assert(pixels_off > 0);

        _MG_PRINTF("Attached to a shared surface: %p, %d, %lu, %lx\n",
                ssurf_named, fd_named,
                (unsigned long)sz_named, (unsigned long)flags_named);
        close(fd_named);

        fd_clwin = GetSharedSurfaceFDByClientWindow(cli, hwnd,
                &sz_clwin, &flags_clwin);
        if (fd_clwin < 0) {
            _ERR_PRINTF("FAILED_CALL: GetSharedSurfaceFDByName(%s)\n", name);
            exit(EXIT_FAILURE);
        }

        _MG_PRINTF("Got fd of a shared surface: %d, %lu, %lx\n",
                fd_clwin,
                (unsigned long)sz_clwin, (unsigned long)flags_clwin);

        ssurf_clwin = AttachToSharedSurface(NULL, fd_clwin, sz_clwin, flags_clwin);
        if (ssurf_clwin == NULL) {
            _ERR_PRINTF("FAILED_CALL: AttachToSharedSurface(%d)\n", fd_clwin);
            exit(EXIT_FAILURE);
        }

        my_name = GetSharedSurfaceInfo(ssurf_clwin, NULL, &sz,
                &pitch, &map_size, &pixels_off);
        assert(strcmp(my_name, "") == 0);
        assert(sz.cx == PRODUCER_CLWIN_WIDTH);
        assert(sz.cy == PRODUCER_CLWIN_HEIGHT);
        assert(pitch >= (PRODUCER_CLWIN_WIDTH * 4));
        assert(map_size > (PRODUCER_CLWIN_WIDTH * 4 * PRODUCER_CLWIN_HEIGHT));
        assert(pixels_off > 0);

        _MG_PRINTF("Attached to a sharee surface: %p, %d, %lu, %lx\n",
                ssurf_clwin, fd_clwin,
                (unsigned long)sz_clwin, (unsigned long)flags_clwin);
        close(fd_clwin);

        memdc_named = CreateMemDCFromSurface(ssurf_named);
        if (memdc_named == HDC_INVALID) {
            _ERR_PRINTF("FAILED_CALL: CreateMemDCFromSurface(%p)\n", ssurf_named);
            exit(EXIT_FAILURE);
        }

        memdc_clwin = CreateMemDCFromSurface(ssurf_clwin);
        if (memdc_clwin == HDC_INVALID) {
            _ERR_PRINTF("FAILED_CALL: CreateMemDCFromSurface(%p)\n", ssurf_clwin);
            exit(EXIT_FAILURE);
        }

    }
    else {
        _ERR_PRINTF("MISSED_ARG: %d\n", argc);
        exit(EXIT_FAILURE);
    }

    CreateInfo.dwStyle =
        WS_VISIBLE | WS_BORDER | WS_CAPTION;
    CreateInfo.dwExStyle = WS_EX_WINTYPE_HIGHER;
    CreateInfo.spCaption = "Consumer of shared surfaces";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = ConsumerWinProc;
    CreateInfo.lx = PRODUCER_CLWIN_WIDTH;
    CreateInfo.ty = PRODUCER_CLWIN_HEIGHT;
    CreateInfo.rx = PRODUCER_CLWIN_WIDTH + CONSUMER_WIDTH;
    CreateInfo.by = PRODUCER_CLWIN_HEIGHT + CONSUMER_HEIGHT;
    CreateInfo.iBkColor = COLOR_black;
    CreateInfo.dwAddData = 0;
    CreateInfo.hHosting = HWND_DESKTOP;

    hMainWnd = CreateMainWindow(&CreateInfo);
    if (hMainWnd == HWND_INVALID) {
        exit(EXIT_FAILURE);
    }

    ShowWindow(hMainWnd, SW_SHOWNORMAL);

    while (GetMessage(&Msg, hMainWnd)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    MainWindowThreadCleanup(hMainWnd);

    DeleteMemDC(memdc_named);
    DetachFromSharedSurface(ssurf_named);

    DeleteMemDC(memdc_clwin);
    DetachFromSharedSurface(ssurf_clwin);

    return EXIT_SUCCESS;
}

#else  /* defined _MGSCHEMA_COMPOSITING */

int main(int argc, const char* argv[])
{
    _WRN_PRINTF ("This test program is a client for compositing schema."
           "But your MiniGUI was not configured as compositing schema.\n");
    return EXIT_SUCCESS;
}

#endif  /* not defined _MGSCHEMA_COMPOSITING */

