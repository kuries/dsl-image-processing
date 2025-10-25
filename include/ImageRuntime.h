
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
    void save_image(Image* img, const char* path);
    void free_image(Image* img);
}
}

// struct Image {
//     std::vector<std::vector<int>> pixels;
// };

// struct Mask {
//     std::vector<std::vector<int>> pixels;
// };

// inline void applyMask(Image& img, const Mask& mask) {
//     for (size_t i = 0; i < img.pixels.size(); ++i)
//         for (size_t j = 0; j < img.pixels[i].size(); ++j)
//             if (mask.pixels[i][j] != 0)
//                 img.pixels[i][j] = 0;
// }

// inline void printImage(const Image& img) {
//     for (auto& row : img.pixels) {
//         for (auto val : row) std::cout << val << " ";
//         std::cout << "\n";
//     }
// }