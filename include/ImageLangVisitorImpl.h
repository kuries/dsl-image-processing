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

// Visitor class
class ImageLangVisitorImpl : public ImageLangBaseVisitor {
public:
    antlrcpp::Any visitImageDecl(ImageLangParser::ImageDeclContext *ctx) override;
    antlrcpp::Any visitMaskDecl(ImageLangParser::MaskDeclContext *ctx) override;
    antlrcpp::Any visitApplyMaskStmt(ImageLangParser::ApplyMaskStmtContext *ctx) override;
    antlrcpp::Any visitPrintStmt(ImageLangParser::PrintStmtContext *ctx) override;

private:
    std::vector<std::vector<int>> parseArray2D(ImageLangParser::Array2DContext *ctx);
};
