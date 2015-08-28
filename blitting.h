#ifndef __BLITT_H__
#define __BLITT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// font constants
#ifdef HAVE_PNG_FONT
#define PNG_FONT_PATH "/dev_hdd0/font.png"  // use external font.png
#define FONT_PNG_W    512                   // width of font png file in pixel
#define FONT_PNG_H    514                   // height of font png file in pixel
#define FONT_W        18                    // font width in pixel
#define FONT_H        22                    // font height in pixel
#else
#define FONT_W        16                    // use hardcoded xbm_font
#define FONT_H        16
#define SHADOW_PX     2                     // lower-right text shadow in pixel
#endif

#define FONT_D        1                     // distance to next char
#define BORD_D        4                     // distance from canvas border
// additional png bitmaps
#define PNG_MAX       4

// canvas constants
// the values for canvas width and height can be changed for make a smaller or larger
// menu. They must be smaller than the max resulution of 1920*1080 and additional
// code is necessary to manage the other possible resolutions.
// Due to the fact that we read and write 64bit values(2 pixel) at once to the framebuffer,
// CANVAS_W must be a multiple of 2.
#define BASE          0xC0000000UL      // local memory base ea
#define CANVAS_W      720               // canvas width in pixel
#define CANVAS_H      400               // canvas height in pixel

// get pixel offset into framebuffer by x/y coordinates
#define OFFSET(x, y) ((((uint32_t)offset) + ((((int16_t)x) + \
                     (((int16_t)y) * (((uint32_t)pitch) / \
                     ((int32_t)4)))) * ((int32_t)4))) + (BASE))

// compose ARGB color by components
#define ARGB(a, r, g, b) ((((a) &0xFF) <<24) | (((r) &0xFF) <<16) \
                        | (((g) &0xFF) << 8) | (((b) &0xFF) << 0))

// extract single component form ARGB color
#define GET_A(color) ((color >>24) &0xFF)
#define GET_R(color) ((color >>16) &0xFF)
#define GET_G(color) ((color >> 8) &0xFF)
#define GET_B(color) ((color >> 0) &0xFF)


// graphic buffers
typedef struct _Buffer{
    uint32_t *addr;        // buffer address
    uint16_t  w;           // buffer width
    uint16_t  h;           // buffer height
} Buffer;


// drawing context
typedef struct _DrawCtx{
    uint32_t *canvas;      // addr of canvas
    uint32_t *bg;          // addr of background backup
    uint32_t bg_color;     // background color
    uint32_t fg_color;     // foreground color

    #ifdef HAVE_PNG_FONT
    uint32_t *font;        // addr of decoded png font
    #endif

    Buffer   png[PNG_MAX]; // bitmaps
} DrawCtx;


void pause_RSX_rendering(void);

void init_graphic(void);
int32_t load_png_bitmap(int32_t idx, const char *path);
void flip_frame(void);
void set_background_color(uint32_t color);
void set_foreground_color(uint32_t color);
void draw_background(void);
void draw_png(int32_t idx, int32_t can_x, int32_t can_y, int32_t png_x, int32_t png_y, int32_t w, int32_t h);

// text
#define LEFT      0   // useless
#define RIGHT     1
#define CENTER    2
uint16_t get_aligned_x(const char *str, uint8_t alignment);
void print_text(int32_t x, int32_t y, const char *str);

void screenshot(uint8_t mode);

// primitives
//void draw_pixel(int32_t x, int32_t y);
//void draw_line(int32_t x, int32_t y, int32_t x2, int32_t y2);
//void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
//void draw_circle(int32_t x_c, int32_t y_c, int32_t r);

#ifdef HAVE_STARFIELD
typedef struct _STAR{
    float   xpos, ypos;
    int16_t zpos, speed;
    uint8_t color;
} STAR;

void init_star(STAR *star, const uint16_t i);
void init_once(void);
void move_star(void);
#endif

#endif // __BLITT_H__
