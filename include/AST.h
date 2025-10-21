#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <memory>

using namespace std;

// Base IR node
struct IRNode {
    virtual ~IRNode() = default;
    virtual void print(int indent = 0) const = 0;
};

struct IRImageDecl : IRNode {
    std::string id;
    std::vector<std::vector<int>> values;

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "IRImageDecl " << id << "\n";
        for (const auto& row : values) {
            std::cout << std::string(indent + 2, ' ');
            for (auto v : row) std::cout << v << " ";
            std::cout << "\n";
        }
    }
};

struct IRMaskDecl : IRNode {
    std::string id;
    std::vector<std::vector<int>> values;

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "IRMaskDecl " << id << "\n";
        for (const auto& row : values) {
            std::cout << std::string(indent + 2, ' ');
            for (auto v : row) std::cout << v << " ";
            std::cout << "\n";
        }
    }
};

struct IRApplyMask : IRNode {
    std::string image, mask;
    int x, y;

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "IRApplyMask mask=" << mask 
                  << " image=" << image 
                  << " x=" << x << " y=" << y << "\n";
    }
};

struct IRReturn : IRNode {
    std::string id;

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "IRReturn " << id << "\n";
    }
};


// IR Program (sequence of statements)
using IRProgram = std::vector<std::unique_ptr<IRNode>>;

class IRPrinter {
public:
    static void print(const IRProgram& program, int indent = 0) {
        std::cout << std::string(indent, ' ') << "=== IR Program (" 
                  << program.size() << " statements) ===\n";

        int index = 0;
        for (const auto& node : program) {
            std::cout << std::string(indent + 2, ' ') << "[" << index++ << "] ";
            node->print(indent + 2);
        }

        std::cout << std::string(indent, ' ') << "===============================\n";
    }
};
