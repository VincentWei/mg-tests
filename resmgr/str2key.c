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
** str2key.c:
**  Test code for Str2Key of MiniGUI 4.0.x.
**  The following APIs are covered:
**
**      Str2Key
**
** Copyright (C) 2019 FMSoft (http://www.fmsoft.cn).
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

#include "fnv.h"

int MiniGUIMain (int argc, const char* argv[])
{
    int i = 0;

#if SIZEOF_PTR == 8
    while (fnv1a_64_vector[i].test) {
        RES_KEY key = Str2Key(fnv1a_64_vector[i].test->buf);
        Fnv64_t hval = fnv_64a_str(fnv1a_64_vector[i].test->buf, FNV1A_64_INIT);
        if (key == hval) {
            print_fnv64(hval, (Fnv64_t)-1, 1, fnv1a_64_vector[i].test->buf);
        }
        else {
            _ERR_PRINTF("Bad Str2Key implementation: %s (%lx vs %lx)\n",
                    (char*)fnv1a_64_vector[i].test->buf,
                    key, hval);
            exit(1);
        }

        i++;
    }
#else
    while (fnv1a_32_vector[i].test) {
        RES_KEY key = Str2Key(fnv1a_32_vector[i].test->buf);
        Fnv32_t hval = fnv_32a_str(fnv1a_32_vector[i].test->buf, FNV1A_32_INIT);
        if (key == hval) {
            print_fnv32(hval, (Fnv32_t)-1, 1, fnv1a_32_vector[i].test->buf);
        }
        else {
            _ERR_PRINTF("Bad Str2Key implementation: %s (%lx vs %lx)\n",
                    (char*)fnv1a_32_vector[i].test->buf,
                    key, hval);
            exit(1);
        }

        i++;
    }
#endif

    return 0;
}

#ifdef _MGRM_THREADS
#include <minigui/dti.c>
#endif

