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
** showimage.c: Test program to show various images.
**
** Copyright (C) 2021 FMSoft (http://www.fmsoft.cn).
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

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "list.h"

static BOOL is_supported(const char *filename)
{
    char *extend, *temp;
    extend = rindex (filename,'.');
    if (extend ==NULL)
        return FALSE;

    extend++;
    temp = extend;
    while (*temp) {
        *temp = tolower(*temp);
        temp++;
    }

    if (
#ifdef _MGIMAGE_JPG
            !strncmp(extend, "jpg", 3) ||
            !strncmp(extend, "jpeg", 4) ||
#endif
#ifdef _MGIMAGE_GIF
            !strncmp(extend, "gif", 3) ||
#endif
#ifdef _MGIMAGE_PCX
            !strncmp(extend, "pcx", 3) ||
#endif
#ifdef _MGIMAGE_LBM
            !strncmp(extend, "lbm", 3) ||
#endif
#ifdef _MGIMAGE_TGA
            !strncmp(extend, "tga", 3) ||
#endif
#ifdef _MGIMAGE_PNG
            !strncmp(extend, "png", 3) ||
#endif
#ifdef _MGIMAGE_WEBP
            !strncmp(extend, "webp", 4) ||
#endif
            !strncmp(extend, "bmp", 3))
        return TRUE;

    return FALSE;
}

static LIST_HEAD(_file_list);

struct image_file {
    struct list_head    list;
    char               *path;
};

static int append_image_files(const char *path)
{
    int             n = 0;
    DIR            *dir;
    struct dirent  *entry;
    struct stat    ftype;
    char           fullpath[PATH_MAX + NAME_MAX + 1];

    if ((dir = opendir(path)) == NULL)
        return -1;

    while ((entry = readdir(dir)) != NULL) {

        strncpy(fullpath, path, PATH_MAX);
        if (fullpath[strlen(fullpath) - 1] != '/')
            strcat(fullpath, "/");

        strcat(fullpath, entry->d_name);
        if (lstat(fullpath, &ftype) < 0) {
            continue;
        }

        if (S_ISREG(ftype.st_mode) && is_supported(entry->d_name)) {
            struct image_file *file;

            file = malloc(sizeof(struct image_file));
            file->path = strdup(fullpath);

            list_add_tail(&file->list, &_file_list);

            n++;
        }
    }

    closedir(dir);

    return n;
}

static int destroy_image_file_list(void)
{
    int n = 0;
    struct image_file *p, *tmp;

    list_for_each_entry_safe(p, tmp, &_file_list, list) {
        free(p->path);
        free(p);

        n++;
    }

    return n;
}

static struct list_head *_curr;

static void next_image(HWND hWnd)
{
    if (_curr == NULL) {
        _curr = _file_list.next;
    }
    else {
        _curr = _curr->next;
    }

    InvalidateRect(hWnd, NULL, TRUE);
}

static void show_image(HDC hdc)
{
    struct image_file *image = list_entry(_curr, struct image_file, list);

    PaintImageFromFile(hdc, 0, 0, image->path);
}

static LRESULT win_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        next_image(hWnd);
        SetTimer (hWnd, 100, 200);
        break;

    case MSG_TIMER:
        next_image(hWnd);
        break;

    case MSG_PAINT: {
        HDC hdc;

        hdc = BeginPaint(hWnd);
        show_image(hdc);
        EndPaint(hWnd, hdc);
        return 0;
    }

    case MSG_KEYDOWN:
        if (wParam == SCANCODE_SPACE) {
            next_image(hWnd);
        }
        else {
            PostMessage(hWnd, MSG_CLOSE, 0, 0);
        }
        return 0;

    case MSG_CLOSE:
        KillTimer(hWnd, 100);
        DestroyMainWindow(hWnd);
        PostQuitMessage(hWnd);
        return 0;

    default:
        break;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

int MiniGUIMain(int argc, const char* argv[])
{
    int i, n;
    int nr_files = 0;
    MSG Msg;
    HWND hMainWnd;
    MAINWINCREATE CreateInfo;

    for (i = 1; i < argc; i++) {
        n = append_image_files(argv[i]);
        if (n < 0) {
            _MG_PRINTF("can not open dir: %s\n", argv[i]);
        }
        else if (n == 0) {
            _MG_PRINTF("No image files found in dir: %s\n", argv[i]);
        }
        else
            nr_files += n;
    }

    if (nr_files <= 0) {
        _MG_PRINTF("No image files found in all directories\n");
        return 0;
    }

#ifdef _MGRM_PROCESSES
    JoinLayer(NAME_DEF_LAYER, "showimage", 0, 0);
#endif

    CreateInfo.dwStyle = WS_VISIBLE | WS_BORDER | WS_CAPTION;
    CreateInfo.dwExStyle = WS_EX_NONE;
    CreateInfo.spCaption = "Show Image";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = win_proc;
    CreateInfo.lx = 0;
    CreateInfo.ty = 0;
    CreateInfo.rx = 0;
    CreateInfo.by = 0;
    CreateInfo.iBkColor = PIXEL_lightwhite;
    CreateInfo.dwAddData = 0;
    CreateInfo.hHosting = HWND_DESKTOP;

    hMainWnd = CreateMainWindow (&CreateInfo);

    if (hMainWnd == HWND_INVALID)
        return 1;

    ShowWindow(hMainWnd, SW_SHOWNORMAL);

    while (GetMessage(&Msg, hMainWnd)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    MainWindowThreadCleanup (hMainWnd);

    n = destroy_image_file_list();
    assert(n == nr_files);

    return 0;
}

