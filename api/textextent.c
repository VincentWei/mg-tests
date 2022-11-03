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
**  This program tests the following APIs:
**
**      GetTextExtent
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

struct test_case {
    const char *text;
    int expect;
};

#define GLYPH_WIDTH   8
#define GLYPH_HEIGHT  16

static struct test_case normal_text_cases [] =
{
    {
        "  MiniGUI and HVML",
        0,
    },
    {
        "  MiniGUI（迷你龟）5.0；版权所有 2002 ~ 2022 飞漫软件",
        0,
    },
};

static void test_normal_text(LOGFONT *lf)
{
    _MG_PRINTF("Testing GetTextExtent(), and GetTextExtentPoint()...\n");

    for (int i = 0; i < TABLESIZE(normal_text_cases); i++) {
        const char *mstr = normal_text_cases[i].text;
        int mstr_len = (int)strlen(mstr);

        _MG_PRINTF("Call GetTextExtent for Case %d: %s\n", i, mstr);

        int glyph_width_exp[mstr_len];
        int pos_chars_exp[mstr_len];
        int dx_chars_exp[mstr_len];

        const char *mchar = mstr;
        int left = mstr_len;
        int nr_ucs = 0;
        int extent_exp = 0;
        while (left > 0) {
            Uchar32 uc;
            int consumed = GetNextUChar(lf, mchar, left, &uc);
            if (consumed <= 0)
                break;

            if (IsUCharWide(uc)) {
                glyph_width_exp[nr_ucs] = GLYPH_WIDTH * 2;
            }
            else
                glyph_width_exp[nr_ucs] = GLYPH_WIDTH;

            pos_chars_exp[nr_ucs] = mchar - mstr;
            dx_chars_exp[nr_ucs] = extent_exp;

            extent_exp += glyph_width_exp[nr_ucs];
            left -= consumed;
            mchar += consumed;
            nr_ucs++;
        }

        SIZE sz;
        GetTextExtent(HDC_SCREEN, mstr, mstr_len, &sz);
        if (sz.cx != extent_exp || sz.cy != GLYPH_HEIGHT) {
            _MG_PRINTF("Failed Case %d: expected (%d, %d), but returned (%d, %d)\n",
                    i, extent_exp, GLYPH_HEIGHT, sz.cx, sz.cy);
            exit(EXIT_FAILURE);
            return;
        }

        normal_text_cases[i].expect = extent_exp;

        int pos_chars[nr_ucs];
        int dx_chars[nr_ucs];

        _MG_PRINTF("Call GetTextExtentPoint for Case %d: %s\n", i, mstr);

        for (int max_extent = normal_text_cases[i].expect; max_extent > 0;
                max_extent -= (GLYPH_WIDTH >> 1)) {

            _MG_PRINTF("\tmax_extent: %d\n", max_extent);

            extent_exp = 0;
            int fit_chars_exp = 0;
            while (fit_chars_exp < nr_ucs) {
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
                        i, fit_chars_exp, nr_ucs, fit_chars, sz.cx);
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

    }

    _MG_PRINTF("Testing for GetTextExtent() and GetTextExtentPoint() passed!\n");
}

static struct test_case tabbed_text_cases [] =
{
    {
        "MiniGUI",
        0,
    },
    {
        "MiniGUI（迷你龟）5.0；版权所有 2002 ~ 2022 飞漫软件",
        0,
    },
};

static void test_tabbed_text(LOGFONT *lf)
{
    _MG_PRINTF("Testing GetTabbedTextExtent(), and GetTabbedTextExtentPoint()...\n");

    for (int i = 0; i < TABLESIZE(tabbed_text_cases); i++) {
        const char *mstr = tabbed_text_cases[i].text;
        int mstr_len = (int)strlen(mstr);

        _MG_PRINTF("Call GetTabbedTextExtent for Case %d: %s\n", i, mstr);

        int glyph_width_exp[mstr_len];
        int pos_chars_exp[mstr_len];
        int dx_chars_exp[mstr_len];

        const char *mchar = mstr;
        int left = mstr_len;
        int nr_ucs = 0;
        int extent_exp = 0;
        while (left > 0) {
            Uchar32 uc;
            int consumed = GetNextUChar(lf, mchar, left, &uc);
            if (consumed <= 0)
                break;

            if (IsUCharWide(uc)) {
                glyph_width_exp[nr_ucs] = GLYPH_WIDTH * 2;
            }
            else
                glyph_width_exp[nr_ucs] = GLYPH_WIDTH;

            pos_chars_exp[nr_ucs] = mchar - mstr;
            dx_chars_exp[nr_ucs] = extent_exp;

            extent_exp += glyph_width_exp[nr_ucs];
            left -= consumed;
            mchar += consumed;
            nr_ucs++;
        }

        SIZE sz;
        GetTabbedTextExtent(HDC_SCREEN, mstr, mstr_len, &sz);
        if (sz.cx != extent_exp || sz.cy != GLYPH_HEIGHT) {
            _MG_PRINTF("Failed Case %d: expected (%d, %d), but returned (%d, %d)\n",
                    i, extent_exp, GLYPH_HEIGHT, sz.cx, sz.cy);
            exit(EXIT_FAILURE);
            return;
        }

        tabbed_text_cases[i].expect = extent_exp;

        int pos_chars[nr_ucs];
        int dx_chars[nr_ucs];

        _MG_PRINTF("Call GetTabbedTextExtentPoint for Case %d: %s\n", i, mstr);

        for (int max_extent = tabbed_text_cases[i].expect; max_extent > 0;
                max_extent -= (GLYPH_WIDTH >> 1)) {

            _MG_PRINTF("\tmax_extent: %d\n", max_extent);

            extent_exp = 0;
            int fit_chars_exp = 0;
            while (fit_chars_exp < nr_ucs) {
                if (extent_exp + glyph_width_exp[fit_chars_exp] <= max_extent) {
                    extent_exp += glyph_width_exp[fit_chars_exp];
                    fit_chars_exp++;
                }
                else
                    break;
            }

            int fit_chars;
            int n = GetTabbedTextExtentPoint(HDC_SCREEN, mstr, mstr_len,
                    max_extent, &fit_chars, pos_chars, dx_chars, &sz);
            _MG_PRINTF("\tGetTextExtentPoint returned: %d\n", n);
            if (fit_chars != fit_chars_exp) {
                _MG_PRINTF("Failed Case %d: expected fit_chars (%d/%d), but returned (%d, %d)\n",
                        i, fit_chars_exp, nr_ucs, fit_chars, sz.cx);
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

    }

    _MG_PRINTF("Testing for GetTabbedTextExtent() and GetTabbedTextExtentPoint() passed!\n");
}

#define DEVFONTFILE "/usr/local/share/minigui/res/font/unifont_160_50.upf"
#define DEVFONTNAME "upf-unifont,SansSerif,monospace-rrncnn-8-16-ISO8859-1,ISO8859-6,ISO8859-8,UTF-8"

#define LOGFONTNAME "upf-monospace-rrncnn-U-16-UTF-8"

int MiniGUIMain(int argc, const char* argv[])
{
    (void)argv;

    _MG_PRINTF("Starting test %s...\n", argv[0]);

    DEVFONT* devfont = NULL;
    if ((devfont = LoadDevFontFromFile(DEVFONTNAME, DEVFONTFILE)) == NULL) {
        _ERR_PRINTF("%s: Failed to load devfont(%s) from %s\n",
                __func__, DEVFONTNAME, DEVFONTFILE);
        goto failed;
    }

    LOGFONT *logfont = NULL;
    if ((logfont = CreateLogFontByName(LOGFONTNAME)) == NULL) {
        _ERR_PRINTF("%s: Failed to create logfont (%s)\n",
                __func__, LOGFONTNAME);
        goto failed;
    }

    LOGFONT *old_logfont = SelectFont(HDC_SCREEN, logfont);

    test_normal_text(logfont);
    test_tabbed_text(logfont);

    SelectFont(HDC_SCREEN, old_logfont);
    DestroyLogFont(logfont);
    DestroyDynamicDevFont(&devfont);

    _MG_PRINTF ("%s passed!\n", argv[0]);
    exit(EXIT_SUCCESS);
    return 0;

failed:
    if (logfont)
        DestroyLogFont(logfont);
    if (devfont)
        DestroyDynamicDevFont(&devfont);

    exit(EXIT_FAILURE);
    return 1;
}

