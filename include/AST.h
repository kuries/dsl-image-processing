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

// /// Number literal -> `5.0`
class NumberExprAST : public ExprAST {
    double Val;

public:
    NumberExprAST(double Val) : Val(Val) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "NumberExprAST " << Val << "\n";
    }
    llvm::Value *codegen() override;
};

//Variable Declaration Node
class VarDeclExprAST : public ExprAST {
    std::string Name;
    std::unique_ptr<ExprAST> InitExpr; // e.g. NumberExprAST or BinaryExprAST or VariableExprAST

public:
    VarDeclExprAST(std::string Name, std::unique_ptr<ExprAST> InitExpr)
        : Name(std::move(Name)), InitExpr(std::move(InitExpr)) {}

    void print(int indent = 0) const override {
        if(Name != "")
        {
            std::cout << std::string(indent, ' ') << "VarDeclExprAST " << Name;
            if (InitExpr) {
                std::cout << std::string(indent, ' ')<<"\n";
                InitExpr->print(indent + 2);
            } else {
                std::cout << " (no init)\n";
            }
        }
        else
        {
            std::cout <<"Invalid VarDeclExprAST\n";
        }
        
    }

    llvm::Value *codegen() override;
};

// /// Variable reference, e.g. `x = 10; or x = i1;`
class VariableExprAST : public ExprAST {
    std::string Name;

public:
    VariableExprAST(std::string Name) : Name(std::move(Name)) {}
    const std::string &getName() const { return Name; }

    void print(int indent = 0) const override {
        if(Name != "")
        {
            std::cout << std::string(indent, ' ') << "VariableExprAST " << Name << "\n";
        }
        else
        {
            std::cout <<"Invalid VariableExprAST\n";
        }
    }

    llvm::Value *codegen() override;
};

// /// Binary operation, e.g. `a + b`
class BinaryExprAST : public ExprAST {
    std::string Op;
    std::unique_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : Op(std::string(1, Op)), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

    BinaryExprAST(std::string Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

    void print(int indent = 0) const override {
        if(Op != "" && LHS != nullptr && RHS != nullptr)
        {
            std::cout << std::string(indent, ' ') << "BinaryExprAST " << Op << "\n";
            LHS->print(indent + 2);
            RHS->print(indent + 2);
        }
        else
        {
            std::cout <<"Invalid BinaryExprAST\n";
        }
    }

    llvm::Value *codegen() override;
};

// Array access, e.g. img[i][j][0]
class ArrayAccessExprAST : public ExprAST {
    std::string ArrayName;
    
public:
    std::unique_ptr<ExprAST> Index1, Index2, Index3;
    ArrayAccessExprAST(std::string ArrayName, std::unique_ptr<ExprAST> Index1, std::unique_ptr<ExprAST> Index2, std::unique_ptr<ExprAST> Index3)
        : ArrayName(std::move(ArrayName)), Index1(std::move(Index1)), Index2(std::move(Index2)), Index3(std::move(Index3)){}
    const std::string &getName() const { return ArrayName; }

    void print(int indent = 0) const override {
        if(ArrayName != "" && Index1 != nullptr && Index2 != nullptr && Index3 != nullptr)
        {
            std::cout << std::string(indent, ' ') << "ArrayAccessExprAST " << ArrayName << "[]";
            Index1->print(indent + 2);
            Index2->print(indent + 2);
            Index3->print(indent + 2);
        }
        else
        {
            std::cout <<"Invalid ArrayAccessExprAST\n";
        }

    }

    llvm::Value *codegen() override;
};

//Assignment operation node
class AssignExprAST : public ExprAST {
    std::unique_ptr<ExprAST> LHS;
    std::unique_ptr<ExprAST> RHS; // e.g. BinaryExprAST, NumberExprAST, VariableExprAST

public:
    AssignExprAST(std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : LHS(std::move(LHS)), RHS(std::move(RHS)) {}

    void print(int indent = 0) const override {
        if(LHS != nullptr && RHS != nullptr)
        {
            std::cout << std::string(indent, ' ') << "\n ";
            LHS->print(indent + 2) ;
            std::cout << "=";
            RHS->print(indent + 2);
        }
        else
        {
            std::cout <<"Invalid AssignExprAST\n";
        }
    }

    llvm::Value *codegen() override;
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond;
  std::vector<std::unique_ptr<ExprAST>> Then, Else;

public:
  IfExprAST(std::unique_ptr<ExprAST> Cond, std::vector<std::unique_ptr<ExprAST>> Then,
            std::vector<std::unique_ptr<ExprAST>> Else)
      : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

    llvm::Value *codegen() override;

    void print(int indent = 0) const override 
    {
        if(Cond != nullptr)
        {
            auto pad = [&](int n) { return std::string(n, ' '); };

            std::cout << pad(indent) << "IfExprAST\n";

            std::cout << pad(indent + 2) << "Condition:\n";
            if (Cond)
                Cond->print(indent + 4);
            else
                std::cout << pad(indent + 4) << "(none)\n";

            std::cout << pad(indent + 2) << "Then:\n";
            if (Then.empty()) {
                std::cout << pad(indent + 4) << "(empty)\n";
            } else {
                for (const auto &stmt : Then) {
                    if (stmt) stmt->print(indent + 4);
                    std::cout << "\n";
                }
            }

            std::cout << pad(indent + 2) << "Else:\n";
            if (Else.empty()) {
                std::cout << pad(indent + 4) << "(empty)\n";
            } else {
                for (const auto &stmt : Else) {
                    if (stmt) stmt->print(indent + 4);
                    std::cout << "\n";
                }
            }
        }
        else
        {
            std::cout <<"Invalid IfExprAST\n";
        }   
    }

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
        if(VarName != "" && Start != nullptr && End != nullptr && Step != nullptr)
        {
            std::cout << std::string(indent, ' ') << "ForExprAST " << VarName << "\n";
            std::cout << std::string(indent + 2, ' ') << "Start:\n";
            Start->print(indent + 4);
            std::cout << std::string(indent + 2, ' ') << "End:\n";
            End->print(indent + 4);
            std::cout << std::string(indent + 2, ' ') << "Step:\n";
            Step->print(indent + 4);
            std::cout << std::string(indent + 2, ' ') << "Body:\n";
            if (Body.empty()) 
            {
                std::cout << std::string(indent + 4, ' ') << "(empty)\n";
            } 
            else 
            {
                for (const auto &stmt : Body) 
                {
                    if (stmt) stmt->print(indent + 4);
                    std::cout << "\n";
                }
            }
        }
        else
        {
            std::cout <<"Invalid ForExprAST\n";
        }
        
    }
    llvm::Value *codegen() override;
};

/// Image object declaration.
class ImageDeclExprAST : public ExprAST {
    std::string Name;
    std::unique_ptr<ExprAST> InitExpr; // LoadExprAST

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


///  Load("path")
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

/// Store("path")
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

//Node for Entire Program
struct ProgramAST : ExprAST {
    std::vector<std::unique_ptr<ExprAST>> Statements;

    void addStmt(std::unique_ptr<ExprAST> stmt) {
        if (!stmt) 
        {
            std::cerr << "[DEBUG] skipping null stmt in addStmt()\n";
            return;
        }
        Statements.push_back(std::move(stmt));
    }

    void print(int indent = 0) const override {
        std::cout << "ProgramAST:\n";
        for (const auto &stmt : Statements)
            stmt->print(indent + 2);
    }
    llvm::Value *codegen();
};

