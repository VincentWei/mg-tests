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
** wallpaper-welcome.c: a MiniGUI 5.0 wallpaper renderer.
**
** Copyright (C) 2019 ~ 2020 FMSoft (http://www.fmsoft.cn).
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
#include <time.h>
#include <sys/times.h>
#include <assert.h>
#include <string.h>

#include <mgeff/mgeff.h>

#define BLK_SIZE 2

typedef struct {
    int x;
    int y;
    int color;
} star_t;

typedef struct {
    int n;
    star_t *star;
} letter_t;

static letter_t *s_letters;
static int s_nrLetters;
static star_t *s_allstar;
static int s_nrStars;

static inline int star_cmp(star_t *s1, star_t *s2) {
    if (s1->y != s2->y) {
        return (-s1->y) - (-s2->y);
    }
    else{
        return s1->x - s2->x;
    }
}

extern const char *g_lines[];

static void gen_letters (void)
{
    int line;
    int i;

    // count letters
    for (line=0; g_lines[line]; line++) {
        if (strcmp(g_lines[line], "----") == 0) {
            s_nrLetters ++;
        }
    }

    _DBG_PRINTF ("total letters: %d\n", s_nrLetters);

    s_letters = (letter_t *)calloc(s_nrLetters, sizeof(letter_t));

    // count stars
    {
        int index=0;
        for (line=0; g_lines[line]; line++) {
            int i;
            if (strcmp(g_lines[line], "----") == 0) {
                _DBG_PRINTF ("number of stars for line %d: %d\n",
                        index, s_letters[index].n);
                index++;
                continue;
            }
            for (i=0; g_lines[line][i]; i++) {
                if (g_lines[line][i] != ' ') {
                    s_letters[index].n++;
                }
            }

        }
    }

    // record star
    {
        int index=0;
        int nStar;
        // malloc
        for (index=0; index<s_nrLetters; index++) {
            if (s_letters[index].n > 0)
                s_letters[index].star =
                    (star_t *)calloc(s_letters[index].n, sizeof(star_t));
            else
                s_letters[index].star = NULL;

            s_nrStars += s_letters[index].n;
        }

        assert (s_nrStars);
        s_allstar = (star_t *)calloc(s_nrStars, sizeof(star_t));

        index = 0;
        nStar = 0;
        for (line=0; g_lines[line]; line++) {
            if (strcmp(g_lines[line], "----") == 0) {
                index++;
                nStar=0;
                continue;
            }
            for (i=0; g_lines[line][i]; i++) {
                if (g_lines[line][i] != ' '){
                    assert(nStar < s_letters[index].n);
                    s_letters[index].star[nStar].x =
                        120 + line * BLK_SIZE;
                    s_letters[index].star[nStar].y =
                        550 - i*BLK_SIZE;
                    s_letters[index].star[nStar].color = MakeRGBA(
                            0xf0, 0xf0, 0xf0, 0xff);
                    nStar++;
                }
            }
        }
    }

    // sort
#if 0
    for (i=0; i<s_nrLetters; i++) {
        qsort(s_letters[i].star, s_letters[i].n, sizeof(s_letters[i].star[0]),
                (void *)star_cmp);
    }
#else   /* only for the special letter */
    qsort (s_letters[7].star, s_letters[7].n, sizeof(s_letters[7].star[0]),
            (void *)star_cmp);
#endif

#if 0
    // print
    for (i=0; i<s_nrLetters; i++) {
        int j;
        for (j=0; j<s_letters[i].n; j++) {
            printf("%d, %d\n",
                    s_letters[i].star[j].x, s_letters[i].star[j].y);
        }
    }
#endif
}

static void paint_all_star (HDC dc)
{
    RECT rc = GetScreenRect ();
    int i;

    SetBrushColor(dc, RGB2Pixel(dc, 0x01, 0x1d, 0x2e));
    FillBox(dc, rc.left, rc.top, RECTW(rc), RECTH(rc));

    for (i=0; i<s_nrStars; i++) {
        star_t *star = &s_allstar[i];
        int color;
        if (star->x >= rc.left && star->x < rc.right
                && star->y >= rc.top && star->y < rc.bottom) {
            color = star->color;

            SetBrushColor(dc, RGB2Pixel(dc,
                        GetRValue(color),
                        GetGValue(color),
                        GetBValue(color)));
            FillBox(dc, star->x, star->y, BLK_SIZE-1, BLK_SIZE-1);
        }
    }
}

static void moveStar(MGEFF_ANIMATION animation, star_t *star, int id, POINT *point)
{
    static HDC memdc;
    int real_id = id;

    if (!memdc) {
        memdc = CreateCompatibleDC (HDC_SCREEN);
    }

    if (real_id < 0) {
        real_id = -real_id;
    }

    s_allstar[real_id].x = point->x;
    s_allstar[real_id].y = point->y;

    if (id < 0) {
        paint_all_star (memdc);
        BitBlt(memdc, 0, 0, 0, 0, HDC_SCREEN, 0, 0, -1);
        SyncUpdateDC (HDC_SCREEN);
    }
}

static void play_animations(void)
{
    MGEFF_ANIMATION sequence_group;
    int i;
    int id=0;
    int w, h;
    RECT win_rc = GetScreenRect ();

    w = RECTW(win_rc);
    h = RECTH(win_rc);
    sequence_group = mGEffAnimationCreateGroup(MGEFF_SEQUENTIAL);

    for (i=0; i<s_nrLetters; i++) {
        int j;
        letter_t *letter = &s_letters[i];
        MGEFF_ANIMATION parallel_group;

        parallel_group = mGEffAnimationCreateGroup(MGEFF_PARALLEL);

        for (j=0; j<letter->n; j++) {
            MGEFF_ANIMATION animation;
            POINT point;
            animation = mGEffAnimationCreate(&letter->star[j], (void *)moveStar,
                    j<letter->n-1 ? id : -id, MGEFF_POINT);
            // duration
            switch (i) {
                case 0: // M
                    mGEffAnimationSetDuration(animation, 100);
                    break;
                case 1: // i
                case 2: // n
                case 3: // i
                case 4: // G
                    mGEffAnimationSetDuration(animation, 600);
                    break;
                case 5: // U
                case 6: // I
                    mGEffAnimationSetDuration(animation, 1000-i*50);
                    break;
                case 7: // 5
                    mGEffAnimationSetDuration(animation, 100 + j);
                    break;
                case 8: // .
                case 9: // 0
                default:
                    mGEffAnimationSetDuration(animation, 500 + j*6);
                    break;
            }

            // start value
            switch (i) {
                case 0: // M
                    point.x = win_rc.left = 5;
                    point.y = letter->star[j].y;
                    break;
                case 1: // i
                case 2: // n
                case 3: // i
                case 4: // G
                    point.x = letter->star[j].x;
                    point.y = letter->star[j].y + 350;
                    break;
                case 5: // U
                case 6: // I
                case 7: // 5
                    point.x = letter->star[j].x;
                    point.y = letter->star[j].y - 600;
                    break;
                case 8: // .
                case 9: // 0
                default:
                    {
                        int x, y;
                        x = (rand() % (2*w)) - w;
                        y = (rand() % (2*h)) - h;
                        if (x > 0 && y > 0) {
                            point.x = win_rc.right;
                            point.y = y;
                        }
                        else if (x > 0 && y < 0) {
                            point.x = x;
                            point.y = win_rc.top - BLK_SIZE;
                        }
                        else if (x < 0 && y > 0) {
                            point.x = win_rc.left - BLK_SIZE - 1;
                            point.y = y;
                        }
                        else {
                            point.x = -x;
                            point.y = win_rc.bottom;
                        }
                    }
                    break;
            }

            // curve
            switch (i) {
                case 0: // M
                    mGEffAnimationSetCurve(animation, InCubic);
                    break;
                case 1: // i
                    mGEffAnimationSetCurve(animation, OutBounce);
                    break;
                case 2: // n
                    mGEffAnimationSetCurve(animation, OutQuad);
                    break;
                case 3: // i
                    mGEffAnimationSetCurve(animation, InBounce);
                    break;
                case 4: // G
                    mGEffAnimationSetCurve(animation, OutCubic);
                    break;
                case 5: // U
                    mGEffAnimationSetCurve(animation, InQuad);
                    break;
                case 6: // I
                    mGEffAnimationSetCurve(animation, OutInBounce);
                    break;
                case 7: // 5
                    mGEffAnimationSetCurve(animation, InBounce);
                    break;
                case 8: // .
                case 9: // 0
                default:
                    mGEffAnimationSetCurve(animation, OutBounce);
                    break;
            }

            // color
            if (i < 7) {
                s_allstar[id].color = letter->star[j].color;
            }
            else {
                s_allstar[id].color = MakeRGBA (0xe8, 0xd1, 0x06, 0xff);
            }
            
            s_allstar[id].x = point.x;
            s_allstar[id].y = point.y;
            mGEffAnimationSetStartValue(animation, &point);
            point.x = letter->star[j].x;
            point.y = letter->star[j].y;
            mGEffAnimationSetEndValue(animation, &point);
            mGEffAnimationAddToGroup(parallel_group, animation);

            id ++;
        }
        mGEffAnimationAddToGroup(sequence_group, parallel_group);
    }
    mGEffAnimationSyncRun(sequence_group);
    mGEffAnimationDelete (sequence_group);
}

int MiniGUIMain (int argc, const char *argv[])
{
    MSG msg;

#ifdef _MGRM_PROCESSES   
    JoinLayer (NAME_DEF_LAYER, "welcome", 0, 0);
#endif

    srand (time(NULL));

    gen_letters();

    mGEffInit();
    play_animations();
    mGEffDeinit();

    SetTimer (HWND_DESKTOP, (LINT)argv, 100);    // quit after 1s
    while (GetMessage(&msg, HWND_DESKTOP)) {
        if (msg.message == MSG_TIMER && msg.wParam == (WPARAM)argv)
            break;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

