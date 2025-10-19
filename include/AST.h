#pragma once
#include <string>
#include <vector>
#include <memory>

struct Node { virtual ~Node() = default; };

struct ImageDecl : Node {
    std::string name;
    std::vector<std::vector<int>> pixels;
};

struct MaskDecl : Node {
    std::string name;
    std::vector<std::vector<int>> pixels;
};

struct ApplyMaskStmt : Node {
    std::string imageName;
    std::string maskName;
};

struct PrintStmt : Node {
    std::string imageName;
};
