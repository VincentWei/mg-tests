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
** global.h:
**  Global definitions for test program of shared surface.
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

#ifndef _MG_TESTS_SHARED_SURFACE_GLOBAL_H
    #define _MG_TESTS_SHARED_SURFACE_GLOBAL_H

#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#define REQID_PRODUCER_NAMED_READY  (MAX_SYS_REQID + 1)
#define REQID_PRODUCER_CLWIN_READY  (MAX_SYS_REQID + 2)
#define SHARED_SURFACE_READY        "ready"
#define SHARED_SURFACE_NAME         "background"
#define CONSUMER_ARG_PATTERN        "%s %d %p"

#define PRODUCER_NAMED_WIDTH        800
#define PRODUCER_NAMED_HEIGHT       600
#define PRODUCER_CLWIN_WIDTH        320
#define PRODUCER_CLWIN_HEIGHT       200

#define CONSUMER_WIDTH              640
#define CONSUMER_HEIGHT             400

struct producer_clwin_info {
    int     cli;
    HWND    hwnd;
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _MG_TESTS_SHARED_SURFACE_GLOBAL_H */

