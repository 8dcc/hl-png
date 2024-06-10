
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#include "include/image.h"
#include "include/util.h"

Image* image_read_file(const char* filename) {
    /* Open the PNG image as "Read bytes" */
    FILE* fp = fopen(filename, "rb");
    if (!fp)
        return NULL;

    /* Create the PNG read and info structs */
    png_structp png =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fclose(fp);
        return NULL;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        fclose(fp);
        return NULL;
    }

    /*
     * This is the first time I see setjmp() being used. See:
     * https://github.com/8dcc/scratch/blob/64e432982b04af77746152d62d97f3ba640e0f7a/C/testing/setjmp.c
     */
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    /* Allocate the Image structure we will be returning. Has to be freed by
     * the caller with image_free(). */
    Image* image = malloc(sizeof(Image));
    if (!image) {
        fclose(fp);
        return NULL;
    }

    image->w          = png_get_image_width(png, info);
    image->h          = png_get_image_height(png, info);
    image->color_type = png_get_color_type(png, info);

    /* Size in bits of each sample, not pixel. See `image_pixel_bits'. */
    image->bit_depth = png_get_bit_depth(png, info);

    /* Total size in bytes of each row */
    image->byte_pitch = png_get_rowbytes(png, info);

    /* Transform grayscale to RGB, adding alpha channel. Search for
     * "png_get_valid" in the libpng manual. */
    if (image->color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    if (image->color_type == PNG_COLOR_TYPE_GRAY && image->bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    /* If the color type doesn't have an alpha channel, fill with 0xFF */
    if (image->color_type == PNG_COLOR_TYPE_RGB ||
        image->color_type == PNG_COLOR_TYPE_GRAY ||
        image->color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    /* Represent grayscale image as RGB */
    if (image->color_type == PNG_COLOR_TYPE_GRAY ||
        image->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    /* Update the png_info structure to reflect the transformations */
    png_read_update_info(png, info);

    /* This is a double pointer. Whoever decided to typedef a pointer should be
     * shot. */
    png_bytep* rows = malloc(sizeof(png_bytep) * image->h);
    for (int y = 0; y < image->h; y++)
        rows[y] = malloc(image->byte_pitch);

    /* Read the PNG image into the rows array */
    png_read_image(png, rows);

    /* TODO: Assumes the number of pixel bits (image->bit_depth) is aligned to 8
     * (to bytes). To support non-aligned depths, you would need to add some
     * weird bit-shifting in the loop below.
     * In other words, assumes that bytes_per_pixel is the same as
     * image_pixel_samples(image). */
    int bytes_per_pixel = image_pixel_bits(image) / 8;
    size_t total_bytes  = image->w * image->h * bytes_per_pixel;
    image->data         = malloc(total_bytes);

    uint8_t* data = (uint8_t*)image->data;
    for (int y = 0; y < image->h; y++) {
        for (int x = 0; x < image->w; x++) {
            png_bytep pixel = &rows[y][x * bytes_per_pixel];

            /* Iterate each byte of the pixel (e.g. R, G, B, A) */
            for (int byte_i = 0; byte_i < bytes_per_pixel; byte_i++)
                *(data++) = pixel[byte_i];
        }
    }

    /* Free the libpng rows */
    for (int y = 0; y < image->h; y++)
        free(rows[y]);
    free(rows);

    /* Close the file descriptor */
    fclose(fp);

    /* Free all memory allocated by libpng */
    png_destroy_read_struct(&png, &info, NULL);

    /* Return our structure, with the image information and rows */
    return image;
}

void image_free(Image* image) {
    free(image->data);
    free(image);
}

int image_pixel_samples(Image* image) {
    switch (image->color_type) {
        case PNG_COLOR_TYPE_GRAY:
            return 1;
        case PNG_COLOR_TYPE_RGB:
            return 3;
        case PNG_COLOR_TYPE_PALETTE:
            return 1;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            return 2;
        case PNG_COLOR_TYPE_RGB_ALPHA:
            return 4;
        default:
            DIE("Unknown image color type: %d", image->color_type);
    }

    return 0;
}
