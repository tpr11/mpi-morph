#include <assert.h>

#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

const double IMAGES_DEFAULT_ALPHA = 0.5;

Image* new_image(int width, int height, int channels) {
    Image* image = (Image*) malloc(sizeof(Image));
    image->width = width;
    image->height = height;
    image->channels = channels;
    image->data = (unsigned char*) malloc(image_size(*image));
    return image;
}

int load_image(const char* filepath, Image* image) {
    int n = 3;
    unsigned char *i = stbi_load(filepath, &image->width, &image->height, &n, 0);
    if (!i) {
        return 0;
    }
    image->data = i;
    image->channels = n;
    return 1;
}

int resize_image(Image* image, int new_width, int new_height) {
    unsigned char* new_data = (unsigned char*) malloc(new_width * new_height * image->channels);
    int return_code = stbir_resize_uint8(image->data, image->width, image->height, 0, 
                                         new_data, new_width, new_height, 0, image->channels);
    unsigned char* aux = image->data;
    image->data = new_data;
    image->width = new_width;
    image->height = new_height;
    free(aux);
    return return_code;
}

int load_resized(const char* filename1, const char* filename2, Image* image1, Image* image2) {
    if (!load_image(filename1, image1)) {
        return 0;
    }
    if (!load_image(filename2, image2)) {
        return 0;
    }
    int lowest_width = image1->width < image2->width ? image1->width : image2->width;
    int lowest_height = image1->height < image2->height ? image1->height : image2->height;
    resize_image(image1, lowest_width, lowest_height);
    resize_image(image2, lowest_width, lowest_height);
    return 1;
}

int image_size(const Image image) {
    return image.width * image.height * image.channels;
}

int save_image(const Image image, const char* filepath) {
    return stbi_write_bmp(filepath, image.width, image.height, image.channels, image.data);
}

void assert_equal_size(const Image* image1, const Image* image2) {
    assert(image1->width == image2->width);
    assert(image1->height == image2->height);
    assert(image1->channels == image2->channels);  
}

int morph_images(const Image image1, const Image image2, const double alpha, Image* result) {
    assert_equal_size(&image1, &image2);
    const double r_alpha = (1.0 - alpha);
    for (int i = 0; i < image_size(*result); ++i) {
        result->data[i] = (image1.data[i] * alpha) + (image2.data[i] * r_alpha);
    }
    return 1;
}

int morph_images_parallel(const Image image1, const Image image2, const double alpha, Image* result) {
    
    assert_equal_size(&image1, &image2);
    const double r_alpha = (1.0 - alpha);

    int world_size, rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    const int img_size = image_size(image1);
    int size = image_size(image1) / world_size;    

    int* counts = (int*) malloc(world_size * sizeof(int));
    int* displs = (int*) malloc(world_size * sizeof(int));
    int world = 0;
    for (; world < world_size - 1; ++world) {
        counts[world] = size;
        displs[world] = world * size;
    }
    counts[world] = img_size - (world * size);
    displs[world] = world * size;

    if (rank == world_size - 1) {
        size = counts[world];
    }

    unsigned char* recv1 = (unsigned char*) malloc(size);
    MPI_Scatterv(image1.data, counts, displs, MPI_UINT8_T, recv1, size, MPI_UINT8_T, 0, MPI_COMM_WORLD);

    unsigned char* recv2 = (unsigned char*) malloc(size);
    MPI_Scatterv(image2.data, counts, displs, MPI_UINT8_T, recv2, size, MPI_UINT8_T, 0, MPI_COMM_WORLD);

    unsigned char* res = (unsigned char*) malloc(size);
    for (int i = 0; i < size; ++i) {
        res[i] = (recv1[i] * alpha) + (recv2[i] * r_alpha);
    }

    MPI_Gatherv(res, size, MPI_UINT8_T, result->data, counts, displs, MPI_UINT8_T, 0, MPI_COMM_WORLD);

    MPI_Finalize();

    free(recv1);
    free(recv2);
    free(res);
    free(counts);
    free(displs);

    return 1;
}

void free_image(Image* image) {
    free(image->data);
}