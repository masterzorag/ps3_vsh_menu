#include "scroller.h"
#include "inc/vsh_exports.h"
#include "blitting.h"

// testing f_sinf() export
// masterzorag, 2015

#define STEP_X  -4         // horizontal displacement

static const char s[] = "   HI FOLKS, NOW WE HAVE A sinuscroller GREETER TOO !!! also: shadowed, color text          * * *          Greets goes to: playstationhax.it, psx-place.com and all the ps3 devs around !                  * peace and love *";
static int32_t sx = CANVAS_W;


/***********************************************************************
* Draw a string of chars, amplifing y by sin(x)
***********************************************************************/
void draw_text(const int y)
{
    uint16_t i;
    int32_t x = sx;       // every call sets the initial x
    float_t amp;
    char c[2];

    c[1] = '\0';          // print_text() deals with '\0' terminated

    for(i = 0; i < strlen(s); i++)
    {
        amp = f_sinf(x    // testing f_sinf() export
              * 0.02)     // it turns out in num of bends
              * 20;       // +/- vertical bounds over y

        c[0] = s[i];      // split text into single characters

        if(x > 0
        && x < CANVAS_W - FONT_W)
          print_text(x, y + amp, c);

        x += FONT_W;
    }
}


/***********************************************************************
* Move string by defined step
***********************************************************************/
void move_text(void)
{
    uint16_t l = strlen(s) * FONT_W;
    sx += STEP_X;

    if(sx < -l)           // horizontal bound, then loop
        sx = CANVAS_W + FONT_W;
}
