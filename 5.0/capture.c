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
**  Test code of SetCapture/ReleaseCapture, MSG_IDLE and  for MiniGUI 5.0.
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
**      GetWindowLocalData
**      SetWindowLocalData
**      RemoveWindowLocalData
**      InvalidateRect
**      PostMessage
**      DestroyAllControls
**      MainWindowCleanup
**      MSG_IDLE
**
** capture.c: Sample program for MiniGUI Programming Guide
**      Demo of using mouse capture
**
** Copyright (C) 2003 ~ 2017 FMSoft (http://www.fmsoft.cn).
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

#define IDC_MYBUTTON    100
#define MSG_LBTN_LONGPRESSED        (MSG_USER + 100)

#define CTRL_DATA_NAME_STATUS       "status"
#define CTRL_DATA_NAME_DOWNTIME     "downTime"

#define CTRL_STATUS_NONE            0x00
#define CTRL_STATUS_CAPTURED        0x01
#define CTRL_STATUS_LONGPRESSED     0x02

static unsigned nr_longpressed;

/* a simple button control */
static LRESULT
mybuttonWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        SetWindowLocalData(hWnd, CTRL_DATA_NAME_STATUS, CTRL_STATUS_NONE, NULL);
        break;

    case MSG_LBUTTONDOWN: {
        DWORD status;

        SetCapture(hWnd);

        GetWindowLocalData(hWnd, CTRL_DATA_NAME_STATUS, &status, NULL);
        SetWindowLocalData(hWnd, CTRL_DATA_NAME_STATUS,
                status | CTRL_STATUS_CAPTURED, NULL);
        SetWindowLocalData(hWnd, CTRL_DATA_NAME_DOWNTIME,
                GetTickCount(), NULL);
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    }

    case MSG_IDLE: {
        DWORD status;
        DWORD downtime;

        GetWindowLocalData(hWnd, CTRL_DATA_NAME_STATUS, &status, NULL);
        if (status & CTRL_STATUS_CAPTURED) {
            GetWindowLocalData(hWnd, CTRL_DATA_NAME_DOWNTIME, &downtime, NULL);
            if (wParam > downtime + 200) { // 2s
                PostMessage(hWnd, MSG_LBTN_LONGPRESSED, 0, 0);
                SetWindowLocalData(hWnd, CTRL_DATA_NAME_DOWNTIME,
                    GetTickCount(), NULL);
                SetWindowLocalData(hWnd, CTRL_DATA_NAME_STATUS,
                        status | CTRL_STATUS_LONGPRESSED, NULL);
            }
        }
        break;
    }

    case MSG_LBUTTONUP:
        ReleaseCapture();
        SetWindowLocalData(hWnd, CTRL_DATA_NAME_STATUS, CTRL_STATUS_NONE, NULL);
        InvalidateRect(hWnd, NULL, TRUE);
    break;

    case MSG_LBTN_LONGPRESSED:
        InvalidateRect(hWnd, NULL, TRUE);
    break;

    case MSG_PAINT: {
        DWORD status;
        GetWindowLocalData(hWnd, CTRL_DATA_NAME_STATUS, &status, NULL);

        HDC hdc = BeginPaint (hWnd);
        SetBkMode(hdc, BM_TRANSPARENT);
        if (status & CTRL_STATUS_LONGPRESSED) {
            DWORD downtime;
            GetWindowLocalData(hWnd, CTRL_DATA_NAME_DOWNTIME, &downtime, NULL);

            char buff[512];
            sprintf(buff, "Left button long pressed: 0x%lx", downtime);
            TextOut(hdc, 10, 0, buff);
            _MG_PRINTF("Left button long pressed: 0x%lx\n", downtime);
            nr_longpressed++;
        }
        else if (status & CTRL_STATUS_CAPTURED) {
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

    case MSG_DESTROY:
        RemoveWindowLocalData(hWnd, CTRL_DATA_NAME_STATUS);
        RemoveWindowLocalData(hWnd, CTRL_DATA_NAME_DOWNTIME);
        return 0;
    }

    return DefaultControlProc (hWnd, message, wParam, lParam);
}

static BOOL registerMyButton (void)
{
    WNDCLASS WndClass;

    WndClass.spClassName = "mybutton";
    WndClass.dwStyle     = 0;
    WndClass.dwExStyle   = 0;
    WndClass.hCursor     = GetSystemCursor(0);
#ifdef _MGSCHEMA_COMPOSITING
    WndClass.dwBkColor   = RGBA_lightgray;
#else
    WndClass.iBkColor    = PIXEL_lightgray;
#endif
    WndClass.WinProc     = mybuttonWindowProc;

    return RegisterWindowClass(&WndClass);
}

/* main windoww proc */
static LRESULT
CaptureWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        registerMyButton();
        CreateWindow("mybutton", "", WS_VISIBLE | WS_CHILD, IDC_MYBUTTON,
                5, 5, 300, 200, hWnd, 0);
        break;

    case MSG_CLOSE:
        DestroyAllControls (hWnd);
        DestroyMainWindow (hWnd);
        PostQuitMessage (hWnd);
        return 0;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

static int real_entry(int argc, const char* arg[])
{
    MSG Msg;
    HWND hMainWnd;
    MAINWINCREATE CreateInfo;

    JoinLayer(NAME_DEF_LAYER , "capture" , 0 , 0);

    CreateInfo.dwStyle = WS_VISIBLE | WS_BORDER | WS_CAPTION;
    CreateInfo.dwExStyle = WS_EX_NONE;
    CreateInfo.spCaption = "Capture";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = CaptureWinProc;
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
        setenv("MG_IAL_ENGINE", "auto", 1);
        setenv("MG_ENV_RECORD_IAL", "capture-input.dat", 1);
    }

    if (InitGUI (argc, argv) != 0) {
        return 1;
    }

    iRet = real_entry(argc, argv);
    exit(iRet);

    TerminateGUI(iRet);
    return iRet;
}

