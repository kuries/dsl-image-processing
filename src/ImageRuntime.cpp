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
    int isGreyscale = 1;
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

extern "C" int get_image_height(Image* img) {
    if (!img || !img->data) {
        std::cerr << "[ImageRuntime] get_image_height: null or empty image\n";
        return -1;
    }
    int height = img->height;
    std::cout << "[ImageRuntime] Succesfully fetched image height " << height << std::endl;
    return height;
}

extern "C" void printInt(int n) {
    std::cout<<"Integer: "<<n<<'\n';
}

extern "C" int get_image_width(Image* img) {
    if (!img || !img->data) {
        std::cerr << "[ImageRuntime] get_image_width: null or empty image\n";
        return -1;
    }
    int width = img->width;
    std::cout << "[ImageRuntime] Succesfully fetched image width " << width << std::endl;
    return width;
}

extern "C" uint8_t* get_image_data(Image* img) {
    if (!img || !img->data) {
        std::cerr << "[ImageRuntime] get_image_data: null or empty image\n";
        return nullptr;
    }
    uint8_t* data = img->data;
    std::cout << "[ImageRuntime] Succesfully fetched image data " << std::endl;
    return data;
}