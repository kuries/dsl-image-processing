#include "ImageRuntime.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include <string>
#include <iostream>

extern "C" Image* load_image(const char* path) {
    std::cout << "[ImageRuntime] Loading image: " << path << std::endl;

    Image* img = new Image();
    img->data = stbi_load(path, &img->width, &img->height, &img->channels, 1);

    if (!img->data) {
        std::cerr << "[ImageRuntime] Failed to load image: " << path << std::endl;
        delete img;
        return nullptr;
    }

    img->channels = 1;
    std::cout << "[ImageRuntime] Loaded grayscale image: "
              << img->width << "x" << img->height << std::endl;
    return img;
}

extern "C" void save_image(Image* img, const char* path) {
    if (!img || !img->data) {
        std::cerr << "[ImageRuntime] save_image: null or empty image\n";
        return;
    }

    if (!stbi_write_png(path, img->width, img->height, img->channels,
                        img->data, img->width * img->channels)) {
        std::cerr << "[ImageRuntime] Failed to save image: " << path << std::endl;
    } else {
        std::cout << "[ImageRuntime] Saved image to " << path << std::endl;
    }
}

extern "C" void free_image(Image* img) {
    if (img) {
        if (img->data) stbi_image_free(img->data);
        delete img;
    }
}