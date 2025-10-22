#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <memory>

using namespace std;

// // Base IR node
// struct IRNode {
//     virtual ~IRNode() = default;
//     virtual void print(int indent = 0) const = 0;
// };

/// Base class for all expression nodes.
class ExprAST {
public:
    virtual ~ExprAST() = default;
    virtual void print(int indent = 0) const = 0;
};

/// Number literal, e.g. `5.0`
class NumberExprAST : public ExprAST {
    double Val;

public:
    NumberExprAST(double Val) : Val(Val) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "NumberExprAST " << Val << "\n";
    }
};

/// Variable reference, e.g. `x`
class VariableExprAST : public ExprAST {
    std::string Name;

public:
    VariableExprAST(std::string Name) : Name(std::move(Name)) {}
    const std::string &getName() const { return Name; }

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "VariableExprAST " << Name << "\n";
    }
};

/// Binary operation, e.g. `a + b`
class BinaryExprAST : public ExprAST {
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "BinaryExprAST " << Op << "\n";
        LHS->print(indent + 2);
        RHS->print(indent + 2);
    }
};

/// Array access, e.g. img[i][j]
class ArrayAccessExprAST : public ExprAST {
    std::string ArrayName;
    std::unique_ptr<ExprAST> Index1, Index2;

public:
    ArrayAccessExprAST(std::string ArrayName, std::unique_ptr<ExprAST> Index1, std::unique_ptr<ExprAST> Index2)
        : ArrayName(std::move(ArrayName)), Index1(std::move(Index1)), Index2(std::move(Index2)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "ArrayAccessExprAST " << ArrayName << "\n";
        Index1->print(indent + 2);
        Index2->print(indent + 2);
    }
};


/// Assignment: e.g. `output[i][j] = expr`
class AssignExprAST : public ExprAST {
    std::unique_ptr<ExprAST> LHS, RHS;

public:
    AssignExprAST(std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : LHS(std::move(LHS)), RHS(std::move(RHS)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "AssignExprAST\n";
        LHS->print(indent + 2);
        RHS->print(indent + 2);
    }
};

/// For loop: for i = start .. end { body }
class ForExprAST : public ExprAST {
    std::string VarName;
    std::unique_ptr<ExprAST> Start, End;
    std::vector<std::unique_ptr<ExprAST>> Body;

public:
    ForExprAST(std::string VarName, std::unique_ptr<ExprAST> Start,
               std::unique_ptr<ExprAST> End, std::vector<std::unique_ptr<ExprAST>> Body)
        : VarName(std::move(VarName)), Start(std::move(Start)), End(std::move(End)), Body(std::move(Body)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "ForExprAST " << VarName << "\n";
        std::cout << std::string(indent + 2, ' ') << "Start:\n";
        Start->print(indent + 4);
        std::cout << std::string(indent + 2, ' ') << "End:\n";
        End->print(indent + 4);
        std::cout << std::string(indent + 2, ' ') << "Body:\n";
        for (auto &stmt : Body)
            stmt->print(indent + 4);
    }
};

/// Represents an 'image' object declaration.
class ImageDeclExprAST : public ExprAST {
    std::string Name;
    std::unique_ptr<ExprAST> InitExpr; // typically a LoadExprAST

public:
    ImageDeclExprAST(std::string Name, std::unique_ptr<ExprAST> InitExpr)
        : Name(std::move(Name)), InitExpr(std::move(InitExpr)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "ImageDeclExprAST " << Name << "\n";
        if (InitExpr)
            InitExpr->print(indent + 2);
    }
};

/// Represents a 'mask' object declaration.
class MaskDeclExprAST : public ExprAST {
    std::string Name;
    std::unique_ptr<ExprAST> InitExpr; // typically a LoadExprAST

public:
    MaskDeclExprAST(std::string Name, std::unique_ptr<ExprAST> InitExpr)
        : Name(std::move(Name)), InitExpr(std::move(InitExpr)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "MaskDeclExprAST " << Name << "\n";
        if (InitExpr)
            InitExpr->print(indent + 2);
    }
};

/// Represents load("path")
class LoadExprAST : public ExprAST {
    std::string Path;

public:
    explicit LoadExprAST(std::string Path) : Path(std::move(Path)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "LoadExprAST \"" << Path << "\"\n";
    }

    const std::string &getPath() const { return Path; }
};

/// Represents store(path)
class StoreExprAST : public ExprAST {
    std::string ImageName;
    std::string Path;

public:
    StoreExprAST(std::string ImageName, std::string Path)
        : ImageName(std::move(ImageName)), Path(std::move(Path)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "StoreExprAST " << ImageName << " -> \"" << Path << "\"\n";
    }
};





// struct IRImageDecl : IRNode {
//     std::string id;
//     std::vector<std::vector<int>> values;

//     void print(int indent = 0) const override {
//         std::cout << std::string(indent, ' ') << "IRImageDecl " << id << "\n";
//         for (const auto& row : values) {
//             std::cout << std::string(indent + 2, ' ');
//             for (auto v : row) std::cout << v << " ";
//             std::cout << "\n";
//         }
//     }
// };

// struct IRMaskDecl : IRNode {
//     std::string id;
//     std::vector<std::vector<int>> values;

//     void print(int indent = 0) const override {
//         std::cout << std::string(indent, ' ') << "IRMaskDecl " << id << "\n";
//         for (const auto& row : values) {
//             std::cout << std::string(indent + 2, ' ');
//             for (auto v : row) std::cout << v << " ";
//             std::cout << "\n";
//         }
//     }
// };

// struct IRApplyMask : IRNode {
//     std::string image, mask;
//     int x, y;

//     void print(int indent = 0) const override {
//         std::cout << std::string(indent, ' ')
//                   << "IRApplyMask mask=" << mask 
//                   << " image=" << image 
//                   << " x=" << x << " y=" << y << "\n";
//     }
// };


// struct IRReturn : IRNode {
//     std::string id;

//     void print(int indent = 0) const override {
//         std::cout << std::string(indent, ' ') << "IRReturn " << id << "\n";
//     }
// };


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
