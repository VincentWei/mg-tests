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
**  This program tests the following APIs for ISO8859-6 or ISO8859-8 text
**  (support for legacy BIDI implementation):
**
**      GetACharsExtent
**      GetTabbedTextExtent
**      GetTextExtentPoint
**      GetTabbedTextExtentPoint
**      GetNextUChar
**      IsUCharWide
**
**  The following APIs are also covered:
**
**      LoadDevFontFromFile
**      CreateLogFontByName
**      SelectFont
**      DestroyLogFont;
**      DestroyDynamicDevFont
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

#include <stdio.h>
#include <string.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#define UCHAR_TAB               0x0009
#define UCHAR_LF                0x000A
#define UCHAR_CR                0x000D

#define GLYPH_WIDTH   8
#define GLYPH_HEIGHT  16

static const char *normal_text_cases [] =
{
    "text: MiniGUI and HVML",
    "file:arabic/arabic_text.txt",
    "file:arabic/arabic_text2.txt",
};

static char *get_file_contents(const char *file, size_t *length)
{
    char *buf = NULL;
    FILE *f = fopen(file, "r");

    if (f) {
        if (fseek(f, 0, SEEK_END))
            goto failed;

        long len = ftell(f);
        if (len < 0)
            goto failed;

        buf = malloc(len + 1);
        if (buf == NULL)
            goto failed;

        fseek(f, 0, SEEK_SET);
        if (fread(buf, 1, len, f) < (size_t)len) {
            free(buf);
            buf = NULL;
        }
        buf[len] = '\0';

        if (length)
            *length = (size_t)len;
failed:
        fclose(f);
    }
    else {
        return NULL;
    }

    return buf;
}

static void test_normal_text(LOGFONT *lf)
{
    _MG_PRINTF("Testing GetACharsExtent(), and GetTextExtentPoint()...\n");

    for (int i = 0; i < TABLESIZE(normal_text_cases); i++) {
        char *buff = NULL;
        const char *mstr = NULL;
        size_t mstr_len = 0;
        if (strncmp("file:", normal_text_cases[i], 5) == 0) {
            buff = get_file_contents(normal_text_cases[i] + 5, &mstr_len);
            mstr = buff;
        }
        else if (strncmp("text:", normal_text_cases[i], 5) == 0) {
            mstr = normal_text_cases[i] + 5;
            mstr_len = strlen(mstr);
        }

        if (mstr == NULL || mstr_len == 0) {
            if (buff)
                free(buff);
            _WRN_PRINTF("Bad Case %d (failed to load): %s\n",
                    i, normal_text_cases[i]);
            continue;
        }

        Achar32 *achars = NULL;
        ACHARMAPINFO *achars_map = NULL;
        int nr_achars = BIDIGetTextVisualAChars(lf, mstr, mstr_len,
            &achars, &achars_map);
        if (nr_achars <= 0) {
            if (buff)
                free(buff);
            _WRN_PRINTF("Bad Case %d (no any visual character): %s\n",
                    i, normal_text_cases[i]);
            continue;
        }

        int glyph_width_exp[nr_achars];
        int pos_chars_exp[nr_achars];
        int dx_chars_exp[nr_achars];

        int ach = 0;
        int left = nr_achars;
        int extent_exp = 0;
        while (left > 0) {
            Uchar32 uc = AChar2UChar(lf, achars[ach]);

#if 0
            if (UCharIsArabicVowel(uc)) {
                glyph_width_exp[ach] = 0;
            }
            else
#endif
            if (IsUCharZeroWidth(uc)) {
                glyph_width_exp[ach] = 0;
            }
            else if (IsUCharWide(uc) || uc == UCHAR_LF) {
                glyph_width_exp[ach] = GLYPH_WIDTH * 2;
            }
            else
                glyph_width_exp[ach] = GLYPH_WIDTH;

            pos_chars_exp[ach] = achars_map[ach].byte_index;
            dx_chars_exp[ach] = extent_exp;

            extent_exp += glyph_width_exp[ach];

            ach++;
            left--;
        }

        _MG_PRINTF("Call GetACharsExtent for Case %d: %s (%d chars)\n",
                i, normal_text_cases[i], nr_achars);

        SIZE sz;
        GetACharsExtent(HDC_SCREEN, achars, nr_achars, &sz);
        if (sz.cx != extent_exp || sz.cy != GLYPH_HEIGHT) {
            _MG_PRINTF("Failed Case %d: expected (%d, %d), but returned (%d, %d)\n",
                    i, extent_exp, GLYPH_HEIGHT, sz.cx, sz.cy);
            exit(EXIT_FAILURE);
            return;
        }

        _MG_PRINTF("GetACharsExtent returned (%d, %d) for %d chars\n",
                sz.cx, sz.cy, nr_achars);

        free(achars);
        free(achars_map);

        int max_expect = extent_exp;
        int pos_chars[nr_achars];
        int dx_chars[nr_achars];

        _MG_PRINTF("Call GetTextExtentPoint for Case %d: %s\n",
                i, normal_text_cases[i]);

        for (int max_extent = max_expect; max_extent > 0;
                max_extent -= (GLYPH_WIDTH >> 1)) {

            _MG_PRINTF("\tmax_extent: %d\n", max_extent);

            extent_exp = 0;
            int fit_chars_exp = 0;
            while (fit_chars_exp < nr_achars) {
                if (extent_exp + glyph_width_exp[fit_chars_exp] <= max_extent) {
                    extent_exp += glyph_width_exp[fit_chars_exp];
                    fit_chars_exp++;
                }
                else
                    break;
            }

            int fit_chars;
            int n = GetTextExtentPoint(HDC_SCREEN, mstr, mstr_len,
                    max_extent, &fit_chars, pos_chars, dx_chars, &sz);
            _MG_PRINTF("\tGetTextExtentPoint returned: %d\n", n);
            if (fit_chars != fit_chars_exp) {
                _MG_PRINTF("Failed Case %d: expected fit_chars (%d/%d), but returned (%d, %d)\n",
                        i, fit_chars_exp, nr_achars, fit_chars, sz.cx);
                exit(EXIT_FAILURE);
                return;
            }

            if (sz.cx != extent_exp || sz.cy != GLYPH_HEIGHT) {
                _MG_PRINTF("Failed Case %d: expected (%d, %d), but returned (%d, %d)\n",
                        i, extent_exp, GLYPH_HEIGHT, sz.cx, sz.cy);
                exit(EXIT_FAILURE);
                return;
            }

            for (int j = 0; j < fit_chars; j++) {
                if (pos_chars[j] != pos_chars_exp[j]) {
                    _MG_PRINTF("Failed Case %d: expected pos_chars[%d]: %d, but returned: %d\n",
                            i, j, pos_chars_exp[j], pos_chars[j]);
                    exit(EXIT_FAILURE);
                    return;
                }

                if (dx_chars[j] != dx_chars_exp[j]) {
                    _MG_PRINTF("Failed Case %d: expected dx_chars[%d]: %d, but returned: %d\n",
                            i, j, dx_chars_exp[j], dx_chars[j]);
                    exit(EXIT_FAILURE);
                    return;
                }
            }

        }

        if (buff)
            free(buff);
    }

    _MG_PRINTF("Testing for GetACharsExtent() and GetTextExtentPoint() passed!\n");
}

static const char *tabbed_text_cases [] =
{
    "text:MiniGUI\tHVML",
    "file:arabic/arabic_text.txt",
    "file:arabic/arabic_text2.txt",
};

#if 0
static int find_line_break(LOGFONT *lf, const Achar32 *achars, int nr_achars)
{
    int n = 0;

    for (int i = 0; i < nr_achars; i++) {

        n++;
        Uint32 achar_type = GetACharBidiType(lf, achars[i]);
        Uint32 type = achar_type & ACHARTYPE_BASIC_MASK;
        if (type == ACHAR_BASIC_LF || type == ACHAR_BASIC_CR)
            break;
    }

    return n;
}
#endif

struct achar_info {
    int pos;
    int dx;
    int dy;
    int ax;
    int ay;
};

static SIZE get_tabbed_achars_extent(LOGFONT *lf,
        Achar32 *achars, ACHARMAPINFO *achars_map, int nr_achars,
        struct achar_info *info)
{
    int ach = 0;
    int last_line_width = 0;
    SIZE sz = { };

    int left = nr_achars;
    while (left > 0) {
        info[ach].pos = achars_map[ach].byte_index;
        info[ach].dx = sz.cx = last_line_width;
        info[ach].dy = sz.cy;

        Uchar32 uc = AChar2UChar(lf, achars[ach]);
        int adv_x = 0, adv_y = 0;
#if 0
        if (UCharIsArabicVowel(uc)) {
            adv_x = 0;
            adv_y = 0;
        }
        else 
#endif
        if (IsUCharZeroWidth(uc)) {
            adv_x = 0;
            adv_y = 0;
        }
        else if (uc == UCHAR_CR) {
            adv_x = 0;
            if (last_line_width > sz.cx) {
                sz.cx = last_line_width;
            }
            last_line_width = 0;
        }
        else if (uc == UCHAR_LF) {
            adv_x = 0;
            adv_y = GLYPH_HEIGHT;
            if (last_line_width > sz.cx) {
                sz.cx = last_line_width;
            }
            last_line_width = 0;
        }
        else if (uc == UCHAR_TAB) {
            adv_x = GLYPH_WIDTH * 8;
            last_line_width += adv_x;
        }
        else if (IsUCharWide(uc)) {
            adv_x = GLYPH_WIDTH * 2;
            adv_y = 0;
            last_line_width += adv_x;
        }
        else {
            adv_x = GLYPH_WIDTH;
            last_line_width += adv_x;
        }

        info[ach].ax = adv_x;
        info[ach].ay = adv_y;

        sz.cx += adv_x;
        sz.cy += adv_y;
        if (last_line_width > sz.cx) {
            sz.cx = last_line_width;
        }

        ach++;
        left--;
    }

    sz.cy += GLYPH_HEIGHT;
    return sz;
}

static void test_tabbed_text(LOGFONT *lf)
{
    _MG_PRINTF("Testing GetTabbedTextExtent(), and GetTabbedTextExtentPoint()...\n");

    for (int i = 0; i < TABLESIZE(tabbed_text_cases); i++) {
        char *buff = NULL;
        const char *mstr = NULL;
        size_t mstr_len = 0;
        if (strncmp("file:", tabbed_text_cases[i], 5) == 0) {
            buff = get_file_contents(tabbed_text_cases[i] + 5, &mstr_len);
            mstr = buff;
        }
        else if (strncmp("text:", tabbed_text_cases[i], 5) == 0) {
            mstr = tabbed_text_cases[i] + 5;
            mstr_len = strlen(mstr);
        }

        if (mstr == NULL || mstr_len == 0) {
            if (buff)
                free(buff);
            _WRN_PRINTF("Bad case %d (failed to load): %s\n",
                    i, tabbed_text_cases[i]);
            continue;
        }

        Achar32 achars_buf[mstr_len];
        Achar32 *achars = achars_buf;
        ACHARMAPINFO achars_map_buf[mstr_len];
        ACHARMAPINFO *achars_map = achars_map_buf;
        int nr_achars = BIDIGetTextVisualAChars(lf, mstr, mstr_len,
                &achars, &achars_map);

        struct achar_info achar_info[mstr_len];
        SIZE size_exp = get_tabbed_achars_extent(lf,
                achars, achars_map, nr_achars, achar_info);

        _MG_PRINTF("Call GetTabbedTextExtentPoint for Case %d: %s(%d): %d\n",
                i, tabbed_text_cases[i], nr_achars, size_exp.cx);

        for (int max_extent = size_exp.cx; max_extent > 0;
                max_extent -= (GLYPH_WIDTH >> 1)) {

            _MG_PRINTF("\tmax_extent: %d\n", max_extent);

            int cx_exp = 0;
            int cy_exp = 0;
            int fit_chars_exp = 0;
            while (fit_chars_exp < nr_achars) {
                int extent = achar_info[fit_chars_exp].dx +
                    achar_info[fit_chars_exp].ax;
                if (extent <= max_extent) {
                    cx_exp = extent;
                    cy_exp = achar_info[fit_chars_exp].dy +
                        achar_info[fit_chars_exp].ay;
                    fit_chars_exp++;
                }
                else
                    break;
            }
            cy_exp += GLYPH_HEIGHT;

            int pos_chars[nr_achars];
            int dx_chars[nr_achars];
            SIZE size;
            int fit_chars;
            int n = GetTabbedTextExtentPoint(HDC_SCREEN, mstr, mstr_len,
                    max_extent, &fit_chars, pos_chars, dx_chars, &size);
            _MG_PRINTF("\tGetTabbedTextExtentPoint returned: %d\n", n);
            if (fit_chars != fit_chars_exp) {
                _MG_PRINTF("Failed Case %d: expected fit_chars (%d/%d), but returned (%d, %d)\n",
                        i, fit_chars_exp, nr_achars, fit_chars, size.cx);
                exit(EXIT_FAILURE);
                return;
            }

            if (size.cx != cx_exp || size.cy != cy_exp) {
                _MG_PRINTF("Failed Case %d: expected (%d, %d), but returned (%d, %d)\n",
                        i, cx_exp, cy_exp, size.cx, size.cy);
                exit(EXIT_FAILURE);
                return;
            }

            for (int j = 0; j < fit_chars; j++) {
                if (pos_chars[j] != achar_info[j].pos) {
                    _MG_PRINTF("Failed Case %d: expected pos_chars[%d]: %d, but returned: %d\n",
                            i, j, achar_info[j].pos, pos_chars[j]);
                    exit(EXIT_FAILURE);
                    return;
                }

                if (dx_chars[j] != achar_info[j].dx) {
                    _MG_PRINTF("Failed Case %d: expected dx_chars[%d]: %d, but returned: %d\n",
                            i, j, achar_info[j].dx, dx_chars[j]);
                    exit(EXIT_FAILURE);
                    return;
                }
            }

        }

        if (buff)
            free(buff);
    }

    _MG_PRINTF("Testing for GetTabbedTextExtent() and GetTabbedTextExtentPoint() passed!\n");
}

#define UPF_DEVFONTFILE "/usr/local/share/minigui/res/font/unifont_160_50.upf"
#define UPF_DEVFONTNAME "upf-unifont,SansSerif,monospace-rrncnn-8-16-ISO8859-6,UTF-8"

#define RBF_DEVFONTFILE "/usr/local/share/minigui/res/font/8x16-iso8859-1.bin"
#define RBF_DEVFONTNAME "rbf-fixed,SansSerif,monospace-rrncnn-8-16-ISO8859-1"

#define LOGFONTNAME "upf-monospace-rrncnn-U-16-ISO8859-6"

int MiniGUIMain(int argc, const char* argv[])
{
    (void)argv;

    _MG_PRINTF("Starting test %s...\n", argv[0]);

    DEVFONT *upf_devfont = NULL, *rbf_devfont = NULL;
    if ((upf_devfont = LoadDevFontFromFile(UPF_DEVFONTNAME, UPF_DEVFONTFILE)) == NULL) {
        _ERR_PRINTF("%s: Failed to load devfont(%s) from %s\n",
                __func__, UPF_DEVFONTNAME, UPF_DEVFONTFILE);
        goto failed;
    }

    if ((rbf_devfont = LoadDevFontFromFile(RBF_DEVFONTNAME, RBF_DEVFONTFILE)) == NULL) {
        _ERR_PRINTF("%s: Failed to load devfont(%s) from %s\n",
                __func__, RBF_DEVFONTNAME, RBF_DEVFONTFILE);
        goto failed;
    }

    LOGFONT *logfont = NULL;
    if ((logfont = CreateLogFontByName(LOGFONTNAME)) == NULL) {
        _ERR_PRINTF("%s: Failed to create logfont (%s)\n",
                __func__, LOGFONTNAME);
        goto failed;
    }

    LOGFONT *old_logfont = SelectFont(HDC_SCREEN, logfont);
    SetBIDIFlags(HDC_SCREEN, BIDI_FLAG_LEGACY);

    test_normal_text(logfont);
    test_tabbed_text(logfont);

    SelectFont(HDC_SCREEN, old_logfont);
    DestroyLogFont(logfont);
    DestroyDynamicDevFont(&upf_devfont);
    DestroyDynamicDevFont(&rbf_devfont);

    _MG_PRINTF ("%s passed!\n", argv[0]);
    exit(EXIT_SUCCESS);
    return 0;

failed:
    if (logfont)
        DestroyLogFont(logfont);
    if (upf_devfont)
        DestroyDynamicDevFont(&upf_devfont);
    if (rbf_devfont)
        DestroyDynamicDevFont(&rbf_devfont);

    exit(EXIT_FAILURE);
    return 1;
}

