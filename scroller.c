#include "scroller.h"
#include "inc/vsh_exports.h"
#include "blitting.h"

// testing f_sinf() export
// masterzorag, 2015

#define STEP_X   -4           // horizontal displacement
#define MSG_LEN  64 *4        // we save 2 strlen(), also it's 256!


static int32_t  sx  = CANVAS_W;
static uint16_t len = 0;

/* 256 characters, 64 * 4 lines or switch from uint8_t i below */
static char msg[MSG_LEN] __attribute__((aligned(16))) = "\
    Hi Folks, now we have a Sinusscroller greeter too !!! also: \
shadowed, color text         * * *          Greets goes to: play\
stationhax.it, psx-place.com and all the ps3 devs around !      \
                      * peace and love *                       .";
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
    c[1] = '\0';              // functions deals with '\0' terminated

    for(i = 0; i < MSG_LEN -1; i++)
    {
        c[0] = msg[i];        // split text into single characters

        if(x > 0 && x < CANVAS_W - FONT_W)
        {
            amp = f_sinf(x    // testing f_sinf() export
                * 0.02)       // it turns out in num of bends
                * 20;         // +/- vertical bounds over y

            x = print_text(x, y + amp, c);
        } else {
            #ifdef HAVE_SYS_FONT
            x += get_render_length(c);

            #else
            x += FONT_W;

            #endif
        }
    }
}


/***********************************************************************
* Move string by defined step
***********************************************************************/
void move_text(void)
{
    if(len == 0)  // init, once
    {
        msg[MSG_LEN -1] = '\0';

        #ifdef HAVE_SYS_FONT
        len = get_render_length(msg);

        #else
        len = MSG_LEN * FONT_W;

        #endif
    }

    sx += STEP_X;

    if(sx < -(uint16_t)len)   // horizontal bound, then restart loop
        sx = CANVAS_W + FONT_W;
}
