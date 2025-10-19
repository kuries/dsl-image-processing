#include "ImageLangVisitorImpl.h"
#include <iostream>
#include <string>

std::unordered_map<std::string, Image> imageTable;
std::unordered_map<std::string, Mask> maskTable;

antlrcpp::Any ImageLangVisitorImpl::visitImageDecl(ImageLangParser::ImageDeclContext *ctx) {
    std::string id = ctx->ID()->getText();
    std::vector<std::vector<int>> pixels = parseArray2D(ctx->array2D());
    imageTable[id] = Image{pixels};
    return nullptr;
}

antlrcpp::Any ImageLangVisitorImpl::visitMaskDecl(ImageLangParser::MaskDeclContext *ctx) {
    std::string id = ctx->ID()->getText();
    std::vector<std::vector<int>> pixels = parseArray2D(ctx->array2D());
    maskTable[id] = Mask{pixels};
    return nullptr;
}

antlrcpp::Any ImageLangVisitorImpl::visitApplyMaskStmt(ImageLangParser::ApplyMaskStmtContext *ctx) {
    std::string img = ctx->ID(0)->getText();
    std::string mask = ctx->ID(1)->getText();
    applyMask(imageTable[img], maskTable[mask]);
    return nullptr;
}

antlrcpp::Any ImageLangVisitorImpl::visitPrintStmt(ImageLangParser::PrintStmtContext *ctx) {
    std::string img = ctx->ID()->getText();
    printImage(imageTable[img]);
    return nullptr;
}

// Helper to convert parse tree array2D to vector<vector<int>>
std::vector<std::vector<int>> ImageLangVisitorImpl::parseArray2D(ImageLangParser::Array2DContext *ctx) {
    std::vector<std::vector<int>> arr;
    for (auto rowCtx : ctx->rowList()->row()) {
        std::vector<int> row;
        for (auto numCtx : rowCtx->numberList()->NUMBER())
            row.push_back(std::stoi(numCtx->getText()));
        arr.push_back(row);
    }
    return arr;
}
