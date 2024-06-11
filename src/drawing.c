
#include <stdlib.h>

#include "include/main.h"
#include "include/util.h"
#include "include/drawing.h"

static void realloc_from_center(Drawing* drawing, int new_w, int new_h) {
    const int old_w = drawing->w;
    const int old_h = drawing->h;

    int x_offset = 0;
    int y_offset = 0;

    /* Calculate difference between old width and height. If it's positive,
     * calculate the offsets for centering, and update the values in the
     * Drawing. */
    const int w_diff = new_w - old_w;
    if (w_diff > 0) {
        x_offset   = w_diff / 2;
        drawing->w = new_w;
    }

    const int h_diff = new_h - old_h;
    if (h_diff > 0) {
        y_offset   = h_diff / 2;
        drawing->h = new_h;
    }

    /* Width and height probably changed, recalculate total size and allocate
     * new array. */
    const size_t new_sz = drawing->w * drawing->h;
    Color* new_data     = calloc(new_sz, sizeof(Color));

    /* Copy the old array, adding the X and Y offsets */
    for (int y = 0; y < old_h; y++) {
        for (int x = 0; x < old_w; x++) {
            const int new_idx = drawing->w * (y + y_offset) + (x + x_offset);
            const int old_idx = old_w * y + x;
            new_data[new_idx] = drawing->data[old_idx];
        }
    }

    /* Update the `data' pointer in the Drawing */
    free(drawing->data);
    drawing->data = new_data;
}

/*----------------------------------------------------------------------------*/

Drawing* drawing_new(int w, int h) {
    Drawing* drawing = malloc(sizeof(Drawing));
    drawing->w       = w;
    drawing->h       = h;
    drawing->data    = calloc(drawing->w * drawing->h, sizeof(Color));
    return drawing;
}

void drawing_free(Drawing* drawing) {
    free(drawing->data);
    free(drawing);
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

    /* Get central position of the array */
    const int arr_center_x = drawing->w / 2;
    const int arr_center_y = drawing->h / 2;

    /* Minimum width and height that the array must have to include `arr_rel_x'
     * and `arr_rel_y'. */
    const int min_w = arr_center_x + (ABS(win_rel_x) * 2);
    const int min_h = arr_center_y + (ABS(win_rel_y) * 2);

    /* If the position is out of bounds, reallocate the array */
    if (drawing->w < min_w || drawing->h < min_h)
        realloc_from_center(drawing, min_w, min_h);

    /* Get the real array positions */
    const int arr_x = arr_center_x + win_rel_x;
    const int arr_y = arr_center_y + win_rel_y;

    drawing->data[drawing->w * arr_y + arr_x] = col;
}
