
#include <stdlib.h>

#include "include/main.h"
#include "include/util.h"
#include "include/drawing.h"

Drawing* drawing_new(void) {
    Drawing* drawing = malloc(sizeof(Drawing));

    drawing->points_sz = DRAWING_POINTS_SIZE;
    drawing->points    = calloc(drawing->points_sz, sizeof(DrawingPoint));
    drawing->points_i  = 0;

    drawing->line_ends_sz = DRAWING_LINES_SIZE;
    drawing->line_ends    = calloc(drawing->line_ends_sz, sizeof(int));
    drawing->line_count   = 0;

    return drawing;
}

void drawing_free(Drawing* drawing) {
    free(drawing->points);
    free(drawing->line_ends);
    free(drawing);
}

void drawing_push(Drawing* drawing, DrawingPoint point) {
    /* If there is no space left, reallocate */
    if (drawing->points_i >= drawing->points_sz) {
        drawing->points_sz += DRAWING_POINTS_SIZE;
        drawing->points =
          realloc(drawing->points, drawing->points_sz * sizeof(DrawingPoint));
    }

    /*
     * Push to the DrawingPoint stack.
     *
     * NOTE: We could avoid using `Drawing.points_i', by using the
     * `Drawing.line_ends' array along with `Drawing.line_count' directly.
     * However, it was harder to understand, and I don't think it's worth it.
     */
    drawing->points[drawing->points_i++] = point;
}

bool drawing_in_progress(Drawing* drawing) {
    const int last_line_end = drawing->line_ends[drawing->line_count];
    return drawing->points_i > last_line_end + 1;
}

void drawing_end_line(Drawing* drawing) {
    /* Should never happen */
    if (drawing->points_i <= 0)
        return;

    /* We are not drawing, there is no line to end */
    if (!drawing_in_progress(drawing))
        return;

    /* If the `Drawing.line_ends' array is not big enough to hold the next line,
     * reallocate */
    if (drawing->line_count >= drawing->line_ends_sz) {
        drawing->line_ends_sz += DRAWING_LINES_SIZE;
        drawing->line_ends =
          realloc(drawing->line_ends, drawing->line_ends_sz * sizeof(int));
    }

    /*
     * The drawing starts like this:
     *
     *     line_ends: [0, ...]
     *     line_count: 0
     *
     * After the first call to `drawing_end_line', the count increases and the
     * array stores the last point.
     *
     *     line_ends: [0, 15, ...]
     *     line_count: 1
     *
     * Now we have a line from 0 to 15. The first item should always be zero,
     * and indicates the start of a line. They update in the next call:
     *
     *     line_ends: [0, 15, 21, ...]
     *     line_count: 2
     *
     * Now we have a line from 0 to 15, and a line from 16 to 21. The 15th and
     * 16th point won't be connected when rendering.
     */

    /* Increase the number of lines we have drawn */
    drawing->line_count++;

    /* Store the last index of `Drawing.points' where the last line ended. Keep
     * in mind that `Drawing.points_i' points to the index for the next
     * insertion, not the index of the last one. */
    drawing->line_ends[drawing->line_count] = drawing->points_i - 1;
}

void drawing_store_from_center(Drawing* drawing, int x, int y, Color col) {
    int win_w, win_h;
    SDL_GetWindowSize(g_window, &win_w, &win_h);

    /*
     * Divide window into 4 regions, get position relative to center:
     *
     *         |
     *   x-,y- | x+,y-
     *   ------+-------
     *   x-,y+ | x+,y+
     *         |
     *
     */
    const int win_center_x = win_w / 2;
    const int win_center_y = win_h / 2;
    const int win_rel_x    = x - win_center_x;
    const int win_rel_y    = y - win_center_y;

    DrawingPoint point = {
        .x   = win_rel_x,
        .y   = win_rel_y,
        .col = col,
    };

    drawing_push(drawing, point);
}

void drawing_clear(Drawing* drawing) {
    drawing->points_i   = 0;
    drawing->line_count = 0;
}
