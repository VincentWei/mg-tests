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
**      DrawText
**
**  The following APIs are also covered:
**
**      LoadDevFontFromFile
**      CreateLogFontByName
**      SelectFont
**      DestroyLogFont;
**      DestroyDynamicDevFont
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

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

struct test_case {
    const char *lang;
    const char *text;
};

static int compare_file_with_checked(const char *filename)
{
    char checked[128];
    sprintf(checked, "checked/%s", filename);

    FILE *fp_to_test = NULL, *fp_checked = NULL;
    fp_checked = fopen(checked, "r");
    if (fp_checked) {
        char buf_to_test[4096];
        char buf_checked[4096];

        fp_to_test = fopen(filename, "r");
        assert(fp_to_test);

        while (1) {
            size_t n1 = fread(buf_to_test, 1, sizeof(buf_to_test), fp_to_test);
            size_t n2 = fread(buf_checked, 1, sizeof(buf_checked), fp_checked);

            if (n1 != n2)
                goto not_matched;

            if (memcmp(buf_to_test, buf_checked, n1))
                goto not_matched;

            if (n1 < sizeof(buf_to_test))
                break;
        }

        fclose(fp_to_test);
        fclose(fp_checked);

        remove(filename);
    }

    return 0;

not_matched:
    if (fp_to_test)
        fclose(fp_to_test);
    if (fp_checked)
        fclose(fp_checked);
    return 1;
}

static int uc32_to_utf8(Uchar32 c, char* outbuf)
{
    int len = 0;
    int first;
    int i;

    if (c < 0x80) {
        first = 0;
        len = 1;
    }
    else if (c < 0x800) {
        first = 0xc0;
        len = 2;
    }
    else if (c < 0x10000) {
        first = 0xe0;
        len = 3;
    }
    else if (c < 0x200000) {
        first = 0xf0;
        len = 4;
    }
    else if (c < 0x4000000) {
        first = 0xf8;
        len = 5;
    }
    else {
        first = 0xfc;
        len = 6;
    }

    if (outbuf) {
        for (i = len - 1; i > 0; --i) {
            outbuf[i] = (c & 0x3f) | 0x80;
            c >>= 6;
        }
        outbuf[0] = c | first;
    }

    return len;
}

static int test_draw_text(HDC hdc, PLOGFONT logfont,
        const struct test_case *test_case)
{
    RECT rc = {0, 0, 100, 200};

    const char *mchar = test_case->text;
    int left = strlen(mchar);
    int nr_ucs = 0;
    while (left > 0) {
        Uchar32 uc;
        int consumed = GetNextUChar(logfont, mchar, left, &uc);
        if (consumed <= 0)
            break;

        char utf8[16];
        utf8[uc32_to_utf8(uc, utf8)] = 0;

        UCharGeneralCategory gc = UCharGetCategory(uc);
        UCharBreakType bt = UCharGetBreakType(uc);
        printf("Char #%d `%s` (0x%04x): gc (%d), bt (%d)\n",
                nr_ucs, utf8, uc, gc, bt);

        nr_ucs++;
        mchar += consumed;
        left -= consumed;
    }

    for (int i = 0; i < 10; i++) {
        char filename[32];

        FillBox(hdc, rc.left, rc.top, rc.right, rc.bottom);
        DrawText(hdc, test_case->text, -1, &rc, DT_CENTER);

        sprintf(filename, "drawtext-%s-%d.bmp", test_case->lang, i);
        if (!SaveScreenRectContent(&rc, filename)) {
            _WRN_PRINTF("Failed SaveScreenRectContent(%s); quit\n", filename);
            return 0;
        }

        if (compare_file_with_checked(filename)) {
            printf("Not matched case: %s\n", filename);
            return 1;
        }

        rc.right += 20;
    }

    return 0;
}

static struct test_case test_cases[] = {
    { "thai", "วันเสาร์" },
    { "korean", "비밀번호 재설정 기능이 잠겼습니다.....30A1분 후 시도하십시오." },
    { "chinese", "飞漫软件是中国最早基于自主开发的开源软件进行商业化运营的基础软件企业。飞漫软件现主持着两大开源项目：MiniGUI 广泛应用于 IoT 智能设备及实时嵌入式系统，为 IoT 智能设备和嵌入式系统提供 GUI 及交互实现；HybridOS 是飞漫软件发起的一个的开源协作项目，其目标是为物联网设备和云计算环境打造一个全新的操作系统。" },
};

#define UPF_DEVFONTFILE "/usr/local/share/minigui/res/font/unifont_160_50.upf"
#define UPF_DEVFONTNAME "upf-unifont,SansSerif,monospace-rrncnn-8-16-UTF-8"

#define RBF_DEVFONTFILE "/usr/local/share/minigui/res/font/8x16-iso8859-1.bin"
#define RBF_DEVFONTNAME "rbf-fixed,SansSerif,monospace-rrncnn-8-16-ISO8859-1"

#define LOGFONTNAME "upf-monospace-rrncnn-U-16-UTF-8"

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

    SetBkMode(HDC_SCREEN, BM_TRANSPARENT);
    LOGFONT *old_logfont = SelectFont(HDC_SCREEN, logfont);

    BOOL passed = TRUE;
    for (size_t i = 0; i < TABLESIZE(test_cases); i++) {
        if (test_draw_text(HDC_SCREEN, logfont, test_cases + i)) {
            passed = FALSE;
            break;
        }
    }

    SelectFont(HDC_SCREEN, old_logfont);
    DestroyLogFont(logfont);
    DestroyDynamicDevFont(&upf_devfont);
    DestroyDynamicDevFont(&rbf_devfont);

    if (passed) {
        _MG_PRINTF ("%s passed!\n", argv[0]);
        exit(EXIT_SUCCESS);
    }
    else {
        _MG_PRINTF ("%s failed!\n", argv[0]);
        exit(EXIT_FAILURE);
    }

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

