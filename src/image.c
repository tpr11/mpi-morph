#include <assert.h>
#include <unistd.h>

#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

const int ROOT = 0;
const double IMAGES_DEFAULT_ALPHA = 0.5;

Image* new_image(int width, int height, int channels) {
    Image* image = (Image*) malloc(sizeof(Image));
    image->width = width;
    image->height = height;
    image->channels = channels;
    image->data = (unsigned char*) calloc(image_size(*image), sizeof(unsigned char));
    return image;
}

int load_image(const char* filepath, Image* image) {
    int n = 3;
    unsigned char *i = stbi_load(filepath, &image->width, &image->height, &n, 0);
    if (!i) {
        return 1;
    }
    image->data = i;
    image->channels = n;
    return 0;
}

int resize_image(Image* image, int new_width, int new_height) {
    unsigned char* new_data = (unsigned char*) malloc(new_width * new_height * image->channels);
    int return_code = !stbir_resize_uint8(image->data, image->width, image->height, 0, 
                                          new_data, new_width, new_height, 0, image->channels);
    unsigned char* aux = image->data;
    image->data = new_data;
    image->width = new_width;
    image->height = new_height;
    free(aux);
    return return_code;
}

int load_resized(const char* filename1, const char* filename2, Image* image1, Image* image2) {
    if (load_image(filename1, image1)) {
        return 1;
    }
    if (load_image(filename2, image2)) {
        return 1;
    }
    int lowest_width = image1->width < image2->width ? image1->width : image2->width;
    int lowest_height = image1->height < image2->height ? image1->height : image2->height;
    resize_image(image1, lowest_width, lowest_height);
    resize_image(image2, lowest_width, lowest_height);
    return 0;
}

int image_size(const Image image) {
    return image.width * image.height * image.channels;
}

int save_image(const Image image, const char* filepath) {
    return !stbi_write_png(filepath, image.width, image.height, image.channels, image.data, 0);
}

void assert_equal_size(const Image image1, const Image image2) {
    assert(image1.width == image2.width);
    assert(image1.height == image2.height);
    assert(image1.channels == image2.channels);  
}

int imgcmp (Image image1, Image image2) {
    assert_equal_size(image1, image2);
    return memcmp(image1.data, image2.data, image_size(image1));
}

int morph_images_parallel(const double alpha, const Image* image1, const Image* image2, Image* result) {
    
    int world_size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    unsigned char* image1_data = (image1 != NULL) ? image1->data : NULL;
    unsigned char* image2_data = (image2 != NULL) ? image2->data : NULL;
    unsigned char* result_data = (result != NULL) ? result->data : NULL;

    const double r_alpha = (1.0 - alpha);

    int img_size = 0;
    if (rank == ROOT) {
        img_size = image_size(*image1);
    }
    MPI_Bcast(&img_size, 1, MPI_INT, ROOT, MPI_COMM_WORLD);

    int size = img_size / world_size;    
    int* counts = (int*) malloc(world_size * sizeof(int));
    int* displs = (int*) malloc(world_size * sizeof(int));
    int world = 0;
    for (; world < world_size - 1; ++world) {
        counts[world] = size;
        displs[world] = world * size;
    }
    counts[world] = img_size - (world * size);
    displs[world] = world * size;
    size = counts[rank];

    unsigned char* recv1 = (unsigned char*) malloc(size);
    MPI_Scatterv(image1_data, counts, displs, MPI_UINT8_T, recv1, size, MPI_UINT8_T, ROOT, MPI_COMM_WORLD);

    unsigned char* recv2 = (unsigned char*) malloc(size);
    MPI_Scatterv(image2_data, counts, displs, MPI_UINT8_T, recv2, size, MPI_UINT8_T, ROOT, MPI_COMM_WORLD);

    unsigned char* res = (unsigned char*) malloc(size);
    for (int i = 0; i < size; ++i) {
        res[i] = (recv1[i] * alpha) + (recv2[i] * r_alpha);
    }

    MPI_Gatherv(res, size, MPI_UINT8_T, result_data, counts, displs, MPI_UINT8_T, ROOT, MPI_COMM_WORLD);

    free(recv1);
    free(recv2);
    free(res);
    free(counts);
    free(displs);

    return 0;
}

void free_image(Image* image) {
    free(image->data);
}