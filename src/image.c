
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

    /*------------------------------------------------------------------------*/
    /* See http://www.libpng.org/pub/png/libpng-1.2.5-manual.html#section-3.7 */

    /* Convert "Palette" type to RGB */
    if (image->color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);

        /* TODO: Test if this is accurate */
        image->color_type = PNG_COLOR_TYPE_RGB;
    }

    /* Transform grayscale images of less than 8 bits to 8 bits */
    if (image->color_type == PNG_COLOR_TYPE_GRAY && image->bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    /* If there is transparency information in a tRNS chunk, add alpha
     * channel. */
    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    /* Reduce images with 16-bit samples to 8 bits per sample */
    if (image->bit_depth == 16)
        png_set_strip_16(png);

    /* Use a minimum of 1 byte for padding */
    if (image->bit_depth < 8)
        png_set_packing(png);

    /* If the color type doesn't have an alpha channel, fill with 0xFF. This
     * function is defined below. */
    image_add_alpha(image, png);

    /* Represent grayscale image as RGB */
    if (image->color_type == PNG_COLOR_TYPE_GRAY ||
        image->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    /* Update the png_info structure to reflect the transformations */
    png_read_update_info(png, info);

    /*------------------------------------------------------------------------*/

    /* Assumes the number of pixel bits (image->bit_depth) is aligned to 8 */
    int bytes_per_pixel = image_pixel_bits(image) / 8;

    /* Total size in bytes of each row. Calculate manually instead of using
     * `png_get_rowbytes'. */
    image->byte_pitch = image->w * bytes_per_pixel;

    /* This is a double pointer. Whoever decided to typedef a pointer should be
     * shot. */
    png_bytep* rows = malloc(image->h * sizeof(png_bytep));
    for (int y = 0; y < image->h; y++)
        rows[y] = malloc(image->byte_pitch);

    /* Read the PNG image into the rows array */
    png_read_image(png, rows);

    /* Allocate the one-dimensional byte array for the Image structure */
    size_t total_bytes = image->h * image->byte_pitch;
    image->data        = malloc(total_bytes);

    /* Iterate rows (y), pixels of row (x) and bytes of the pixel (e.g. RGBA) */
    uint8_t* data = (uint8_t*)image->data;
    for (int y = 0; y < image->h; y++)
        for (int x = 0; x < image->byte_pitch; x++)
            *(data++) = rows[y][x];

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

void image_add_alpha(Image* image, png_structp png) {
    switch (image->color_type) {
        case PNG_COLOR_TYPE_RGB: {
            png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
            image->color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        } break;

        case PNG_COLOR_TYPE_GRAY: {
            png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
            image->color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
        } break;

        case PNG_COLOR_TYPE_PALETTE: {
            DIE("Image of type Palette should have been converted to RGBA.");
        } break;

        /* Other types already have alpha channel */
        default:
            break;
    }
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
