#include <iostream>
#include <cstdlib>
#include <opencv2/opencv.hpp>

using namespace cv;

// Compare two grayscale images and return mean absolute error
double compareImages(const cv::Mat& img1, const cv::Mat& img2) {
    if (img1.size() != img2.size() || img1.type() != img2.type()) {
        std::cerr << "Image size/type mismatch\n";
        return -1.0;
    }

    cv::Mat diff;
    cv::absdiff(img1, img2, diff);
    return cv::mean(diff)[0]; // mean absolute error
}

int main() {
    const std::string dslCommand = "../build/image-dsl ../test/threshold.imgdsl -opt=true";
    const std::string dslOutputPath = "/root/data/samples/images/img2.jpg";
    const std::string inputPath = "/root/data/samples/images/img-r.jpeg";

    // Step 1: Run the DSL program
    int ret = std::system(dslCommand.c_str());
    if (ret != 0) {
        std::cerr << "DSL execution failed with code " << ret << "\n";
        return 1;
    }

    // Step 2: Load DSL output and original input
    cv::Mat dslOutput = cv::imread(dslOutputPath);
    cv::Mat input = cv::imread(inputPath);

    if (dslOutput.empty() || input.empty()) {
        std::cerr << "Failed to load images\n";
        return 1;
    }

    // Step 3: Apply equivalent OpenCV operation (example: threshold at 128)
    // cv::Mat expected;
    // cv::threshold(input, expected, 128, 255, cv::THRESH_BINARY);

    cv::Mat brightened;
    cv::add(input, cv::Scalar(100, 100, 100), brightened);

    // Step 4: Compare
    double error = compareImages(dslOutput, brightened);
    if (error < 0) {
        std::cerr << "Comparison failed\n";
        return 1;
    }

    std::cout << "Mean absolute pixel error: " << error << "\n";
    if (error > 0.1) {
        std::cout << "❌ Test failed: output differs from expected\n";
    } else {
        std::cout << "✅ Test passed: output matches expected\n";
    }

    return 0;
}
