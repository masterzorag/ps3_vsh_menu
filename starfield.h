#ifndef __STARFLD_H__
#define __STARFLD_H__

#include <sys/types.h>

typedef struct _STAR{
    float     xpos;
    float     ypos;
    uint8_t   color;
    uint16_t  zpos;
    uint16_t  speed;
} STAR
__attribute__((aligned(16)));

void init_star(STAR *star, const uint16_t i);
void init_once(void);
void move_star(uint32_t *canvas);

#endif // __STARFLD_H__
