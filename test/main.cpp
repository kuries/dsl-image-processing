#include <iostream>
#include <fstream> 
#include <filesystem>
#include <cstdlib>
#include <opencv2/opencv.hpp>

using namespace cv;

// Compare two images and return mean absolute error
double compareImages(const cv::Mat& img1, const cv::Mat& img2) {
    if (img1.size() != img2.size() || img1.type() != img2.type()) {
        std::cerr << "Image size/type mismatch\n";
        return -1.0;
    }

    cv::Mat diff;
    cv::absdiff(img1, img2, diff);
    return cv::mean(diff)[0];
}

cv::Mat applyReferenceOperation(const cv::Mat& input, const std::string& method) {
    cv::Mat result;

    if (method == "brighten") {
        cv::add(input, cv::Scalar(100, 100, 100), result);
    } else if (method == "contrast") {
        double alpha = 100;
        input.convertTo(result, -1, alpha, 128 * (1 - alpha));
    } else if (method == "threshold") {
        cv::Mat gray, binary;
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
        cv::threshold(gray, binary, 128, 255, cv::THRESH_BINARY);
        cv::cvtColor(binary, result, cv::COLOR_GRAY2BGR);
    }
    else if (method == "greyscale") {
        cv::Mat output(input.size(), CV_8UC3);

        for (int y = 0; y < input.rows; ++y) {
            for (int x = 0; x < input.cols; ++x) {
                cv::Vec3b pixel = input.at<cv::Vec3b>(y, x);
                // Luminosity formula: 0.299*R + 0.587*G + 0.114*B
                unsigned char gray = static_cast<unsigned char>(
                    0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0]);

                output.at<cv::Vec3b>(y, x) = cv::Vec3b(gray, gray, gray);
            }
        }

        result = output;
    }
     else {
        std::cerr << "Unsupported method: " << method << "\n";
        std::exit(1);
    }

    return result;
}

void updateDSL(std::string newInputPath, std::string newOutputPath, std::string dslFilePath, std::string method){
    std::string methodCall;
    if(method == "brighten")
        methodCall = "adjust_brightness(input, 100);\n";
    else if(method == "threshold")
        methodCall = "apply_threshold(input, 128, 255);\n";
    else if(method == "greyscale")
        methodCall = "convertToGreyscale(input);\n";
    else if(method == "contrast")
        methodCall = "adjust_contrast(input , 100);\n";

    std::string updatedDSL = 
        "image input = load(\"" + newInputPath + "\");\n" + methodCall + "input.save(\"" + newOutputPath + "\");\n";

    // Write to file
    std::ofstream outFile(dslFilePath);
    if (!outFile) {
        std::cerr << "Failed to open DSL file for writing: " << dslFilePath << "\n";
        return;
    }

    outFile << updatedDSL;
    outFile.close();

    std::cout << "DSL file updated successfully.\n";
}

std::vector<std::string> getImagePaths(const std::string& rootDir) {
    std::vector<std::string> imagePaths;
    int i=0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(rootDir)) {
        if(i>10)
            break;
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            if (ext == ".jpg" || ext == ".JPEG" || ext == ".png" || ext == ".bmp" || ext == ".tiff") {
                imagePaths.push_back(entry.path().string());
            }
        }
        i++;
    }

    return imagePaths;
}

double testMethod(std::string inputPath, std::string dslOutputPath, std::string method){
    // Run DSL program (assumes DSL uses inputPath and writes to dslOutputPath)
    std::string dslFilePath = "../test/" + method + ".imgdsl";
    updateDSL(inputPath, dslOutputPath, dslFilePath, method);

    std::string dslCommand = "../build/image-dsl " + dslFilePath + " -opt=true";
    int ret = std::system(dslCommand.c_str());
    if (ret != 0) {
        std::cerr << "DSL execution failed with code " << ret << "\n";
        return 1;
    }

    // Load images
    cv::Mat input = cv::imread(inputPath);
    cv::Mat dslOutput = cv::imread(dslOutputPath);

    if (input.empty() || dslOutput.empty()) {
        std::cerr << "Failed to load input or output image\n";
        return 1;
    }

    // Apply reference OpenCV operation
    cv::Mat expected = applyReferenceOperation(input, method);
    bool success = cv::imwrite("output.jpg", expected); // Save the image
    // Compare
    double error = compareImages(dslOutput, expected);
    return error;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input_image> <dsl_output_image> <method>\n";
        std::cerr << "Methods: brightness, contrast, threshold\n";
        return 1;
    }

    std::string inputPath = argv[1];
    std::string dslOutputPath = argv[2];
    std::string method = argv[3];

    std::string directory = "/root/data/samples/images/imagenet-mini/val";
    std::vector<std::string> images = getImagePaths(directory);
    // std::cout<<"LOGGING"<<images.size()<<"\n";
    // for(auto i: images)
    //     std::cout<<i<<"\n";


    double totalError = 0.0;
    int count = 0;

    for (const auto& inputPath : images) {
        double error = testMethod(inputPath, dslOutputPath, method);
        totalError += error;
        ++count;
    }

    double meanError = (count > 0) ? totalError / count : 0.0;
    std::cout << "Mean error over " << count << " images: " << meanError << "\n";

    if (meanError < 0) {
        std::cerr << "Comparison failed\n";
        return 1;
    }

    std::cout << "Mean absolute pixel error: " << meanError << "\n";
    if (meanError > 0.1) {
        std::cout << "❌ Test failed: output differs from expected ny error: "<< meanError<<"\n";
    } else {
        std::cout << "✅ Test passed: output matches expected\n";
    }

    return 0;
}
