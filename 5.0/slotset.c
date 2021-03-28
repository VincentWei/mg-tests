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
**  Test code of z-order for MiniGUI 5.0.
**
**  This test program tests the internal functions for slot set operations.
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

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <minigui/common.h>

size_t __mg_lookfor_unused_slot (unsigned char* bitmap, size_t len_bmp, int set);

static inline
void __mg_slot_set_use (unsigned char* bitmap, size_t index) {
    bitmap += index >> 3;
    *bitmap &= (~(0x80 >> (index % 8)));
}

static inline
int __mg_slot_clear_use (unsigned char* bitmap, size_t index) {
    bitmap += index >> 3;
    *bitmap |= (0x80 >> (index % 8));
    return 1;
}

static void test_mg_slot (void)
{
    size_t i, slot;
    unsigned char usage_bmp [8];

    memset (usage_bmp, 0xFF, sizeof (usage_bmp));
    for (i = 0; i < 64; i++) {
        slot = __mg_lookfor_unused_slot (usage_bmp, sizeof (usage_bmp), 1);
        assert (slot == i);
    }

    slot = __mg_lookfor_unused_slot (usage_bmp, sizeof (usage_bmp), 1);
    assert (slot == -1);

    for (i = 63; i > 0; i--) {
        __mg_slot_clear_use (usage_bmp, i);
        __mg_slot_set_use (usage_bmp, i);
        slot = __mg_lookfor_unused_slot (usage_bmp, sizeof (usage_bmp), 1);
        assert (slot == -1);
    }

    for (i = 0; i > 64; i--) {
        __mg_slot_clear_use (usage_bmp, i);
        slot = __mg_lookfor_unused_slot (usage_bmp, sizeof (usage_bmp), 1);
        assert (slot == i);
    }
}

int main (int argc, const char* argv[])
{
    test_mg_slot ();

    _MG_PRINTF ("Test for slot set passed!\n");
    return 0;
}

