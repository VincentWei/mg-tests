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
** Flying-GUIs - Another MiniGUI demo...
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include <mgeff/mgeff.h>

/* 16.16 fixed number */
#define fixed  int32_t

#define DEFAULT_WIDTH   320
#define DEFAULT_HEIGHT  200

#define DEFAULT_GENTIME      200   /* msec */
#define DEFAULT_MAXSIZE      100   /* percent */
#define DEFAULT_CLUSTERSIZE  100   /* percent */
#define DEFAULT_SPEED        30

/* Global Info */

static int screen_width;
static int screen_height;
static int screen_diag;

static int banner_width;
static int banner_height;
static int banner_diag;

static int gen_time;      /* msec */
static fixed max_size;    /* default 1.0 = full screen */
static int cluster_size;  /* pixels */
static int fixed_speed=0;
static int speed;

static int use_putbox;

static gal_pixel lookup[256];

static HDC mem_dc;

static void *image_buf;
static int image_size;

typedef struct texture
{
    struct texture *succ;
    struct texture *tail;

    fixed mid_x, mid_y;
    fixed size;
    fixed millis;
    fixed speed;

    gal_uint8 color;
} Texture;

static Texture *texture_list;

static char *banner[46];
static void banner_size(int *width, int *height)
{
    *width=0;

    for (*height=0; banner[*height] != NULL; (*height)++) {

        int len = strlen(banner[*height]);

        if (len > *width) {
            *width = len;
        }
    }
}

static int random_in_range(int low, int high)
{
    return low + random() % (high-low+1);
}

static void setup_palette(void)
{
    int i;

    use_putbox = 0;

    if (GetGDCapability (HDC_SCREEN, GDCAP_DEPTH) == 8)
        use_putbox = 1;

    for (i=0; i < 256; i++) {

        GAL_Color col;

        col.r = ((i >> 5) & 7) * 0xffff / 7;
        col.g = ((i >> 2) & 7) * 0xffff / 7;
        col.b = ((i)      & 3) * 0xffff / 3;

        lookup[i] = RGB2Pixel (HDC_SCREEN, col.r, col.g, col.b);
    }
}

static gal_uint8 trans_buffer[8192];

static void translate_hline(HDC hdc, int x, int y, int w, gal_uint8 *data)
{
    int ww = w;
    BITMAP bmp = {BMP_TYPE_NORMAL};

    gal_uint8  *buf1 = (gal_uint8  *) trans_buffer;
    gal_uint16 *buf2 = (gal_uint16 *) trans_buffer;
    gal_uint32 *buf4 = (gal_uint32 *) trans_buffer;

    switch ( GetGDCapability (hdc, GDCAP_BPP)) {

    case 1:
        for (; ww > 0; ww--) {
            *buf1++ = lookup[*data++];
        }
        break;

    case 2:
        for (; ww > 0; ww--) {
            *buf2++ = lookup[*data++];
        }
        break;

    case 3:
        for (; ww > 0; ww--) {
            gal_pixel pix = lookup[*data++];

            *buf1++ = pix; pix >>= 8;
            *buf1++ = pix; pix >>= 8;
            *buf1++ = pix;
        }
        break;

    case 4:
        for (; ww > 0; ww--) {
            *buf4++ = lookup[*data++];
        }
        break;
    }

    bmp.bmBitsPerPixel = GetGDCapability (hdc, GDCAP_DEPTH);
    bmp.bmBytesPerPixel = GetGDCapability (hdc, GDCAP_BPP);
    bmp.bmPitch = w;
    bmp.bmWidth = w; bmp.bmHeight = 1;
    bmp.bmBits = trans_buffer;
    FillBoxWithBitmap (hdc, x, y, 0, 0, &bmp);
}

static void update_frame(HDC hdc)
{
    if (use_putbox) {
        BITMAP bmp = {BMP_TYPE_NORMAL};
        bmp.bmBitsPerPixel = GetGDCapability (hdc, GDCAP_DEPTH);
        bmp.bmBytesPerPixel = GetGDCapability (hdc, GDCAP_BPP);
        bmp.bmPitch = screen_width;
        bmp.bmWidth = screen_width;
        bmp.bmHeight = screen_height;
        bmp.bmBits = image_buf;
        FillBoxWithBitmap (hdc, 0, 0, 0, 0, &bmp);
    } else {
        int y;
        gal_uint8 *src = (gal_uint8 *) image_buf;

        for (y=0; y < screen_height; y++) {
            translate_hline(hdc, 0, y, screen_width, src);
            src += screen_width;
        }
    }
}

static void init_textures(void)
{
    texture_list = NULL;
}

static void free_textures(void)
{
    Texture *t;

    while (texture_list != NULL) {

        t = texture_list;
        texture_list = t->succ;

        free(t);
    }
}

static void add_texture(int x, int y, gal_uint8 color)
{
    Texture *t;

    t = (Texture *) malloc(sizeof(Texture));

    t->mid_x = x << 16;
    t->mid_y = y << 16;

    t->millis = 0;
    t->color  = color;
    t->speed  = speed + (fixed_speed ? 0 :
            random_in_range(-(speed/2), +(speed/2)));
    t->succ = texture_list;
    texture_list = t;
}

static void render_texture(int width, int height, Texture *t)
{
    int x, y;
    int sx, sy, bx;
    int dx, dy;

    gal_uint8 *dest;

    height <<= 16;
    width  <<= 16;

    dx = dy = t->size * screen_diag / banner_diag;

    bx = t->mid_x - (banner_width  * dx / 2);
    sy = t->mid_y - (banner_height * dy / 2);

    for (y=0; (banner[y] != NULL) && (sy < height); y++, sy += dy) {

        char *pos = banner[y];

        if (sy >= 0) {

            dest = image_buf;
            dest += ((sy>>16) * screen_width);

            for (x=0, sx=bx; (*pos != 0) && (sx < width);
                    x++, pos++, sx += dx) {

                if ((sx >= 0) && (*pos == '#'))
                {
                    dest[sx>>16] = t->color;
                }
            }
        }
    }
}

static void update_texture(Texture *t, Texture ***prev_ptr, int millis)
{
    t->millis += millis;

    t->size = t->millis * t->speed;

    if (t->size > max_size) {

        /* remove texture */

        **prev_ptr = t->succ;

        free(t);

        return;
    }

    *prev_ptr = &t->succ;

    render_texture(screen_width, screen_height, t);
}

static void update_all_textures(int millis)
{
    Texture *cur;
    Texture **prev_ptr;

    cur = texture_list;
    prev_ptr = (Texture **) &texture_list;

    while (cur != NULL) {

        update_texture(cur, &prev_ptr, millis);

        cur = *prev_ptr;
    }
}

struct timeval cur_time, prev_time;
static int gen_millis;
static void InitFlyingGUI (HWND hwnd)
{
    /* initialize */
    srand(time(NULL));

    banner_size(&banner_width, &banner_height);

    banner_diag = sqrt(banner_width * banner_width +
            banner_height * banner_height);

    screen_width  = DEFAULT_WIDTH;
    screen_height = DEFAULT_HEIGHT;

    gen_time = DEFAULT_GENTIME;

    speed        = -1;
    max_size     = -1;
    cluster_size = -1;

    init_textures();

    if (speed < 0) {
        speed = DEFAULT_SPEED;
    }

    if (max_size < 0) {
        max_size = DEFAULT_MAXSIZE;
    }

    max_size = (max_size << 16) / 100;

    if (cluster_size < 0) {
        cluster_size = DEFAULT_CLUSTERSIZE;
    }

    setup_palette();

    screen_width  = 320;
    screen_height = 200;

    screen_diag = sqrt(screen_width  * screen_width +
            screen_height * screen_height);

    image_size = screen_width * screen_height * 1;

    image_buf = malloc(image_size);

    gettimeofday (&prev_time, NULL);

    gen_millis=0;

    mem_dc = CreateCompatibleDCEx(HDC_SCREEN, screen_width, screen_height);
}

#define CMPOPT(x,s,l,n)  (((strcmp(x,s)==0) || \
               (strcmp(x,l)==0)) && ((i+n) < argc))

static void OnPaint (HDC hdc)
{
    int x, y;
    int millis;


    x = screen_width/2;
    y = screen_width/2;

    /* determine time lapse */

    gettimeofday (&cur_time, NULL);

    millis = (cur_time.tv_sec  - prev_time.tv_sec)  * 1000
                       + (cur_time.tv_usec - prev_time.tv_usec) / 1000;

    prev_time = cur_time;

    if (use_putbox) {
        memset(image_buf, lookup[0], image_size);
    }
    else {
        memset(image_buf, 0,         image_size);
    }

    update_all_textures(millis);

    update_frame (hdc);

    for (gen_millis += millis; gen_millis >  gen_time;
                     gen_millis -= gen_time) {

        int disp_x = screen_width  * cluster_size / 200;
        int disp_y = screen_height * cluster_size / 141;

        x += random_in_range(-disp_x, +disp_x);
        y += random_in_range(-disp_y, +disp_y);

        if (x < 0) {
                x += screen_width;
        }
        if (y < 0) {
                y += screen_height;
        }
        if (x >= screen_width) {
                x -= screen_width;
        }
        if (y >= screen_height) {
                y -= screen_height;
        }

        add_texture(x, y, random_in_range(0, 255));
    }

}

static void TermFlyingGUI (HWND hwnd)
{
    free_textures();
    free(image_buf);

    DeleteMemDC(mem_dc);
}

static LRESULT FlyingGUIWinProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_CREATE:
        mGEffInit();
        InitFlyingGUI (hWnd);
        SetTimer (hWnd, 100, 5);
        break;

    case MSG_TIMER:
        if (wParam == 100) {
            OnPaint (mem_dc);
            InvalidateRect (hWnd, NULL, FALSE);
        }
        break;

    case MSG_PAINT: {
        HDC hdc = BeginPaint (hWnd);
        BitBlt(mem_dc, 0, 0, screen_width, screen_height, hdc, 0, 0, 0);
        EndPaint (hWnd, hdc);
        return 0;
    }

    case MSG_CLOSE:
        KillTimer (hWnd, 100);
        DestroyMainWindow(hWnd);
        MainWindowCleanup(hWnd);
        return 0;

    case MSG_DESTROY:
        mGEffDeinit();
        TermFlyingGUI(hWnd);
        return 0;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

static void InitCreateInfo (PMAINWINCREATE pCreateInfo)
{
    pCreateInfo->dwStyle = WS_CAPTION | WS_VISIBLE | WS_BORDER;
    pCreateInfo->dwExStyle = WS_EX_TOPMOST;
    pCreateInfo->spCaption = "Flying GUI";
    pCreateInfo->hMenu = 0;
    pCreateInfo->hCursor = GetSystemCursor (0);
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = FlyingGUIWinProc;
    pCreateInfo->lx = 0;
    pCreateInfo->ty = 0;
    pCreateInfo->rx = pCreateInfo->lx + DEFAULT_WIDTH;
    pCreateInfo->by = pCreateInfo->ty + DEFAULT_HEIGHT;
    pCreateInfo->iBkColor = COLOR_black;
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting = HWND_DESKTOP;
}

static void animated_cb(MGEFF_ANIMATION handle, HWND hwnd, int id, POINT *pt)
{
    HWND hosting;

    MoveWindow (hwnd, pt->x, pt->y, DEFAULT_WIDTH, DEFAULT_HEIGHT, FALSE);

    hosting = GetHosting(hwnd);
    UpdateInvalidClient(hosting, FALSE);
}

void move_flying_window (HWND hwnd)
{
    MGEFF_ANIMATION animation;
    animation = mGEffAnimationCreate((void *)hwnd, (void *)animated_cb, 1, MGEFF_POINT);
    if (animation) {
        RECT rc;
        POINT start_pt, end_pt;
        GetWindowRect(hwnd, &rc);

        start_pt.x = rc.left;
        start_pt.y = rc.top;
        end_pt.x = random_in_range(-DEFAULT_WIDTH/2, g_rcScr.right - DEFAULT_WIDTH/2);
        end_pt.y = random_in_range(-DEFAULT_HEIGHT/2, g_rcScr.bottom - DEFAULT_HEIGHT/2);

        mGEffAnimationSetStartValue(animation, &start_pt);
        mGEffAnimationSetEndValue(animation, &end_pt);
        mGEffAnimationSetDuration(animation, 200);
        mGEffAnimationSetProperty(animation, MGEFF_PROP_LOOPCOUNT, 1);
        mGEffAnimationSetCurve(animation, InOutQuart);
        mGEffAnimationSyncRun(animation);
        mGEffAnimationDelete(animation);
    }
    else {
        MoveWindow (hwnd,
            random_in_range(-DEFAULT_WIDTH/2, g_rcScr.right - DEFAULT_WIDTH/2),
            random_in_range(-DEFAULT_HEIGHT/2, g_rcScr.bottom - DEFAULT_HEIGHT/2),
            DEFAULT_WIDTH, DEFAULT_HEIGHT, FALSE);
    }
}

HWND create_flying_window (HWND hosting)
{
    MAINWINCREATE CreateInfo;
    HWND hMainWnd;

    InitCreateInfo (&CreateInfo);
    CreateInfo.hHosting = hosting;
    hMainWnd = CreateMainWindow (&CreateInfo);

    if (hMainWnd != HWND_INVALID) {
        ShowWindow(hMainWnd, SW_SHOW);
        EnableWindow(hMainWnd, FALSE);
    }

    return hMainWnd;
}

/* Image of GUI */
static char *banner[46] = {

"..................###########..................................................................................",
"..............###################...##.....#################..............################..###################",
"............########.......###########........###########....................###########........###########....",
"..........#######.............########.........#########......................#########..........#########.....",
".........######................#######..........#######........................#######............#######......",
"........######...................#####..........#######........................#######............#######......",
".......######.....................#####.........#######........................#######............#######......",
"......######......................#####.........#######........................#######............#######......",
".....#######.......................####.........#######........................#######............#######......",
"....#######.........................###.........#######........................#######............#######......",
"....######..........................###.........#######........................#######............#######......",
"...#######..........................###.........#######........................#######............#######......",
"...#######...........................##.........#######........................#######............#######......",
"..#######.......................................#######........................#######............#######......",
"..#######.......................................#######........................#######............#######......",
".########.......................................#######........................#######............#######......",
".#######........................................#######........................#######............#######......",
".#######........................................#######........................#######............#######......",
".#######........................................#######........................#######............#######......",
"########........................................#######........................#######............#######......",
"########........................................#######........................#######............#######......",
"########..................###################...#######........................#######............#######......",
"########......................###########.......#######........................#######............#######......",
"########.......................#########........#######........................#######............#######......",
"########........................########........#######........................#######............#######......",
"########........................#######.........#######........................#######............#######......",
"########........................#######.........#######........................#######............#######......",
"#########.......................#######.........#######........................#######............#######......",
".########.......................#######.........#######........................#######............#######......",
".########.......................#######.........#######........................#######............#######......",
".########.......................#######.........#######........................#######............#######......",
"..########......................#######.........#######........................#######............#######......",
"..########......................#######.........#######........................#######............#######......",
"..#########.....................#######.........#######........................#######............#######......",
"...########.....................#######..........#######......................#######.............#######......",
"....########....................#######..........#######......................#######.............#######......",
"....########....................#######...........#######....................#######..............#######......",
".....########...................#######...........#######....................#######..............#######......",
"......########..................#######............#######..................#######...............#######......",
".......########.................#######.............#######................#######................#######......",
"........########................#######..............#######..............#######.................#######......",
"..........########............#########...............#######............#######.................#########.....",
"............#########......##########..................########.........#######.................###########....",
"..............####################........................##################................###################",
"..................############...............................############......................................",

NULL
};
