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

#include "starfield.h"
#include "inc/vsh_exports.h"      // testing rand()
#include "blitting.h"

#define NUMBER_OF_STARS 256*4     // max 2^16 for uint16_t


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
    //star->color = i >> 2;
    star->color = i * 20;
}

void init_once(/* stars, first run */)
{
  uint16_t i;
  for(i = 0; i < NUMBER_OF_STARS; i++) 
    init_star(stars +i, i +1);
}

void move_star(uint32_t *canvas)
{
    int16_t tx, ty;
    uint16_t i;

    for(i = 0; i < NUMBER_OF_STARS; i++)
    {
        stars[i].zpos -= stars[i].speed;

        if(stars[i].zpos <= 0) init_star(stars +i, i +1);

        /* compute 3D position */
        tx = (stars[i].xpos / stars[i].zpos) + (CANVAS_W >>1);
        ty = (stars[i].ypos / stars[i].zpos) + (CANVAS_H >>1);

        /* check if a star leaves the screen */
        if(tx < 0 || tx > CANVAS_W -1
        || ty < 0 || ty > CANVAS_H -1)
        {
            init_star(stars +i, i +1);
            continue;
        }

        canvas[tx + ty * CANVAS_W] = ARGB(0xff, stars[i].color, stars[i].color, stars[i].color);
    }
}
