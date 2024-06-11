
#ifndef DRAWING_H_
#define DRAWING_H_ 1

#include <stdint.h>

#define C(RGBA)                           \
    ((Color){ .r = ((RGBA) >> 24) & 0xFF, \
              .g = ((RGBA) >> 16) & 0xFF, \
              .b = ((RGBA) >> 8) & 0xFF,  \
              .a = (RGBA)&0xFF })

typedef struct Color {
    uint8_t r, g, b, a;
} Color;

typedef struct Drawing {
    int w, h;
    Color* data;
} Drawing;

/*----------------------------------------------------------------------------*/

/* Allocate a new Drawing with the specified width and height */
Drawing* drawing_new(int w, int h);

/* Free a Drawing structure */
void drawing_free(Drawing* drawing);

/* Store the user click in absolute position (X,Y) into the specified Drawing,
 * relative to the center of the window. */
void drawing_store_from_center(Drawing* drawing, int x, int y, Color col);

#endif /* DRAWING_H_ */
