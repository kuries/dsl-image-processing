#include "ImageRuntime.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include <string>
#include <iostream>
#include <algorithm>

extern "C" Image* load_image(const char* path) {
    std::cout << "[ImageRuntime] Loading image: " << path << std::endl;

    Image* img = new Image();
    int isGreyscale = 0;
    img->data = stbi_load(path, &img->width, &img->height, &img->channels, isGreyscale);
    if(isGreyscale)
        img->channels=1;

    if (!img->data) {
        std::cerr << "[ImageRuntime] Failed to load image: " << path << std::endl;
        delete img;
        return nullptr;
    }
    std::cout << "[ImageRuntime] Loaded grayscale image: "
              << img->width << "x" << img->height << "with channels: " << img->channels << std::endl;
    return img;
}

extern "C" void save_image(Image* img, double* image_data, const char* path) {
    if (!image_data) {
        std::cerr << "[ImageRuntime] save_image: null or empty image\n";
        return;
    }
    uint8_t* buffer = new uint8_t[img->width * img->height * 3];

    // Convert and clamp each pixel
    for (int i = 0; i < img->width * img->height * 3; ++i) {
        buffer[i] = static_cast<uint8_t>(std::clamp(image_data[i], 0.0, 255.0));
    }

    if (!stbi_write_png(path, img->width, img->height, img->channels,
                        buffer, img->width * img->channels)) {
        std::cerr << "[ImageRuntime] Failed to save image: " << path << std::endl;
    } else {
        std::cout << "[ImageRuntime] Saved image to " << path << std::endl;
    }
    if(img)
        free_image(img);
    delete[] buffer;
}

extern "C" void free_image(Image* img) {
    if (img) {
        if (img->data) stbi_image_free(img->data);
        delete img;
    }
}

extern "C" double get_image_height(Image* img) {
    if (!img || !img->data) {
        std::cerr << "[ImageRuntime] get_image_height: null or empty image\n";
        return -1;
    }
    int height = img->height;
    std::cout << "[ImageRuntime] Succesfully fetched image height " << height << std::endl;
    return height;
}

extern "C" double get_image_width(Image* img) {
    if (!img || !img->data) {
        std::cerr << "[ImageRuntime] get_image_width: null or empty image\n";
        return -1;
    }
    int width = img->width;
    std::cout << "[ImageRuntime] Succesfully fetched image width " << width << std::endl;
    return width;
}

extern "C" double* get_image_data(Image* img) {
    if (!img || !img->data) {
        std::cerr << "[ImageRuntime] get_image_data: null or empty image\n";
        return nullptr;
    }

    // Allocate flat double array on heap
    double* image = new double[img->width * img->height * 3];
    for (int i = 0; i < img->width * img->height * 3; ++i) {
        image[i] = static_cast<double>(img->data[i]);
    }

    std::cout << "[ImageRuntime] Succesfully fetched image data " << std::endl;
    return image;
}

extern "C" void printDouble(double n) {
    std::cout<<"Integer: "<<n<<'\n';
}

extern "C" void printInt(int n) {
    std::cout<<"Integer: "<<n<<'\n';
}