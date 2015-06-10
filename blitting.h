#ifndef __BLITT_H__
#define __BLITT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


// font constants
#define PNG_FONT_PATH "/dev_hdd0/font.png"  
#define FONT_PNG_W    512                    	// width of font png file in pixel
#define FONT_PNG_H    514                    	// height of font png file in pixel
#define FONT_W        18         		// font width in pixel
#define FONT_H        22           		// font height in pixel


// canvas constants
// the values for canvas width and height can be changed for make a smaller or larger
// menu. They must be smaller than the max resulution of 1920*1080 and additional
// code is necessary to manage the other possible resolutions.
// Due to the fact that we read and write 64bit values(2 pixel) at once to the framebuffer,
// CANVAS_W must be a multiple of 2.
#define BASE          0xC0000000UL           	// local memory base ea
#define CANVAS_W      720       		// canvas width in pixel
#define CANVAS_H      400         		// canvas height in pixel


// get pixel offset into framebuffer by x/y coordinates
#define OFFSET(x, y) ((((uint32_t)offset) + ((((int16_t)x) + \
                     (((int16_t)y) * (((uint32_t)pitch) / \
                     ((int32_t)4)))) * ((int32_t)4))) + (BASE))


#define PNG_MAX    4            // additional png bitmaps


// graphic buffers
typedef struct _Buffer{
	uint32_t *addr;			// buffer address
	int32_t  w;                   	// buffer width
	int32_t  h;                   	// buffer height
} Buffer;


// drawing context
typedef struct _DrawCtx{
	uint32_t *canvas;             // addr of canvas
	uint32_t *bg;                 // addr of background backup
	uint32_t *font;               // addr of decoded png font 
	Buffer   png[PNG_MAX];        // bitmaps
	uint32_t bg_color;            // background color
	uint32_t fg_color;            // foreground color
} DrawCtx;


void pause_RSX_rendering(void);

void init_graphic(void);
int32_t load_png_bitmap(int32_t idx, const char *path);
void flip_frame(void);
void set_background_color(uint32_t color);
void set_foreground_color(uint32_t color);
void draw_background(void);
void print_text(int32_t x, int32_t y, const char *str);
void draw_png(int32_t idx, int32_t can_x, int32_t can_y, int32_t png_x, int32_t png_y, int32_t w, int32_t h);

void screenshot(uint8_t mode);

//void draw_pixel(int32_t x, int32_t y);
//void draw_line(int32_t x, int32_t y, int32_t x2, int32_t y2);
//void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
//void draw_circle(int32_t x_c, int32_t y_c, int32_t r);


#endif // __BLITT_H__ 
