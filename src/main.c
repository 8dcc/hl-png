
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#include "include/main.h"
#include "include/util.h"
#include "include/image.h"
#include "include/drawing.h"

#define FPS 60

#define GRID_STEP 10

#define COLOR_GRID 0x111111

/* RGBA masks change depending on endianness */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define MASK_R 0xFF000000
#define MASK_G 0x00FF0000
#define MASK_B 0x0000FF00
#define MASK_A 0x000000FF
#else
#define MASK_R 0x000000FF
#define MASK_G 0x0000FF00
#define MASK_B 0x00FF0000
#define MASK_A 0xFF000000
#endif

/*----------------------------------------------------------------------------*/
/* Globals */

SDL_Window* g_window     = NULL;
SDL_Renderer* g_renderer = NULL;

static bool g_render_grid = true;

static bool g_drawing          = false; /* Holding LMouse */
static bool g_on_straight_mode = false; /* Holding Ctrl */

/*----------------------------------------------------------------------------*/
/* SDL helper functions */

static inline void set_render_color(SDL_Renderer* rend, uint32_t col) {
    const uint8_t r = (col >> 16) & 0xFF;
    const uint8_t g = (col >> 8) & 0xFF;
    const uint8_t b = (col >> 0) & 0xFF;
    const uint8_t a = 255;
    SDL_SetRenderDrawColor(rend, r, g, b, a);
}

/* Render a subtle grid */
static void render_grid(void) {
    if (!g_render_grid)
        return;

    int win_w, win_h;
    SDL_GetWindowSize(g_window, &win_w, &win_h);

    const int step = GRID_STEP + 1;
    for (int y = GRID_STEP; y < win_h; y += step)
        SDL_RenderDrawLine(g_renderer, 0, y, win_w, y);

    for (int x = GRID_STEP; x < win_w; x += step)
        SDL_RenderDrawLine(g_renderer, x, 0, x, win_h);
}

/* Render a texture, centered in the window */
static void render_image(Image* image, SDL_Texture* texture) {
    int win_w, win_h;
    SDL_GetWindowSize(g_window, &win_w, &win_h);
    const int center_x = win_w / 2;
    const int center_y = win_h / 2;

    const SDL_Rect src_rect = {
        0,
        0,
        image->w,
        image->h,
    };
    const SDL_Rect dst_rect = {
        center_x - (image->w / 2),
        center_y - (image->h / 2),
        image->w,
        image->h,
    };

    SDL_RenderCopy(g_renderer, texture, &src_rect, &dst_rect);
}

/* Render a line using `Drawing.points', from `start_idx' to `end_idx'
 * (inclusive). */
static void render_drawing_line(Drawing* drawing, int start_idx, int end_idx) {
    int win_w, win_h;
    SDL_GetWindowSize(g_window, &win_w, &win_h);
    const int center_x = win_w / 2;
    const int center_y = win_h / 2;

    for (int i = start_idx + 1; i <= end_idx; i++) {
        DrawingPoint a = drawing->points[i - 1];
        DrawingPoint b = drawing->points[i];
        Color col      = a.col;

        /* Since the points are stored relative to the center of the window,
         * we convert them to the absolute positions here. */
        const int src_x = center_x + a.x;
        const int src_y = center_y + a.y;
        const int dst_x = center_x + b.x;
        const int dst_y = center_y + b.y;

        SDL_SetRenderDrawColor(g_renderer, col.r, col.g, col.b, col.a);
        SDL_RenderDrawLine(g_renderer, src_x, src_y, dst_x, dst_y);
    }
}

/* Render a drawing, centered in the window */
static void render_drawing(Drawing* drawing) {
    /* Iterate each line in the drawing */
    for (int line = 1; line <= drawing->line_count; line++) {
        /* The first line starts at point zero, rest start at the point next to
         * where the previous line ended. */
        const int start_idx =
          (line == 1) ? 0 : drawing->line_ends[line - 1] + 1;
        const int end_idx = drawing->line_ends[line];

        render_drawing_line(drawing, start_idx, end_idx);
    }

    /* If we are currently drawing a line, it's not stored in
     * `Drawing.line_ends' yet. */
    if (drawing_in_progress(drawing)) {
        const int start_idx = (drawing->line_count == 0)
                                ? 0
                                : drawing->line_ends[drawing->line_count] + 1;
        const int end_idx   = drawing->points_i - 1;

        render_drawing_line(drawing, start_idx, end_idx);
    }
}

/*----------------------------------------------------------------------------*/
/* Main function */

int main(int argc, char** argv) {
    if (argc < 2)
        DIE("Usage: %s [...] file.png", argv[0]);

    /* Parse arguments */
    bool arg_fullscreen = false;
    bool arg_fixed      = false;
    for (int i = 1; i < argc - 1; i++) {
        if (argv[i][0] != '-')
            continue;

        for (int j = 1; argv[i][j] != '\0'; j++) {
            switch (argv[i][j]) {
                case 'f': {
                    arg_fullscreen = true;
                } break;

                case 'F': {
                    arg_fixed = true;
                } break;

                case 'h': {
                    printf("Usage:\n"
                           "  %s [-fF] file.png\n"
                           "Arguments:\n"
                           "  -f\tLaunch in full-screen mode.\n"
                           "  -F\tLaunch in fixed mode.\n"
                           "  -h\tPrint this help and exit.\n",
                           argv[0]);
                    exit(0);
                } break;

                default:
                    break;
            }
        }
    }

    /* Last argument must be the image path */
    const char* filename = argv[argc - 1];
    Image* image         = image_read_file(filename);

    /*------------------------------------------------------------------------*/
    /* SDL initialization */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
        DIE("Unable to start SDL.");

    /* Use different window flags depending on arguments */
    int window_flags = 0;
    if (arg_fullscreen)
        window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (!arg_fixed)
        window_flags |= SDL_WINDOW_RESIZABLE;

    /* Create SDL window */
    const int window_w = image->w;
    const int window_h = image->h;
    g_window =
      SDL_CreateWindow("hl-png", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       window_w, window_h, window_flags);
    if (!g_window)
        DIE("Error creating SDL window.");

    /* Create SDL renderer */
    g_renderer =
      SDL_CreateRenderer(g_window, -1,
                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer) {
        SDL_DestroyWindow(g_window);
        DIE("Error creating SDL renderer.");
    }

#ifdef SCALE_QUALITY
    /* Use the best scaling quality of the texture */
    if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best") != SDL_TRUE)
        DIE("Could not set RENDER_SCALE_QUALITY hint.");
#endif

    /* Create the surface and texture for the image */
    SDL_Surface* image_surface =
      SDL_CreateRGBSurfaceFrom(image->data, image->w, image->h,
                               image_pixel_bits(image), image->byte_pitch,
                               MASK_R, MASK_G, MASK_B, MASK_A);
    if (!image_surface)
        DIE("Error creating RGBA surface from PNG data.");

    SDL_Texture* image_texture =
      SDL_CreateTextureFromSurface(g_renderer, image_surface);
    if (!image_texture)
        DIE("Error creating texture from RGBA surface.");

    /* Allocate the main Drawing structure */
    Drawing* drawing = drawing_new();

    /*------------------------------------------------------------------------*/
    /* Main loop */
    bool running = true;
    while (running) {
        /* Parse SDL events */
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    running = false;
                } break;

                case SDL_KEYDOWN: {
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_ESCAPE:
                        case SDL_SCANCODE_Q: {
                            running = false;
                        } break;

                        case SDL_SCANCODE_LCTRL:
                        case SDL_SCANCODE_RCTRL: {
                            g_on_straight_mode = true;
                        } break;

                        case SDL_SCANCODE_G: {
                            g_render_grid = !g_render_grid;
                        } break;

                        case SDL_SCANCODE_C: {
                            drawing_clear(drawing);
                        } break;

                        case SDL_SCANCODE_F11:
                        case SDL_SCANCODE_F: {
                            /* Toggle fullscreen */
                            uint32_t new_flags =
                              (SDL_GetWindowFlags(g_window) &
                               SDL_WINDOW_FULLSCREEN_DESKTOP)
                                ? 0
                                : SDL_WINDOW_FULLSCREEN_DESKTOP;

                            SDL_SetWindowFullscreen(g_window, new_flags);
                        } break;

                        default:
                            break;
                    }
                } break;

                case SDL_KEYUP: {
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_LCTRL:
                        case SDL_SCANCODE_RCTRL: {
                            /* Try to end the line when releasing Ctrl. The
                             * function checks if there is a line to end. */
                            drawing_end_line(drawing);

                            g_on_straight_mode = false;
                        } break;

                        default:
                            break;
                    }
                } break;

                case SDL_MOUSEBUTTONDOWN: {
                    switch (event.button.button) {
                        case SDL_BUTTON_LEFT: {
                            /* TODO: Change colors, brush type, etc. */
                            g_drawing = true;

                            /* Store first point of the drawing. Next ones will
                             * be stored in SDL_MOUSEMOTION. */
                            drawing_store_from_center(drawing, event.motion.x,
                                                      event.motion.y,
                                                      C(0xFF0000FF));
                        } break;

                        default:
                            break;
                    }
                } break;

                case SDL_MOUSEBUTTONUP: {
                    switch (event.button.button) {
                        case SDL_BUTTON_LEFT: {
                            /* Released click, end the line we were drawing */
                            g_drawing = false;

                            if (!g_on_straight_mode)
                                drawing_end_line(drawing);
                        } break;

                        default:
                            break;
                    }
                } break;

                case SDL_MOUSEMOTION: {
                    if (!g_drawing)
                        break;

                    drawing_store_from_center(drawing, event.motion.x,
                                              event.motion.y, C(0xFF0000FF));
                } break;

                default:
                    break;
            }
        }

        /* Clear window */
        set_render_color(g_renderer, 0x000000);
        SDL_RenderClear(g_renderer);

        /* Draw background grid */
        set_render_color(g_renderer, COLOR_GRID);
        render_grid();

        render_image(image, image_texture);

        render_drawing(drawing);

        /* Send to renderer and delay depending on FPS */
        SDL_RenderPresent(g_renderer);
        SDL_Delay(1000 / FPS);
    }

    SDL_DestroyTexture(image_texture);
    SDL_FreeSurface(image_surface);
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    drawing_free(drawing);
    image_free(image);

    return 0;
}
