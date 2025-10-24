#pragma once

#include "AST.h"
#include "ImageRuntime.h"
#include "ImageLangBaseVisitor.h"
#include "ImageLangParser.h"

#include <unordered_map>
#include <string>

using namespace std;

// Global tables for runtime images and masks
extern std::unordered_map<std::string, Image> imageTable;
extern std::unordered_map<std::string, Mask> maskTable;

// // Visitor class
// class ImageLangVisitorImpl : public ImageLangBaseVisitor {
// public:
//     antlrcpp::Any visitImageDecl(ImageLangParser::ImageDeclContext *ctx) override;
//     antlrcpp::Any visitMaskDecl(ImageLangParser::MaskDeclContext *ctx) override;
//     antlrcpp::Any visitApplyMask(ImageLangParser::ApplyMaskContext *ctx) override;
//     antlrcpp::Any visitPrint(ImageLangParser::ApplyMaskContext *ctx) override;

// private:
//     std::vector<std::vector<int>> parseArray2D(ImageLangParser::Array2DContext *ctx);
// };


// class ASTBuilderVisitor {
// public:
//     // Entry point: build IR from ProgramContext
//     std::unique_ptr<IRProgram> build(ImageLangParser::ProgramContext* ctx);

// private:
//     // Visit methods for each grammar rule
//     std::unique_ptr<IRNode> visitImageDecl(ImageLangParser::ImageDeclContext* ctx);
//     std::unique_ptr<IRNode> visitMaskDecl(ImageLangParser::MaskDeclContext* ctx);
//     std::unique_ptr<IRNode> visitApplyMask(ImageLangParser::ApplyMaskContext* ctx);
//     std::unique_ptr<IRNode> visitReturnStmt(ImageLangParser::ReturnStmtContext* ctx);

//     // Helper
//     std::vector<std::vector<int>> parseArray2D(ImageLangParser::Array2DContext* ctx);
// };


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
    std::unique_ptr<ExprAST> buildStatement(ImageLangParser::StatementContext *ctx) {
        if (ctx->imageDecl()) return buildImageDecl(ctx->imageDecl());
        if (ctx->saveStmt())  return buildSaveStmt(ctx->saveStmt());
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

    static std::string stripQuotes(const std::string &s) {
        if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
            return s.substr(1, s.size() - 2);
        return s;
    }
};