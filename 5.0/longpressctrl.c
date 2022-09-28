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
**  Test code of SetCapture, ReleaseCapture, and MSG_IDLE for MiniGUI 5.0.
**
**  This test program creates some main windows in different z-order levels.
**  Every main window will have a distinct background color and size.
**  The program will check the correctness of a z-order operation by reading
**  some pixel values of specific pixels on the screen.
**
**  Under MiniGUI-Threads runtime mode, this program will create 7 GUI threads
**  for all z-order levels and create main windows in different levels in
**  theses threads.
**
**  Note that this program cannot give correct result under compositing schema.
**  Also note that the underlying screen should run in 32-bit color depth.
**
**  The following APIs are covered:
**
**      CreateMainWindow
**      DestroyMainWindow
**      RegisterWindowClass
**      CreateWindow
**      SetCapture
**      ReleaseCapture
**      SetWindowAdditionalData2
**      GetWindowAdditionalData2
**      InvalidateRect
**      PostMessage
**      DestroyAllControls
**      MainWindowCleanup
**      MSG_IDLE
**
** Copyright (C) 2022 FMSoft (http://www.fmsoft.cn).
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
#include <minigui/control.h>

enum {
    MSG_LBTN_DOWN = MSG_USER + 100,
    MSG_LBTN_UP,
    MSG_LBTN_LONGPRESSED,
};

#define CTRL_STATUS_NONE            0x00
#define CTRL_STATUS_CAPTURED        0x01
#define CTRL_STATUS_LONGPRESSED     0x02

struct longpress_ctrl_head {
    DWORD       status;
    DWORD       downtime;
    unsigned    ticks;  /* how many ticks to generate longpressed message */
};

static inline void initLongPressCtrlHead(struct longpress_ctrl_head *head,
        unsigned longpress_ticks)
{
    assert(longpress_ticks > 50);

    head->status = CTRL_STATUS_NONE;
    head->ticks = longpress_ticks;
}

LRESULT
longpressCtrlProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#define IDC_MYBUTTON    100

static unsigned nr_longpressed;

LRESULT
longpressCtrlProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_LBUTTONDOWN: {
        struct longpress_ctrl_head *head;
        head = (struct longpress_ctrl_head *)GetWindowAdditionalData2(hWnd);
        if (head) {
            SetCapture(hWnd);

            head->status |= CTRL_STATUS_CAPTURED;
            head->downtime = GetTickCount();
            PostMessage(hWnd, MSG_LBTN_DOWN, wParam, lParam);
        }
        break;
    }

    case MSG_IDLE: {
        struct longpress_ctrl_head *head;
        head = (struct longpress_ctrl_head *)GetWindowAdditionalData2(hWnd);
        if (head && head->status & CTRL_STATUS_CAPTURED) {
            if (wParam >= head->downtime + head->ticks) {
                PostMessage(hWnd, MSG_LBTN_LONGPRESSED, 0, 0);
                head->downtime = GetTickCount();
                head->status |= CTRL_STATUS_LONGPRESSED;
            }
        }
        break;
    }

    case MSG_LBUTTONUP: {
        struct longpress_ctrl_head *head;
        head = (struct longpress_ctrl_head *)GetWindowAdditionalData2(hWnd);
        if (head) {
            ReleaseCapture();
            head->status = CTRL_STATUS_NONE;
            PostMessage(hWnd, MSG_LBTN_UP, wParam, lParam);
        }
        break;
    }

    default:
        break;
    }

    return DefaultControlProc (hWnd, message, wParam, lParam);
}

struct my_ctrl_data {
    struct longpress_ctrl_head head;
    int foobar;
    int bar;
};

static LRESULT
myCtrlProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE: {
        struct my_ctrl_data *data;
        data = (struct my_ctrl_data *)calloc(1, sizeof(*data));
        assert(data);
        initLongPressCtrlHead(&data->head, 200); // 2s
        SetWindowAdditionalData2(hWnd, (DWORD)data);
        break;
    }

    case MSG_LBTN_DOWN:
        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case MSG_LBTN_UP:
        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case MSG_LBTN_LONGPRESSED:
        InvalidateRect(hWnd, NULL, TRUE);
    break;

    case MSG_PAINT: {
        struct my_ctrl_data *data;
        data = (struct my_ctrl_data *)GetWindowAdditionalData2(hWnd);

        HDC hdc = BeginPaint (hWnd);
        SetBkMode(hdc, BM_TRANSPARENT);
        if (data->head.status & CTRL_STATUS_LONGPRESSED) {
            char buff[512];
            sprintf(buff, "Left button long pressed: 0x%lx", data->head.downtime);
            TextOut(hdc, 10, 0, buff);
            _MG_PRINTF("Left button long pressed: 0x%lx\n", data->head.downtime);
            nr_longpressed++;
        }
        else if (data->head.status & CTRL_STATUS_CAPTURED) {
            TextOut(hdc, 10, 0, "Left button pressed and captured");
            _MG_PRINTF("Left button pressed and captured\n");
        }
        else {
            TextOut(hdc, 10, 0, "Left button is not pressed");
            _MG_PRINTF("Left button is not pressed\n");
        }
        EndPaint(hWnd, hdc);
        return 0;
    }

    case MSG_DESTROY: {
        struct my_ctrl_data *data;
        data = (struct my_ctrl_data *)GetWindowAdditionalData2(hWnd);
        free(data);
        break;
    }

    default:
        break;
    }

    /* use longpressCtrlProc here */
    return longpressCtrlProc(hWnd, message, wParam, lParam);
}

static BOOL registerMyCtrl (void)
{
    WNDCLASS WndClass;

    WndClass.spClassName = "myctrl";
    WndClass.dwStyle     = 0;
    WndClass.dwExStyle   = 0;
    WndClass.hCursor     = GetSystemCursor(0);
#ifdef _MGSCHEMA_COMPOSITING
    WndClass.dwBkColor   = RGBA_lightgray;
#else
    WndClass.iBkColor    = PIXEL_lightgray;
#endif
    WndClass.WinProc     = myCtrlProc;

    return RegisterWindowClass(&WndClass);
}

/* main windoww proc */
static LRESULT
myWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        registerMyCtrl();
        CreateWindow("myctrl", "", WS_VISIBLE | WS_CHILD, IDC_MYBUTTON,
                5, 5, 300, 200, hWnd, 0);
        break;

    case MSG_CLOSE:
        DestroyAllControls(hWnd);
        DestroyMainWindow(hWnd);
        PostQuitMessage(hWnd);
        return 0;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

static int real_entry(int argc, const char* arg[])
{
    MSG Msg;
    HWND hMainWnd;
    MAINWINCREATE CreateInfo;

    JoinLayer(NAME_DEF_LAYER , "longpressctrl" , 0 , 0);

    CreateInfo.dwStyle = WS_VISIBLE | WS_BORDER | WS_CAPTION;
    CreateInfo.dwExStyle = WS_EX_NONE;
    CreateInfo.spCaption = "Control Supporting Longpress";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = myWinProc;
    CreateInfo.lx = 0;
    CreateInfo.ty = 0;
    CreateInfo.rx = 320;
    CreateInfo.by = 240;
    CreateInfo.iBkColor = COLOR_lightwhite;
    CreateInfo.dwAddData = 0;
    CreateInfo.hHosting = HWND_DESKTOP;

    hMainWnd = CreateMainWindow (&CreateInfo);

    if (hMainWnd == HWND_INVALID) {
        return 1;
    }

    ShowWindow(hMainWnd, SW_SHOWNORMAL);

    while (GetMessage(&Msg, hMainWnd)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    MainWindowCleanup (hMainWnd);

    if (nr_longpressed > 0) {
        _MG_PRINTF("PASSED\n");
        return 0;
    }

    _MG_PRINTF("FAILED\n");
    return 1;
}

int main(int argc, const char* argv[])
{
    int iRet = 0;

    if (argc > 1 && strcmp(argv[1], "auto") == 0) {
#ifdef _MGRM_PROCESSES
        _MG_PRINTF("This test cannot run under MiniGUI-Processes automatically.\n");
        _MG_PRINTF("Skipped.\n");
        exit(0);
#else
        setenv("MG_IAL_ENGINE", "auto", 1);
        setenv("MG_ENV_RECORD_IAL", "capture-input.dat", 1);
#endif
    }

    if (InitGUI (argc, argv) != 0) {
        return 1;
    }

    iRet = real_entry(argc, argv);
    exit(iRet);

    TerminateGUI(iRet);
    return iRet;
}

