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
** exstubs.c: the default (nil) implementation of external stubs.
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

#if defined(__TARGET_EXTERNAL__) && defined(_MGGAL_DRM)

#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/exstubs.h>

DrmDriverOps* __drm_ex_driver_get(const char* driver_name)
{
    return NULL;
}

#endif /* defined(__TARGET_EXTERNAL__) && defined(_MGGAL_DRM) */
