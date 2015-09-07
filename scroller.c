#include "scroller.h"
#include "inc/vsh_exports.h"
#include "blitting.h"

// testing f_sinf() export
// masterzorag, 2015

#define STEP_X  -4            // horizontal displacement
#define S_LEN   64 *4         // we save 2 strlen(), also it's 256!


static int32_t sx = CANVAS_W;

/* 256 characters, 64 * 4 lines or switch from uint8_t i below */
static const char ss[S_LEN] = "\
    HI FOLKS, NOW WE HAVE A sinuscroller GREETER TOO !!! also: s\
hadowed, color text          * * *          Greets goes to: play\
stationhax.it, psx-place.com and all the ps3 devs around !      \
           * peace and love *                                  .";
/*-------------------------------------------------------------*/

/***********************************************************************
* Draw a string of chars, amplifing y position by sin(x)
***********************************************************************/
void draw_text(const int y)
{
    uint8_t i;                // max 64*4 characters message !
    float_t amp;
    int16_t x = sx;           // every call sets the initial x
    char   c[2];
    c[1] = '\0';              // print_text() deals with '\0' terminated

    for(i = 0; i < S_LEN -1; i++)
    {
        if(x > 0 && x < CANVAS_W - FONT_W)
        {
            amp = f_sinf(x    // testing f_sinf() export
              * 0.02)         // it turns out in num of bends
              * 20;           // +/- vertical bounds over y

            c[0] = ss[i];     // split text into single characters

            print_text(x, y + amp, c);
        }
        x += FONT_W;
    }
}


/***********************************************************************
* Move string by defined step
***********************************************************************/
void move_text(void)
{
    sx += STEP_X;

    if(sx < -(uint16_t)(S_LEN * FONT_W))  // horizontal bound, then loop
        sx = CANVAS_W + FONT_W;
}
