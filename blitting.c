#include "blitting.h"
#include "inc/vsh_exports.h"
#include "png_dec.h"
#include "misc.h"
#include "mem.h"

#include <cell/rtc.h>



// graphic buffers and drawing context
static Buffer buf[2];
static DrawCtx ctx;

// display values
static uint32_t unk1 = 0, offset = 0, pitch = 0;
static uint16_t h = 0, w = 0, canvas_x = 0, canvas_y = 0;



/***********************************************************************
* pause rsx fifo
***********************************************************************/
void pause_RSX_rendering()
{
    // is a flip occurred ? (the moment RSX has finished the rendering of a new frame and flip him on screen)
    while(1)
        if(*(uint32_t*)0x60201100 == 0x80000000) break;

    rsx_fifo_pause(1);       // pause rsx fifo (no new frames...)
}


/***********************************************************************
* dump background
***********************************************************************/
static void dump_bg(void)
{
    uint16_t i, k;
    uint64_t *bg = (uint64_t*)ctx.bg;

    for(i = 0; i < CANVAS_H; i++)
        for(k = 0; k < CANVAS_W /2; k++)
            bg[k + i * CANVAS_W /2] = *(uint64_t*)(OFFSET(canvas_x + (k*2), canvas_y + (i)));
}


/***********************************************************************
*
***********************************************************************/
void init_graphic()
{
    memset(&ctx, 0, sizeof(DrawCtx));

    // alloc VSH Menu graphic buffers, generic based on canvas constants
    buf[0].addr = mem_alloc(CANVAS_W * CANVAS_H * sizeof(uint32_t));    // canvas buffer
    buf[1].addr = mem_alloc(CANVAS_W * CANVAS_H * sizeof(uint32_t));    // background buffer

    #ifdef HAVE_PNG_FONT
    // load font png
    Buffer font = load_png(PNG_FONT_PATH);
    ctx.font    = font.addr;
    #endif

    // set drawing context
    ctx.canvas   = buf[0].addr;
    ctx.bg       = buf[1].addr;
    ctx.bg_color = 0xFF000000;          // black, opaque
    ctx.fg_color = 0xFFFFFFFF;          // white, opaque

    // get current display values
    offset = *(uint32_t*)0x60201104;    // start offset of current framebuffer
    getDisplayPitch(&pitch, &unk1);     // framebuffer pitch size
    h = getDisplayHeight();             // display height
    w = getDisplayWidth();              // display width

    // get x/y start coordinates for our canvas, always center
    canvas_x = (w - CANVAS_W) /2;
    canvas_y = (h - CANVAS_H) /2;

    // dump background, for alpha blending
    dump_bg();

    // init first frame with background dump
    memcpy((uint8_t *)ctx.canvas, (uint8_t *)ctx.bg, CANVAS_W * CANVAS_H * sizeof(uint32_t));
}


/***********************************************************************
* load a png file
* 
* int32_t idx      = index of png, max 4 (0 - 3)
* const char *path = path to png file
***********************************************************************/
int32_t load_png_bitmap(int32_t idx, const char *path)
{
    if(idx > PNG_MAX) return -1;
    ctx.png[idx] = load_png(path);
    return 0;
}


/***********************************************************************
* alpha blending (ARGB)
*
* uint32_t bg = background color
* uint32_t fg = foreground color
***********************************************************************/
static uint32_t mix_color(uint32_t bg, uint32_t fg)
{
    uint32_t a = fg >>24;

    if(a == 0) return bg;

    uint32_t rb = (((fg & 0x00FF00FF) * a) + ((bg & 0x00FF00FF) * (255 - a))) & 0xFF00FF00;
    uint32_t g  = (((fg & 0x0000FF00) * a) + ((bg & 0x0000FF00) * (255 - a))) & 0x00FF0000;
    fg = a + ((bg >>24) * (255 - a) / 255);

    return (fg <<24) | ((rb | g) >>8);
}


/***********************************************************************
* flip finished frame into paused ps3-framebuffer
***********************************************************************/
void flip_frame()
{
    uint16_t i, k;
    uint64_t *canvas = (uint64_t*)ctx.canvas;

    for(i = 0; i < CANVAS_H; i++)
        for(k = 0; k < CANVAS_W /2; k++)
            *(uint64_t*)(OFFSET(canvas_x + (k*2), canvas_y + (i))) = canvas[k + i * CANVAS_W /2];

    // after flip, clear frame buffer with background
    memcpy((uint8_t *)ctx.canvas, (uint8_t *)ctx.bg, CANVAS_W * CANVAS_H * sizeof(uint32_t));
}


/***********************************************************************
* set background color
***********************************************************************/
void set_background_color(uint32_t color)
{
    ctx.bg_color = color;
}


/***********************************************************************
* set foreground color
***********************************************************************/
void set_foreground_color(uint32_t color)
{
    ctx.fg_color = color;
}


/***********************************************************************
* draw background, with current background color
***********************************************************************/
void draw_background()
{
    uint16_t tmp_x = 0, tmp_y = 0;
    uint32_t i;

    for(i = 0; i < CANVAS_W * CANVAS_H; i++)
    {
        ctx.canvas[i] = mix_color(ctx.bg[i], ctx.bg_color);

        tmp_x++;

        if(tmp_x == CANVAS_W)
        {
            tmp_x = 0;
            tmp_y++;
        }
    }
}


/***********************************************************************
* compute x to align text into canvas
*
* const char *str = referring string
* uint8_t align   = CENTER / RIGHT (1/2)
***********************************************************************/
uint16_t get_aligned_x(const char *str, uint8_t align)
{
    return (CANVAS_W - (strlen(str) * FONT_W)) / align;
}


#ifdef HAVE_PNG_FONT
/***********************************************************************
* print text, with data from font.png
*
* int32_t x       = start x coordinate into canvas
* int32_t y       = start y coordinate into canvas
* const char *str = string to print
***********************************************************************/
void print_text(int32_t x, int32_t y, const char *str)
{
    int32_t c_x = x, c_y = y;
    int32_t i = 0, char_w = 0, p_x = 0, p_y = 0;
    int32_t tmp_x = 0, tmp_y = 0;
    uint8_t c = 0;

    while(*str != '\0')
    {
        c = *str;

        if(c > 127)
        {
            str++;
            c = (*str & 0x3F) | 0x80;
        }

        if(c == '\n')
        {
            c_x = x;
            c_y += FONT_H;
        }
        else
        {
            p_y = (c >> 4) * FONT_H * FONT_PNG_W;
            p_x = (c & 0x0F) * FONT_W;

            char_w = ctx.font[p_x + p_y];

            for(i = 0; i < FONT_H * char_w; i++)
            {
                if((ctx.font[(p_x + tmp_x) + (p_y + tmp_y * FONT_PNG_W)]) != 0)
                {
                    ctx.canvas[(c_y + tmp_y) * CANVAS_W + c_x + tmp_x] =
                    mix_color(ctx.canvas[(c_y + tmp_y) * CANVAS_W + c_x + tmp_x],
                    (ctx.font[(p_x + tmp_x) + (p_y + tmp_y * FONT_PNG_W)] & 0xFF000000) |
                    (ctx.fg_color & 0x00FFFFFF));
                }

                tmp_x++;

                if(tmp_x == char_w)
                {
                    tmp_x = 0;
                    tmp_y++;
                }
            }
            tmp_y = 0;
            c_x += char_w;
        }
        str++;
    }
}

#else

/***********************************************************************
* print text, with data from xbm_font.h
*
* int32_t x       = start x coordinate into canvas
* int32_t y       = start y coordinate into canvas
* const char *str = string to print
***********************************************************************/
#include "xbm_font.h"
void print_text(int32_t x, int32_t y, const char *str)
{
    uint8_t c, i, j, tx = 0, ty = 0;
    uint32_t tc = ctx.fg_color;

    while(*str != '\0')
    {
        // address the font bitmap
        c = *str;
        if(c < LOWER_ASCII_CODE || c > UPPER_ASCII_CODE) c = 180;

        char *bit = xbmFont[c - LOWER_ASCII_CODE];

        // reset color (for each glyph) to get vertical gradient
        tc = ctx.fg_color;

        // dump bits map (bytes_per_line 2, size 32 char of 8 bit)
        for(i = 0; i < ((FONT_W * FONT_H) / BITS_IN_BYTE); i++)
        {
            for(j = 0; j < BITS_IN_BYTE; j++)
            {
                // least significant bit first
                if(bit[i] & (1 << j))
                {
                    // draw a shadow, displaced by +2px
                    ctx.canvas[(x + tx * BITS_IN_BYTE + j +2) + (y + ty +2) * CANVAS_W] = 0xff303030;

                    // paint FG pixel
                    ctx.canvas[(x + tx * BITS_IN_BYTE + j) + (y + ty) * CANVAS_W] = tc;
                }
                else
                {
                    // paint BG pixel (or do nothing for trasparency)
                    //ctx.canvas[(x + tx * BITS_IN_BYTE + j) + (y + ty) * CANVAS_W] = ctx.bg_color;
                }
            }
            tx++;

            if(tx == (FONT_W / BITS_IN_BYTE))
            {
                tx = 0, ty++;        // use to decrease gradient

                // vertical gradient
                tc -= ty * 580;

                // horizontal gradient
                //tc -= ty * 15;
            }
        }
        ty = 0;

        // glyph painted, move one char right in text
        x += FONT_W;
        ++str;
    }
}
#endif


/***********************************************************************
* draw png part into frame.
*
* int32_t idx      =  index of loaded png
* int32_t can_x    =  start x coordinate into canvas
* int32_t can_y    =  start y coordinate into canvas
* int32_t png_x    =  start x coordinate into png
* int32_t png_y    =  start y coordinate into png
* int32_t w        =  width of png part to blit
* int32_t h        =  height of png part to blit
***********************************************************************/
void draw_png(int32_t idx, int32_t can_x, int32_t can_y, int32_t png_x, int32_t png_y, int32_t w, int32_t h)
{
    int32_t i = 0;
    int32_t tmp_x = 0, tmp_y = 0;

    for(i = 0; i < w * h; i++)
    {
        ctx.canvas[(can_x + can_y * CANVAS_W) + (tmp_x + tmp_y * CANVAS_W)] =
            mix_color(ctx.canvas[(can_x + can_y * CANVAS_W) + (tmp_x + tmp_y * CANVAS_W)],
            ctx.png[idx].addr[(png_x + png_y * ctx.png[idx].w) + (tmp_x + tmp_y * ctx.png[idx].w)]);

        tmp_x++;

        if(tmp_x == w)
        {
            tmp_x = 0;
            tmp_y++;
        }
    }
}


uint8_t bmp_header[] = {
  0x42, 0x4D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


/***********************************************************************
* xmb screenshot
*
* uint8_t mode = 0(XMB only), 1(XMB + menu)
***********************************************************************/
void screenshot(uint8_t mode)
{
    FILE *fd = NULL;
    uint32_t tmp = 0;
    int32_t i, k, idx = 0, pad = 0, mem_size = (15 * 1024 * 1024);  // 9MB(frame dump) 6MB(bmp data)
    sys_addr_t sys_mem = NULL;
    sys_memory_container_t mc_app = (sys_memory_container_t)-1;
    CellRtcDateTime t;
    char path[128];


    // alloc buffers
    mc_app = vsh_memory_container_by_id(1);
    sys_memory_allocate_from_container(mem_size, mc_app, SYS_MEMORY_PAGE_SIZE_1M, &sys_mem);
    uint64_t *dump_buf = (uint64_t*)sys_mem;
    uint8_t *bmp_buf = (uint8_t*)sys_mem + (9 * 1024 * 1024);
    uint64_t *bg = (uint64_t*)ctx.bg;

    // build file path
    cellRtcGetCurrentClockLocalTime(&t);
    sprintf(path, "/dev_hdd0/screenshot_%02d_%02d_%02d_%02d_%02d_%02d.bmp", t.year, t.month, t.day, t.hour, t.minute, t.second);

    // create bmp file
    fd = fopen(path, "wb");

    // dump framebuffer (restore if needed)
    for(i = 0; i < h; i++)
        for(k = 0; k < w/2; k++)
        {
            dump_buf[k + i * w/2] = *(uint64_t*)(OFFSET(k*2, i));
            
            if(mode == 0)
                if((k*2 >= canvas_x) && (k*2 < canvas_x + CANVAS_W) && (i >= canvas_y) && (i < canvas_y + CANVAS_H))
                    dump_buf[k + i * w/2] = bg[(((i - canvas_y) * CANVAS_W) + ((k*2) - canvas_x)) /2];
        }

    // convert dump color data from ARGB to RGB
    uint8_t *tmp_buf = (uint8_t*)sys_mem;

    for(i = 0; i < h; i++)
    {
        idx = (h-1-i)*w*3;

        for(k = 0; k < w; k++)
        {
            bmp_buf[idx   ] = tmp_buf[(i*w+k)*4 +3];  // R
            bmp_buf[idx +1] = tmp_buf[(i*w+k)*4 +2];  // G
            bmp_buf[idx +2] = tmp_buf[(i*w+k)*4 +1];  // B
            idx+=3;
        }
    }

    // set bmp header
    tmp = _ES32(w*h*3+0x36);
    memcpy(bmp_header + 2 , &tmp, 4);     // file size
    tmp = _ES32(w);
    memcpy(bmp_header + 18, &tmp, 4);     // bmp width
    tmp = _ES32(h);
    memcpy(bmp_header + 22, &tmp, 4);     // bmp height
    tmp = _ES32(w*h*3);
    memcpy(bmp_header + 34, &tmp, 4);     // bmp data size

    // write bmp header
    fwrite(bmp_header, 1, sizeof(bmp_header), fd);

    // write bmp data
    fwrite(bmp_buf, 1, (w*h*3), fd);

    // padding
    int32_t rest = (w*3) % 4;
    if(rest)
         pad = 4 - rest;
    fseek(fd, pad, SEEK_CUR);

    fclose(fd);
    sys_memory_free((sys_addr_t)sys_mem);
}


// some primitives...
/***********************************************************************
* draw a single pixel,
*
* int32_t x   = start x coordinate into frame
* int32_t y   = start y coordinate into frame
**********************************************************************
void draw_pixel(int32_t x, int32_t y)
{
    ctx.canvas[x + y * CANVAS_W] = ctx.fg_color;
}*/

/***********************************************************************
* draw a line,
*
* int32_t x   = line start x coordinate into frame
* int32_t y   = line start y coordinate into frame
* int32_t x2  = line end x coordinate into frame
* int32_t y2  = line end y coordinate into frame
**********************************************************************
void draw_line(int32_t x, int32_t y, int32_t x2, int32_t y2)
{
    int32_t i = 0, dx1 = 0, dy1 = 0, dx2 = 0, dy2 = 0;
    int32_t w = x2 - x;
    int32_t h = y2 - y;


    if(w < 0) dx1 = -1; else if(w > 0) dx1 = 1;
    if(h < 0) dy1 = -1; else if(h > 0) dy1 = 1;
    if(w < 0) dx2 = -1; else if(w > 0) dx2 = 1;

    int32_t l = abs(w);
    int32_t s = abs(h);

    if(!(l > s))
    {
        l = abs(h);
        s = abs(w);
        
    if(h < 0) dy2 = -1; else if(h > 0) dy2 = 1;

        dx2 = 0;
    }

    int32_t num = l >> 1;
    
    for(i = 0; i <= l; i++)
    {
        ctx.canvas[x + y * CANVAS_W] = ctx.fg_color;
    num+=s;

    if(!(num < l))
    {
            num-=l;
            x+=dx1;
            y+=dy1;
        }
        else
        {
            x+=dx2;
            y+=dy2;
        }
    }
}*/

/***********************************************************************
* draw a rectangle,
*
* int32_t x   = rectangle start x coordinate into frame
* int32_t y   = rectangle start y coordinate into frame
* int32_t w   = width of rectangle
* int32_t h   = height of rectangle
**********************************************************************
void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    draw_line(x, y, x + w, y);
    draw_line(x + w, y, x + w, y + h);
    draw_line(x + w, y + h, x, y + h);
    draw_line(x, y + h, x, y);
}*/

/***********************************************************************
* circle helper function
* 
* int32_t x_c = circle center x coordinate into frame
* int32_t y_c = circle center y coordinate into frame
* int32_t x   = circle point x coordinate into frame
* int32_t y   = circle point y coordinate into frame
**********************************************************************
static void circle_points(int32_t x_c, int32_t y_c, int32_t x, int32_t y)
{
    draw_pixel(x_c + x, y_c + y);
    draw_pixel(x_c - x, y_c + y);
    draw_pixel(x_c + x, y_c - y);
    draw_pixel(x_c - x, y_c - y);
    draw_pixel(x_c + y, y_c + x);
    draw_pixel(x_c - y, y_c + x);
    draw_pixel(x_c + y, y_c - x);
    draw_pixel(x_c - y, y_c - x);
}*/

/***********************************************************************
* draw a circle,
*
* int32_t x_c = circle center x coordinate into frame
* int32_t y_c = circle center y coordinate into frame
* int32_t r   = circle radius
**********************************************************************
void draw_circle(int32_t x_c, int32_t y_c, int32_t r)
{
    int32_t x = 0;
    int32_t y = r;
    int32_t p = 1 - r;

    circle_points(x_c, y_c, x, y);

    while(x < y)
    {
        x++;
        
        if(p < 0)
        {
            p += 2 * x + 1;
        }
        else
        {
            y--;
            p += 2 * (x - y) + 1;
        }
        
        circle_points(x_c, y_c, x, y);
    }
}*/


#ifdef HAVE_STARFIELD
/* Copyright (C) 2002 W.P. van Paassen - peter@paassen.tmfweb.nl

   This program is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your
   option) any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to the Free
   Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* note that the code has not been fully optimized */

#define NUMBER_OF_STARS 256*6        // max 2^16 for uint16_t

static STAR stars[NUMBER_OF_STARS];

void init_star(STAR *star, const uint16_t i)
{
    /* randomly init stars, generate them around the center of the screen */
    star->xpos =  -10.0 + (20.0 * (rand()/(RAND_MAX+1.0)));
    star->ypos =  -10.0 + (20.0 * (rand()/(RAND_MAX+1.0)));

    /* change viewpoint */
    star->xpos *= 3141;
    star->ypos *= 3141;

    star->zpos =  i;
    star->speed = 2 + (int)(2.0 * (rand()/(RAND_MAX+1.0)));

    /* the closer to the viewer the brighter */
    star->color = i >> 2;
}

void init_once(/* stars, first run */)
{
  uint16_t i;
  for(i = 0; i < NUMBER_OF_STARS; i++) 
    init_star(stars + i, i + 1);
}

void move_star(void)
{
    int16_t tx, ty;
    uint16_t i;

    for(i = 0; i < NUMBER_OF_STARS; i++){
        stars[i].zpos -= stars[i].speed;

        if(stars[i].zpos <= 0) init_star(stars + i, i + 1);

        /* compute 3D position */
        tx = (stars[i].xpos / stars[i].zpos) + (CANVAS_W >> 1);
        ty = (stars[i].ypos / stars[i].zpos) + (CANVAS_H >> 1);

        /* check if a star leaves the screen */
        if(tx < 0 || tx > CANVAS_W - 1
        || ty < 0 || ty > CANVAS_H - 1)
        {
            init_star(stars + i, i + 1);
            continue;
        }

        ctx.canvas[tx + ty * CANVAS_W] = ARGB(0xff, stars[i].color, stars[i].color, stars[i].color);
    }
}
#endif
