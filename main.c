#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"

// Example usage:
//      mpirun -n 4 morph data/bush.png data/schwarzenegger.png data/morphed.png

const char* DEFAULT_OUTPUT_PATH = "./morphed.png";

int main(int argc, char** argv) {
    
    if (argc < 3) {
        printf("Error: missing the 2 requried positional arguments (image filepaths).\n");
        exit(-1);
    }

    Image image1, image2;
    if(!load_resized(argv[1], argv[2], &image1, &image2)) {
        printf("Error loading/resizing images.\n");
        exit(-1);
    }

    Image* morphed = new_image(image1.width, image1.height, image1.channels);
    double alpha = argc >= 5 ? strtod(argv[4], NULL) : IMAGES_DEFAULT_ALPHA;
    if (!morph_images_parallel(image1, image2, alpha, morphed)) {
        printf("Error morphing images.\n");
        exit(-1);
    }

    const char* output_filepath = argc >= 4 ? argv[3] : DEFAULT_OUTPUT_PATH;
    if (!save_image(*morphed, output_filepath)) {
        printf("Error saving morphed image.\n");
        exit(-1);
    }

    free_image(&image1);
    free_image(&image2);
    free_image(morphed);
    free(morphed);

    return 0;
}