#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include "CodegenContext.h"

using namespace std;

/// Base class for all expression nodes.
class ExprAST {
public:
    virtual ~ExprAST() = default;
    virtual void print(int indent = 0) const = 0;
    virtual llvm::Value *codegen() = 0;
};

// /// Number literal, e.g. `5.0`
class NumberExprAST : public ExprAST {
    double Val;

public:
    NumberExprAST(double Val) : Val(Val) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "NumberExprAST " << Val << "\n";
    }
};

// /// Variable reference, e.g. `x`
class VariableExprAST : public ExprAST {
    std::string Name;

public:
    VariableExprAST(std::string Name) : Name(std::move(Name)) {}
    const std::string &getName() const { return Name; }

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "VariableExprAST " << Name << "\n";
    }
};

// /// Binary operation, e.g. `a + b`
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

// /// Array access, e.g. img[i][j]
// class ArrayAccessExprAST : public ExprAST {
//     std::string ArrayName;
//     std::unique_ptr<ExprAST> Index1, Index2;

// public:
//     ArrayAccessExprAST(std::string ArrayName, std::unique_ptr<ExprAST> Index1, std::unique_ptr<ExprAST> Index2)
//         : ArrayName(std::move(ArrayName)), Index1(std::move(Index1)), Index2(std::move(Index2)) {}

//     void print(int indent = 0) const override {
//         std::cout << std::string(indent, ' ') << "ArrayAccessExprAST " << ArrayName << "\n";
//         Index1->print(indent + 2);
//         Index2->print(indent + 2);
//     }
// };


// /// Assignment: e.g. `output[i][j] = expr`
// class AssignExprAST : public ExprAST {
//     std::unique_ptr<ExprAST> LHS, RHS;

// public:
//     AssignExprAST(std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
//         : LHS(std::move(LHS)), RHS(std::move(RHS)) {}

//     void print(int indent = 0) const override {
//         std::cout << std::string(indent, ' ') << "AssignExprAST\n";
//         LHS->print(indent + 2);
//         RHS->print(indent + 2);
//     }
// };

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond;
  std::vector<std::unique_ptr<ExprAST>> Then, Else;

public:
  IfExprAST(std::unique_ptr<ExprAST> Cond, std::vector<std::unique_ptr<ExprAST>> Then,
            std::vector<std::unique_ptr<ExprAST>> Else)
      : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

    llvm::Value *codegen() override;
};

/// For loop: for i = start .. end { body }
class ForExprAST : public ExprAST {
    std::string VarName;
    std::unique_ptr<ExprAST> Start, End, Step;
    std::vector<std::unique_ptr<ExprAST>> Body;

public:
    ForExprAST(std::string VarName, std::unique_ptr<ExprAST> Start,
               std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step, 
               std::vector<std::unique_ptr<ExprAST>> Body)
        : VarName(std::move(VarName)), Start(std::move(Start)), End(std::move(End)), Step(std::move(Step)), Body(std::move(Body)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "ForExprAST " << VarName << "\n";
        std::cout << std::string(indent + 2, ' ') << "Start:\n";
        Start->print(indent + 4);
        std::cout << std::string(indent + 2, ' ') << "End:\n";
        End->print(indent + 4);
        std::cout << std::string(indent + 2, ' ') << "Step:\n";
        Step->print(indent + 4);
        std::cout << std::string(indent + 2, ' ') << "Body:\n";
        for (auto &stmt : Body)
            stmt->print(indent + 4);
    }
    llvm::Value *codegen() override;
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
    llvm::Value *codegen();
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
    llvm::Value *codegen();
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
    llvm::Value *codegen();
};

struct ProgramAST : ExprAST {
    std::vector<std::unique_ptr<ExprAST>> Statements;

    void addStmt(std::unique_ptr<ExprAST> stmt) {
        Statements.push_back(std::move(stmt));
    }

    void print(int indent = 0) const override {
        std::cout << "ProgramAST:\n";
        for (const auto &stmt : Statements)
            stmt->print(indent + 2);
    }
    llvm::Value *codegen();
};

