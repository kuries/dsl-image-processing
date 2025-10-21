#pragma once

#include "AST.h"
#include "ImageRuntime.h"
#include "ImageLangBaseVisitor.h"
#include "ImageLangParser.h"

#include <unordered_map>
#include <string>

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


class ASTBuilderVisitor {
public:
    // Entry point: build IR from ProgramContext
    std::unique_ptr<IRProgram> build(ImageLangParser::ProgramContext* ctx);

private:
    // Visit methods for each grammar rule
    std::unique_ptr<IRNode> visitImageDecl(ImageLangParser::ImageDeclContext* ctx);
    std::unique_ptr<IRNode> visitMaskDecl(ImageLangParser::MaskDeclContext* ctx);
    std::unique_ptr<IRNode> visitApplyMask(ImageLangParser::ApplyMaskContext* ctx);
    std::unique_ptr<IRNode> visitReturnStmt(ImageLangParser::ReturnStmtContext* ctx);

    // Helper
    std::vector<std::vector<int>> parseArray2D(ImageLangParser::Array2DContext* ctx);
};