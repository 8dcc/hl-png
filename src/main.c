
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#include "include/main.h"
#include "include/util.h"
#include "include/image.h"

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

static bool g_draw_grid = true;

/*----------------------------------------------------------------------------*/
/* SDL helper functions */

static inline void set_render_color(SDL_Renderer* rend, uint32_t col) {
    const uint8_t r = (col >> 16) & 0xFF;
    const uint8_t g = (col >> 8) & 0xFF;
    const uint8_t b = (col >> 0) & 0xFF;
    const uint8_t a = 255;
    SDL_SetRenderDrawColor(rend, r, g, b, a);
}

static void draw_grid(void) {
    if (!g_draw_grid)
        return;

    int win_w, win_h;
    SDL_GetWindowSize(g_window, &win_w, &win_h);

    const int step = GRID_STEP + 1;
    for (int y = GRID_STEP; y < win_h; y += step)
        SDL_RenderDrawLine(g_renderer, 0, y, win_w, y);

    for (int x = GRID_STEP; x < win_w; x += step)
        SDL_RenderDrawLine(g_renderer, x, 0, x, win_h);
}

static void draw_image(Image* image, SDL_Texture* texture, int center_x,
                       int center_y) {
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

/*----------------------------------------------------------------------------*/
/* Main function */

int main(int argc, char** argv) {
    if (argc < 2)
        DIE("Usage: %s file.png", argv[0]);

    const char* filename = argv[1];
    Image* image         = image_read_file(filename);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
        DIE("Unable to start SDL.");

    /* Create SDL window */
    const int window_w = image->w;
    const int window_h = image->h;
    g_window =
      SDL_CreateWindow("hl-png", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       window_w, window_h, SDL_WINDOW_RESIZABLE);
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

                        case SDL_SCANCODE_G: {
                            g_draw_grid = !g_draw_grid;
                        } break;

                        default:
                            break;
                    }
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
        draw_grid();

        int win_w, win_h;
        SDL_GetWindowSize(g_window, &win_w, &win_h);

        draw_image(image, image_texture, win_w / 2, win_h / 2);

        /* Send to renderer and delay depending on FPS */
        SDL_RenderPresent(g_renderer);
        SDL_Delay(1000 / FPS);
    }

    SDL_FreeSurface(image_surface);
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    image_free(image);

    return 0;
}
