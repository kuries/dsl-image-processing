
#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <cstdint>

extern "C" {
    struct Image 
    {
        uint8_t* data;
        int width;
        int height;
        int channels;
    };

    extern "C" {
        Image* load_image(const char* path);
        void save_image(Image* img, double*, const char* path);
        void free_image(Image* img);
        double get_image_height(Image* img);
        double get_image_width(Image* img);
    }
}