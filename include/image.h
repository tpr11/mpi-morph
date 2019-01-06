#ifndef IMAGE_H
#define IMAGE_H

#include <mpi.h>

typedef struct {
    int width, height, channels;
    unsigned char* data;
} Image;

const int ROOT;
const double IMAGES_DEFAULT_ALPHA;

Image* new_image(int width, int height, int channels);
int load_image(const char* filepath, Image* image);
int resize_image(Image* image, int new_width, int new_height);
int load_resized(const char* filename1, const char* filename2, Image* image1, Image* image2);
int image_size(const Image image);
int save_image(const Image image, const char* filepath);
int imgcmp(Image image1, Image image2);
int morph_images_parallel(const double alpha, const Image* image1, const Image* image2, Image* result);
void free_image(Image* image);

#endif // IMAGE_H