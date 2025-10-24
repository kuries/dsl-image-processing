#include "ImageRuntime.h"
#include <opencv4/opencv2/opencv.hpp>
#include <string>

// Simple wrapper that allocates a new cv::Mat and returns pointer.
// Caller (runtime) will manage lifetime; for demo we allocate and return pointer.
extern "C" void* load_image(const char* path) {
    try {
        cv::Mat* mat = new cv::Mat(cv::imread(path, cv::IMREAD_UNCHANGED));
        std::cout << "Loaded image: " << path << " (" 
              << mat->cols << "x" << mat->rows << ")\n";
        return static_cast<void*>(mat);
    } catch (...) {
        std::cerr << "Failed to load image: " << path << std::endl;
        return nullptr;
    }
}



extern "C" void save_image(void* image, const char* path) {
    if (!image) 
    {
        std::cout << "Unable to save image to : " << path <<"\n Image obj is null";
        return;
    }
    cv::Mat* mat = static_cast<cv::Mat*>(image);
    cv::imwrite(path, *mat);

    std::cout << "Saved image to : " << path;
}
