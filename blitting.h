#ifndef __BLITT_H__
#define __BLITT_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "inc/vsh_exports.h"


// font constants
#ifdef HAVE_SYS_FONT
#define FONT_W          16.f            // font width
#define FONT_H          16.f            // font height
#define FONT_WEIGHT     1.f             // font weight
#define FONT_CACHE_MAX  256             // max glyph cache count

#elif HAVE_PNG_FONT
#define PNG_FONT_PATH   "/dev_hdd0/font.png" // use external font.png
#define FONT_PNG_W      512                  // width of font png file in pixel
#define FONT_PNG_H      514                  // height of font png file in pixel
#define FONT_W          18                   // font width in pixel
#define FONT_H          22                   // font height in pixel

#elif HAVE_XBM_FONT
#define FONT_W          16                   // use hardcoded xbm_font
#define FONT_H          16

#endif

// common
#define SHADOW_PX       2                    // lower-right text shadow in pixel
#define FONT_D          1                    // distance to next char
#define BORD_D          4                    // distance from canvas border

// additional png bitmaps
#define PNG_MAX         4

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
#define ARGB(a, r, g, b) ( \
          (((a) &0xFF) <<24) | (((r) &0xFF) <<16) | \
          (((g) &0xFF) << 8) | (((b) &0xFF) << 0))

// extract single component form ARGB color
#define GET_A(color) ((color >>24) &0xFF)
#define GET_R(color) ((color >>16) &0xFF)
#define GET_G(color) ((color >> 8) &0xFF)
#define GET_B(color) ((color >> 0) &0xFF)


#ifdef HAVE_PNG_FONT
void set_foreground_color(uint32_t color);

#else
#define LINEAR_GRADIENT_STEP      ((uint8_t)(FONT_H /2))     // steps we split delta
void update_gradient(const uint32_t *a, const uint32_t *b);

#endif


#ifdef HAVE_SYS_FONT

extern int32_t LINE_HEIGHT;

// font cache
typedef struct _Glyph {
    uint32_t code;                           // char unicode
    CellFontGlyphMetrics metrics;            // glyph metrics
    uint16_t w;                              // image width 
    uint16_t h;                              // image height
    uint8_t *image;                          // addr -> image data
} Glyph;

typedef struct _Bitmap {
    CellFontHorizontalLayout horizontal_layout;   // struct -> horizontal text layout info
    float font_w, font_h;                         // char w/h
    float weight, slant;                          // line weight and char slant
    int32_t distance;                             // distance between chars
    int32_t count;                                // count of current cached glyphs
    int32_t max;                                  // max glyph into this cache
    Glyph glyph[FONT_CACHE_MAX];                  // glyph struct
} Bitmap;

void font_finalize(void);
void set_font(float_t font_w, float_t font_h, float_t weight, int32_t distance);
uint16_t get_render_length(const char *str);

#endif


// graphic buffers
typedef struct _Buffer
{
    uint32_t *addr;        // buffer address
    uint32_t  w;           // buffer width
    uint32_t  h;           // buffer height
} Buffer;
//__attribute__((aligned(16)));


// drawing context
typedef struct _DrawCtx
{
    Buffer   png[PNG_MAX]; // bitmaps
    uint32_t *canvas;      // addr of canvas
    uint32_t *bg;          // addr of background backup

    uint32_t bg_color;     // background color

    #ifdef HAVE_SYS_FONT
    uint32_t *font_cache;  // addr of glyph bitmap cache buffer
    CellFont font;
    CellFontRenderer renderer;
    #endif

    #ifdef HAVE_PNG_FONT
    uint32_t *font;        // addr of decoded png font
    uint32_t fg_color;     // foreground color
    #else
    uint32_t fading_color[LINEAR_GRADIENT_STEP];  // precomputed gradient [0-7]
    #endif

} DrawCtx
__attribute__((aligned(16)));


void init_graphic(void);
void flip_frame(void);
void pause_RSX_rendering(void);

void set_background_color(uint32_t color);

void draw_background(void);
void blend_canvas(void);

int32_t load_png_bitmap(const int32_t idx, const char *path);

int32_t draw_png(const int32_t idx, const int32_t c_x, const int32_t c_y, const int32_t p_x, const int32_t p_y, const int32_t w, const int32_t h);
int32_t draw_png_2x(const int32_t idx, const int32_t c_x, const int32_t c_y, const int32_t p_x, const int32_t p_y, const uint32_t w, const uint32_t h);

void screenshot(const uint8_t mode);


// text
#define LEFT      0   // useless
#define RIGHT     1
#define CENTER    2
uint16_t get_aligned_x(const char *str, const uint8_t alignment);
int32_t print_text(int32_t x, int32_t y, const char *str);


// menu views
typedef struct _menu_palette_ctx
{
    uint32_t c[3];
    uint8_t  max_lines;       // (256 seems enough)
    uint8_t  unused[3];
} menu_palette_ctx
__attribute__((aligned(16)));

#define VIEWS 4               // how many different views
void store_palette(menu_palette_ctx *data, size_t len);
void init_menu_palette(menu_palette_ctx *palette);


// primitives
//void draw_pixel(int32_t x, int32_t y);
//void draw_line(int32_t x, int32_t y, int32_t x2, int32_t y2);
//void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
//void draw_circle(int32_t x_c, int32_t y_c, int32_t r);


#ifdef HAVE_STARFIELD
void draw_stars(void);
#endif


#endif // __BLITT_H__
