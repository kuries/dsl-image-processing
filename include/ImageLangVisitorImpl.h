#pragma once

#include "AST.h"
#include "ImageRuntime.h"
#include "ImageLangBaseVisitor.h"
#include "ImageLangParser.h"

#include <unordered_map>
#include <string>

using namespace std;


class ASTBuilder {
public:
    std::unique_ptr<ProgramAST> build(ImageLangParser::ProgramContext *ctx) {
        auto program = std::make_unique<ProgramAST>();

        for (auto stmtCtx : ctx->statement()) {
            auto stmt = buildStatement(stmtCtx);
            if (stmt)
                program->addStmt(std::move(stmt));
        }
        return program;
    }

private:
    std::unique_ptr<ExprAST> buildStatement(ImageLangParser::StatementContext *ctx) 
    {
        if (ctx->imageDecl())     return buildImageDecl(ctx->imageDecl());
        if (ctx->maskDecl())      return buildMaskDecl(ctx->maskDecl());
        if (ctx->saveStmt())      return buildSaveStmt(ctx->saveStmt());
        if (ctx->pixelAssign())   return buildPixelAssign(ctx->pixelAssign());
        if (ctx->applyMask())     return buildApplyMask(ctx->applyMask());
        if (ctx->intDecl())       return buildIntDecl(ctx->intDecl());
        if (ctx->intAssign())     return buildIntAssign(ctx->intAssign());
        if (ctx->applyThreshold())     return buildApplyThreshold(ctx->applyThreshold());
        return nullptr;
    }


    std::unique_ptr<ExprAST> buildApplyThreshold(ImageLangParser::ApplyThresholdContext *ctx)
    {
        std::vector<std::unique_ptr<ExprAST>> body;

        std::string imageName = ctx->ID()->getText();

        auto thresholdExpr = buildExpr(ctx->expr(0));
        auto maxValueExpr = buildExpr(ctx->expr(1));

        auto thresholdStr = "threshold";
        auto maxValueStr = "maxValue";

        

        auto thresholdAssign = std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(thresholdStr),
            std::move(thresholdExpr)
        );

        auto maxValueAssign = std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(maxValueStr),
            std::move(maxValueExpr)
        );

        body.push_back(std::move(thresholdAssign));
        body.push_back(std::move(maxValueAssign));

        auto iVar = "i";
        auto jVar = "j";

        auto condition = std::make_unique<BinaryExprAST>(
            '>', 
            std::make_unique<ArrayAccessExprAST>(
                imageName,
                std::make_unique<VariableExprAST>(iVar),
                std::make_unique<VariableExprAST>(jVar)
            ),
            std::make_unique<VariableExprAST>(thresholdStr)
        );

        auto lhsAssign = std::make_unique<AssignExprAST>(
            std::make_unique<ArrayAccessExprAST>(
                imageName,
                std::make_unique<VariableExprAST>(iVar),
                std::make_unique<VariableExprAST>(jVar)
            ),
            std::make_unique<VariableExprAST>(maxValueStr)
        );

        auto rhsAssign = std::make_unique<AssignExprAST>(
            std::make_unique<ArrayAccessExprAST>(
                imageName,
                std::make_unique<VariableExprAST>(iVar),
                std::make_unique<VariableExprAST>(jVar)
            ),
            std::make_unique<NumberExprAST>(0)
        );

        std::vector<std::unique_ptr<ExprAST>> thenBody;
        thenBody.push_back(std::move(lhsAssign));

        std::vector<std::unique_ptr<ExprAST>> elseBody;
        elseBody.push_back(std::move(rhsAssign));

        auto ifElseConfition = std::make_unique<IfExprAST>(
            std::move(condition),
            std::move(thenBody),
            std::move(elseBody)
        );
        

        std::vector<std::unique_ptr<ExprAST>> innerBody;
        innerBody.push_back(std::move(ifElseConfition));

        // Inner loop (j)
        auto innerLoop = std::make_unique<ForExprAST>(
            jVar,
            std::make_unique<NumberExprAST>(0),
            std::make_unique<VariableExprAST>(imageName + ".mask_width"),
            std::make_unique<NumberExprAST>(1),
            std::move(innerBody)
        );

        std::vector<std::unique_ptr<ExprAST>> outerBody;
        outerBody.push_back(std::move(innerLoop));

        // Outer loop (i)
        auto outerLoop = std::make_unique<ForExprAST>(
            iVar,
            std::make_unique<NumberExprAST>(0),
            std::make_unique<VariableExprAST>(imageName + ".height"),
            std::make_unique<NumberExprAST>(1),
            std::move(outerBody)
        );

        body.push_back(std::move(outerLoop));

        auto program = std::make_unique<ProgramAST>();
        for (auto &stmt : body)
            program->addStmt(std::move(stmt));

        return program;
    }

    // Entry point for building expressions
    std::unique_ptr<ExprAST> buildExpr(ImageLangParser::ExprContext *ctx) 
    {
        return buildAddExpr(ctx->addExpr());
    }

    // Handles addition and subtraction
    std::unique_ptr<ExprAST> buildAddExpr(ImageLangParser::AddExprContext *ctx) 
    {
        auto lhs = buildMulExpr(ctx->mulExpr(0));

        // ctx->children contains interleaved operators and operands
        for (size_t i = 1; i < ctx->mulExpr().size(); ++i) {
            std::string opText = ctx->children[2*i - 1]->getText(); // operator token
            char op = opText[0]; // '+' or '-'
            auto rhs = buildMulExpr(ctx->mulExpr(i));
            lhs = std::make_unique<BinaryExprAST>(op, std::move(lhs), std::move(rhs));
        }

        return lhs;
    }

    // Handles multiplication and division
    std::unique_ptr<ExprAST> buildMulExpr(ImageLangParser::MulExprContext *ctx) {
        auto lhs = buildPrimary(ctx->primary(0));

        for (size_t i = 1; i < ctx->primary().size(); ++i) {
            std::string opText = ctx->children[2*i - 1]->getText(); // operator token
            char op = opText[0]; // '*' or '/'
            auto rhs = buildPrimary(ctx->primary(i));
            lhs = std::make_unique<BinaryExprAST>(op, std::move(lhs), std::move(rhs));
        }

        return lhs;
    }

    std::unique_ptr<ExprAST> buildIntDecl(ImageLangParser::IntDeclContext *ctx) 
    {
        std::string name = ctx->ID()->getText();
        std::unique_ptr<ExprAST> initExpr = nullptr;

        if (ctx->expr()) {
            initExpr = buildExpr(ctx->expr());
        }

        return std::make_unique<VarDeclExprAST>(name, std::move(initExpr));
    }

    std::unique_ptr<ExprAST> buildIntAssign(ImageLangParser::IntAssignContext *ctx) 
    {
        // Build LHS as a VariableExprAST
        auto lhs = std::make_unique<VariableExprAST>(ctx->ID()->getText());

        // Build RHS expression
        auto rhs = buildExpr(ctx->expr());

        // Create assignment AST
        return std::make_unique<AssignExprAST>(std::move(lhs), std::move(rhs));
    }


    std::unique_ptr<ExprAST> buildPrimary(ImageLangParser::PrimaryContext *ctx)
    {
        // Case 1: Integer literal
        if (ctx->INT())
            return std::make_unique<NumberExprAST>(std::stoi(ctx->INT()->getText()));

        // Case 2: Pixel access like img[x][y]
        if (ctx->ID() && ctx->expr().size() == 2)
        {
            std::string imageName = ctx->ID()->getText();
            auto xExpr = buildExpr(ctx->expr(0));  // first index
            auto yExpr = buildExpr(ctx->expr(1));  // second index

            return std::make_unique<ArrayAccessExprAST>(imageName, std::move(xExpr), std::move(yExpr));
        }

        // Case 3: Simple variable
        if (ctx->ID())
            return std::make_unique<VariableExprAST>(ctx->ID()->getText());

        // Case 4: Parenthesized expression
        if (ctx->expr(0))
            return buildExpr(ctx->expr(0));

        return nullptr;
    }


    std::unique_ptr<ExprAST> buildImageDecl(ImageLangParser::ImageDeclContext *ctx) {
        std::string name = ctx->ID()->getText();
        auto loadExpr = buildLoadExpr(ctx->loadExpr());
        return std::make_unique<ImageDeclExprAST>(name, std::move(loadExpr));
    }

    std::unique_ptr<ExprAST> buildLoadExpr(ImageLangParser::LoadExprContext *ctx) {
        std::string path = stripQuotes(ctx->STRING()->getText());
        return std::make_unique<LoadExprAST>(path);
    }

    std::unique_ptr<ExprAST> buildSaveStmt(ImageLangParser::SaveStmtContext *ctx) {
        std::string imageName = ctx->ID()->getText();
        std::string path = stripQuotes(ctx->STRING()->getText());
        return std::make_unique<StoreExprAST>(imageName, path);
    }

    std::unique_ptr<ExprAST> buildPixelAssign(ImageLangParser::PixelAssignContext *ctx) 
    {
        std::string imageName = ctx->ID()->getText();

        // Use buildExpr for indices
        auto index1 = buildExpr(ctx->expr(0));
        auto index2 = buildExpr(ctx->expr(1));

        auto valueExpr = buildExpr(ctx->expr(2));

        // LHS: ArrayAccessExprAST
        auto lhs = std::make_unique<ArrayAccessExprAST>(imageName, std::move(index1), std::move(index2));

        // Assignment node
        return std::make_unique<AssignExprAST>(std::move(lhs), std::move(valueExpr));
    }

    std::unique_ptr<ExprAST> buildMaskDecl(ImageLangParser::MaskDeclContext *ctx) {
        std::string name = ctx->ID()->getText();
    std::vector<std::vector<int>> data;

    for (auto rowCtx : ctx->array2D()->row()) {
        std::vector<int> row;
        for (auto intCtx : rowCtx->INT()) {
            row.push_back(std::stoi(intCtx->getText()));
        }
        data.push_back(std::move(row));
    }

    return std::make_unique<MaskDeclExprAST>(name, std::move(data), data.size(), data.size() == 0 ? 0 : data[0].size());
    }


    // -----------------------------
    // Apply mask
    std::unique_ptr<ExprAST> buildApplyMask(ImageLangParser::ApplyMaskContext *ctx) {
        std::string imageName = ctx->ID(0)->getText();
        std::string maskName = ctx->ID(1)->getText();
        int offsetX = std::stoi(ctx->INT(0)->getText());
        int offsetY = std::stoi(ctx->INT(1)->getText());
        return std::make_unique<ApplyMaskExprAST>(imageName, maskName, offsetX, offsetY);
    }


    static std::string stripQuotes(const std::string &s) {
        if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
            return s.substr(1, s.size() - 2);
        return s;
    }
    
};