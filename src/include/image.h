
#ifndef IMAGE_H_
#define IMAGE_H_ 1

#include <png.h>

typedef struct Image {
    void* data;
    int w, h;
    int color_type;
    int bit_depth;
    int byte_pitch;
} Image;

/* Read a PNG file, and return a Image structure. Returned structure must be
 * freed by the caller. */
Image* image_read_file(const char* filename);

/* Free an Image structure */
void image_free(Image* image);

/* Add an alpha channel for color types that don't have it, and update the color
 * type. */
void image_add_alpha(Image* image, png_structp png);

/* Get the number of samples per pixel depending on the `color_type' of the
 * Image. */
int image_pixel_samples(Image* image);

/* Get the bits per pixel of the specified Image, based on the `bit_depth' and
 * `color_type' attributes. */
static inline int image_pixel_bits(Image* image) {
    return image->bit_depth * image_pixel_samples(image);
}

#endif /* IMAGE_H_ */
