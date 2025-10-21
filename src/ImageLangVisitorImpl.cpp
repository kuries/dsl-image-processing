#include "ImageLangVisitorImpl.h"
#include "ImageLangParser.h"
#include <iostream>
#include <string>
#include <stdexcept>

using namespace std;

// std::unordered_map<std::string, Image> imageTable;
// std::unordered_map<std::string, Mask> maskTable;

// antlrcpp::Any ImageLangVisitorImpl::visitImageDecl(ImageLangParser::ImageDeclContext *ctx) {
//     std::string id = ctx->ID()->getText();
//     std::vector<std::vector<int>> pixels = parseArray2D(ctx->array2D());
//     imageTable[id] = Image{pixels};
//     return nullptr;
// }

// antlrcpp::Any ImageLangVisitorImpl::visitMaskDecl(ImageLangParser::MaskDeclContext *ctx) {
//     std::string id = ctx->ID()->getText();
//     std::vector<std::vector<int>> pixels = parseArray2D(ctx->array2D());
//     maskTable[id] = Mask{pixels};
//     return nullptr;
// }

// antlrcpp::Any ImageLangVisitorImpl::visitApplyMask(ImageLangParser::ApplyMaskContext *ctx) {
//     std::string img = ctx->ID(0)->getText();
//     std::string mask = ctx->ID(1)->getText();
//     applyMask(imageTable[img], maskTable[mask]);
//     return nullptr;
// }

// antlrcpp::Any ImageLangVisitorImpl::visitPrint(ImageLangParser::ApplyMaskContext *ctx) {
//     std::string img = ctx->ID()->getText();
//     printImage(imageTable[img]);
//     return nullptr;
// }

// // Helper to convert parse tree array2D to vector<vector<int>>
// std::vector<std::vector<int>> ImageLangVisitorImpl::parseArray2D(ImageLangParser::Array2DContext *ctx) {
//     std::vector<std::vector<int>> arr;
//     for (auto rowCtx : ctx->rowList()->row()) {
//         std::vector<int> row;
//         for (auto numCtx : rowCtx->numberList()->NUMBER())
//             row.push_back(std::stoi(numCtx->getText()));
//         arr.push_back(row);
//     }
//     return arr;
// }


//-------------------------------------------------------------------------------------------------
//ASTBuilder

std::unique_ptr<IRProgram> ASTBuilderVisitor::build(ImageLangParser::ProgramContext* ctx) {
    auto program = make_unique<IRProgram>();

    try {
        std::cout << "[DEBUG] Starting IR build..." << std::endl;
        std::cout << "[DEBUG] Number of statements: " << ctx->statement().size() << std::endl;

        for (size_t i = 0; i < ctx->statement().size(); ++i) {
            auto stmtCtx = ctx->statement(i);
            std::cout << "[DEBUG] Visiting statement " << i << std::endl;

            if (stmtCtx->imageDecl()) {
                std::cout << "[DEBUG] -> imageDecl" << std::endl;
                auto node = visitImageDecl(stmtCtx->imageDecl());
                std::cout << "[DEBUG]   Done imageDecl" << std::endl;
                program->push_back(std::move(node));
            } 
            else if (stmtCtx->maskDecl()) {
                std::cout << "[DEBUG] -> maskDecl" << std::endl;
                auto node = visitMaskDecl(stmtCtx->maskDecl());
                std::cout << "[DEBUG]   Done maskDecl" << std::endl;
                program->push_back(std::move(node));
            } 
            else if (stmtCtx->applyMask()) {
                std::cout << "[DEBUG] -> applyMask" << std::endl;
                auto node = visitApplyMask(stmtCtx->applyMask());
                std::cout << "[DEBUG]   Done applyMask" << std::endl;
                program->push_back(std::move(node));
            } 
            else if (stmtCtx->returnStmt()) {
                std::cout << "[DEBUG] -> returnStmt" << std::endl;
                auto node = visitReturnStmt(stmtCtx->returnStmt());
                std::cout << "[DEBUG]   Done returnStmt" << std::endl;
                program->push_back(std::move(node));
            } 
            else {
                std::cout << "[DEBUG] -> Empty or unknown statement" << std::endl;
            }
        }

        std::cout << "[DEBUG] Finished IR build successfully." << std::endl;
    } 
    catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in IRBuilderVisitorStandalone::build: "
                  << e.what() << std::endl;
        throw; // rethrow after logging
    }
    catch (...) {
        std::cerr << "[ERROR] Unknown exception in IRBuilderVisitorStandalone::build" << std::endl;
        throw;
    }

    return program;
}

// -----------------------------
// imageDecl
// -----------------------------
std::unique_ptr<IRNode> ASTBuilderVisitor::visitImageDecl(ImageLangParser::ImageDeclContext* ctx) {
    auto node = make_unique<IRImageDecl>();
    node->id = ctx->ID()->getText();
    node->values = parseArray2D(ctx->array2D());
    return node;
}

// -----------------------------
// maskDecl
// -----------------------------
std::unique_ptr<IRNode> ASTBuilderVisitor::visitMaskDecl(ImageLangParser::MaskDeclContext* ctx) {
    auto node = make_unique<IRMaskDecl>();
    node->id = ctx->ID()->getText();
    node->values = parseArray2D(ctx->array2D());
    return node;
}

// -----------------------------
// applyMask
// -----------------------------
std::unique_ptr<IRNode> ASTBuilderVisitor::visitApplyMask(ImageLangParser::ApplyMaskContext* ctx) {
    auto node = make_unique<IRApplyMask>();
    node->image = ctx->ID(0)->getText();
    node->mask  = ctx->ID(1)->getText();
    node->x     = stoi(ctx->INT(0)->getText());
    node->y     = stoi(ctx->INT(1)->getText());
    return node;
}

// -----------------------------
// returnStmt
// -----------------------------
std::unique_ptr<IRNode> ASTBuilderVisitor::visitReturnStmt(ImageLangParser::ReturnStmtContext* ctx) {
    auto node = make_unique<IRReturn>();
    node->id = ctx->ID()->getText();
    return node;
}

// -----------------------------
// parse 2D array
// -----------------------------
std::vector<std::vector<int>> ASTBuilderVisitor::parseArray2D(ImageLangParser::Array2DContext* ctx) {
    vector<vector<int>> array;
    for (auto rowCtx : ctx->row()) {
        vector<int> row;
        for (auto intToken : rowCtx->INT()) {
            row.push_back(stoi(intToken->getText()));
        }
        array.push_back(move(row));
    }
    return array;
}