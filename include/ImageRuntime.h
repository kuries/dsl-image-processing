#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>

struct Image {
    std::vector<std::vector<int>> pixels;
};

struct Mask {
    std::vector<std::vector<int>> pixels;
};

inline void applyMask(Image& img, const Mask& mask) {
    for (size_t i = 0; i < img.pixels.size(); ++i)
        for (size_t j = 0; j < img.pixels[i].size(); ++j)
            if (mask.pixels[i][j] != 0)
                img.pixels[i][j] = 0;
}

inline void printImage(const Image& img) {
    for (auto& row : img.pixels) {
        for (auto val : row) std::cout << val << " ";
        std::cout << "\n";
    }
}
