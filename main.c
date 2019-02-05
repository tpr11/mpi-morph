#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"

// Example usage:
//      mpirun -n 4 morph data/bush.png data/schwarzenegger.png data/morphed.png 0.5

const char* DEFAULT_OUTPUT_PATH = "./morphed.png";

int main(int argc, char** argv) {
    
    MPI_Init(NULL, NULL);
    
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    const double alpha = argc >= 5 ? strtod(argv[4], NULL) : IMAGES_DEFAULT_ALPHA;

    if (rank == ROOT) {
        if (argc < 3) {
            printf("Error: missing the 2 requried positional arguments (image filepaths).\n");
            exit(1);
        }
        Image image1, image2;
        if(load_resized(argv[1], argv[2], &image1, &image2)) {
            printf("Error loading/resizing images.\n");
            exit(1);
        }
        Image* morphed = new_image(image1.width, image1.height, image1.channels);
        if (morph_images_parallel(alpha, &image1, &image2, morphed)) {
            printf("Error morphing images from ROOT.\n");
            exit(1);
        }
        const char* output_filepath = argc >= 4 ? argv[3] : DEFAULT_OUTPUT_PATH;
        if (save_image(*morphed, output_filepath)) {
            printf("Error saving morphed image.\n");
            exit(1);
        }
        free_image(&image1);
        free_image(&image2);
        free_image(morphed);
        free(morphed);
    }
    else {
        if (morph_images_parallel(alpha, NULL, NULL, NULL)) {
            printf("Error morphing images from proc %d.\n", rank);
            exit(-1);
        }
    }

    MPI_Finalize();

    return 0;
}