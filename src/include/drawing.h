
#ifndef DRAWING_H_
#define DRAWING_H_ 1

#include <stdint.h>

/* Initial value for `Drawing.points_sz', will also be used for resizing
 * `Drawing.points' when needed. */
#define DRAWING_POINTS_SIZE 200

/* Initial value for `Drawing.line_ends_sz', will also be used for resizing
 * `Drawing.line_ends' when needed. */
#define DRAWING_LINES_SIZE 20

#define C(RGBA)                           \
    ((Color){ .r = ((RGBA) >> 24) & 0xFF, \
              .g = ((RGBA) >> 16) & 0xFF, \
              .b = ((RGBA) >> 8) & 0xFF,  \
              .a = (RGBA)&0xFF })

typedef struct Color {
    uint8_t r, g, b, a;
} Color;

typedef struct DrawingPoint {
    /* Point position relative to the center of the window */
    int x, y;

    /* Color of the point.
     * TODO: Store color of line instead of point to save memory? */
    Color col;
} DrawingPoint;

typedef struct Drawing {
    /* Stack of points */
    DrawingPoint* points;

    /* Number of elements that the `points' array can hold */
    int points_sz;

    /* Current position inside the `points' array */
    int points_i;

    /* Each element in the `line_ends' array contains the index inside `points'
     * where a line started. See `drawing_end_line' function. */
    int* line_ends;

    /* Number of elements that the `line_ends' array can hold */
    int line_ends_sz;

    /* Number of lines we have drawn */
    int line_count;
} Drawing;

/*----------------------------------------------------------------------------*/

/* Allocate a new Drawing. It must be freed by the caller with `drawing_free' */
Drawing* drawing_new(void);

/* Free a Drawing structure */
void drawing_free(Drawing* drawing);

/* Push a point to the `drawing->points' stack */
void drawing_push(Drawing* drawing, DrawingPoint point);

/* Store that the current line in the drawing has ended. The next points will
 * belong to a different line. */
void drawing_end_line(Drawing* drawing);

/* Store the user click in absolute position (X,Y) into the specified Drawing,
 * relative to the center of the window. */
void drawing_store_from_center(Drawing* drawing, int x, int y, Color col);

#endif /* DRAWING_H_ */
