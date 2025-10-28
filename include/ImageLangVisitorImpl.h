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
        if (ctx->imageDecl())           return buildImageDecl(ctx->imageDecl());
        if (ctx->saveStmt())            return buildSaveStmt(ctx->saveStmt());
        if (ctx->pixelAssign())         return buildPixelAssign(ctx->pixelAssign());
        if (ctx->numDecl())             return buildNumDecl(ctx->numDecl());
        if (ctx->numAssign())           return buildNumAssign(ctx->numAssign());
        if (ctx->applyThreshold())      return buildApplyThreshold(ctx->applyThreshold());
        if (ctx->applyBoxBlur())        return buildApplyBoxBlur(ctx->applyBoxBlur());
        
        std::cout<<"Gotcha !\n";
        return nullptr;
    }

    std::unique_ptr<ProgramAST> buildApplyBoxBlur(ImageLangParser::ApplyBoxBlurContext *ctx)
    {
        std::vector<std::unique_ptr<ExprAST>> body;

        std::string imageName = ctx->ID()->getText();

        auto kernelWidthExpr = buildExpr(ctx->expr());

        // Variable names
        auto kernelWidthVar = "kernelWidth";
        auto radiusVar = "radius";
        auto negRadiusVar = "negRadius";
        auto sumVar = "sum";
        auto countVar = "count";
        //image nested for loop
        auto iVar = "i";
        auto jVar = "j";
        //kernel nested for loop
        auto dxVar = "dx";
        auto dyVar = "dy";
        auto nxVar = "nx";
        auto nyVar = "ny";

        // kernelWidth = <expr>
        auto kernelWidthAssign = std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(kernelWidthVar),
            std::move(kernelWidthExpr)
        );

        // radius = kernelWidth / 2 + 1
        auto radiusAssign = std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(radiusVar),
            std::make_unique<BinaryExprAST>(
                "/",
                std::make_unique<VariableExprAST>(kernelWidthVar),
                std::make_unique<NumberExprAST>(2)
            )
        );

        //for for loop checking 
        auto radiusAssignIncr = std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(radiusVar),
            std::make_unique<BinaryExprAST>(
                "/",
                std::make_unique<VariableExprAST>(radiusVar),
                std::make_unique<NumberExprAST>(2)
            )
        );

        // radius = kernelWidth / 2
        auto negRadiusAssign = std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(negRadiusVar),
            std::make_unique<BinaryExprAST>(
                "*",
                std::make_unique<VariableExprAST>(radiusVar),
                std::make_unique<NumberExprAST>(-1)
            )
        );

        body.push_back(std::move(kernelWidthAssign));
        body.push_back(std::move(radiusAssign));
        body.push_back(std::move(radiusAssignIncr));
        body.push_back(std::move(negRadiusAssign));
        

        // Outer loops over image (i, j)
        // for (i = 0; i < image.height; i++)
        // for (j = 0; j < image.width; j++)

        // ----- Inner dx, dy loops -----
        // Create loop body for dx, dy: accumulate sum and count
        std::vector<std::unique_ptr<ExprAST>> dxBody;

        // nx = j + dx
        auto nxAssign = std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(nxVar),
            std::make_unique<BinaryExprAST>(
                "+",
                std::make_unique<VariableExprAST>(jVar),
                std::make_unique<VariableExprAST>(dxVar)
            )
        );

        // ny = i + dy
        auto nyAssign = std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(nyVar),
            std::make_unique<BinaryExprAST>(
                "+",
                std::make_unique<VariableExprAST>(iVar),
                std::make_unique<VariableExprAST>(dyVar)
            )
        );

        // condition: nx >= 0 && nx < image.width && ny >= 0 && ny < image.height
        auto nxInBounds = std::make_unique<BinaryExprAST>(
            "&&",
            std::make_unique<BinaryExprAST>(
                ">=",
                std::make_unique<VariableExprAST>(nxVar),
                std::make_unique<NumberExprAST>(0)
            ),
            std::make_unique<BinaryExprAST>(
                "<",
                std::make_unique<VariableExprAST>(nxVar),
                std::make_unique<VariableExprAST>(imageName + ".width")
            )
        );

        auto nyInBounds = std::make_unique<BinaryExprAST>(
            "&&",
            std::make_unique<BinaryExprAST>(
                ">=",
                std::make_unique<VariableExprAST>(nyVar),
                std::make_unique<NumberExprAST>(0)
            ),
            std::make_unique<BinaryExprAST>(
                "<",
                std::make_unique<VariableExprAST>(nyVar),
                std::make_unique<VariableExprAST>(imageName + ".height")
            )
        );

        auto fullCondition = std::make_unique<BinaryExprAST>(
            "&&",
            std::move(nxInBounds),
            std::move(nyInBounds)
        );

        // sum = sum + image[ny][nx]
        auto sumUpdate = std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(sumVar),
            std::make_unique<BinaryExprAST>(
                "+",
                std::make_unique<VariableExprAST>(sumVar),
                std::make_unique<ArrayAccessExprAST>(
                    imageName,
                    std::make_unique<VariableExprAST>(nyVar),
                    std::make_unique<VariableExprAST>(nxVar)
                )
            )
        );

        // count = count + 1
        auto countUpdate = std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(countVar),
            std::make_unique<BinaryExprAST>(
                "+",
                std::make_unique<VariableExprAST>(countVar),
                std::make_unique<NumberExprAST>(1)
            )
        );

        std::vector<std::unique_ptr<ExprAST>> ifBody;
        ifBody.push_back(std::move(sumUpdate));
        ifBody.push_back(std::move(countUpdate));

        auto ifCondition = std::make_unique<IfExprAST>(
            std::move(fullCondition),
            std::move(ifBody),
            std::vector<std::unique_ptr<ExprAST>>{} //no else
        );

        std::vector<std::unique_ptr<ExprAST>> dxLoopBody;
        dxLoopBody.push_back(std::move(nxAssign));
        dxLoopBody.push_back(std::move(nyAssign));
        dxLoopBody.push_back(std::move(ifCondition));

        // dx loop
        auto dxLoop = std::make_unique<ForExprAST>(
            dxVar,
            std::make_unique<VariableExprAST>(negRadiusVar),
            std::make_unique<VariableExprAST>(radiusVar),
            std::make_unique<NumberExprAST>(1),
            std::move(dxLoopBody)
        );

        // dy loop wraps dx loop
        std::vector<std::unique_ptr<ExprAST>> dyBody;
        dyBody.push_back(std::move(dxLoop));

        auto dyLoop = std::make_unique<ForExprAST>(
            dyVar,
            std::make_unique<VariableExprAST>(negRadiusVar),
            std::make_unique<VariableExprAST>(radiusVar),
            std::make_unique<NumberExprAST>(1),
            std::move(dyBody)
        );

        // sum/count assign to image[i][j]
        auto assignBlur = std::make_unique<AssignExprAST>(
            std::make_unique<ArrayAccessExprAST>(
                imageName,
                std::make_unique<VariableExprAST>(iVar),
                std::make_unique<VariableExprAST>(jVar)
            ),
            std::make_unique<BinaryExprAST>(
                "/",
                std::make_unique<VariableExprAST>(sumVar),
                std::make_unique<VariableExprAST>(countVar)
            )
        );

        // inner body (for each pixel)
        std::vector<std::unique_ptr<ExprAST>> innerBody;
        innerBody.push_back(std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(sumVar),
            std::make_unique<NumberExprAST>(0)
        ));
        innerBody.push_back(std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(countVar),
            std::make_unique<NumberExprAST>(0)
        ));
        innerBody.push_back(std::move(dyLoop));
        innerBody.push_back(std::move(assignBlur));

        // j loop
        auto jLoop = std::make_unique<ForExprAST>(
            jVar,
            std::make_unique<NumberExprAST>(0),
            std::make_unique<VariableExprAST>(imageName + ".width"),
            std::make_unique<NumberExprAST>(1),
            std::move(innerBody)
        );

        std::vector<std::unique_ptr<ExprAST>> outerBody;
        outerBody.push_back(std::move(jLoop));

        // i loop
        auto iLoop = std::make_unique<ForExprAST>(
            iVar,
            std::make_unique<NumberExprAST>(0),
            std::make_unique<VariableExprAST>(imageName + ".height"),
            std::make_unique<NumberExprAST>(1),
            std::move(outerBody)
        );

        body.push_back(std::move(iLoop));

        auto program = std::make_unique<ProgramAST>();
        for (auto &stmt : body)
            program->addStmt(std::move(stmt));

        return program;
    }



    std::unique_ptr<ProgramAST> buildApplyThreshold(ImageLangParser::ApplyThresholdContext *ctx)
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
            ">", 
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
            char op = opText[0]; // "+" or "-"
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
            char op = opText[0]; // "*" or "/"
            auto rhs = buildPrimary(ctx->primary(i));
            lhs = std::make_unique<BinaryExprAST>(op, std::move(lhs), std::move(rhs));
        }

        return lhs;
    }

    std::unique_ptr<ExprAST> buildNumDecl(ImageLangParser::NumDeclContext *ctx) 
    {
        std::string name = ctx->ID()->getText();
        std::unique_ptr<ExprAST> initExpr = nullptr;

        if (ctx->expr()) {
            initExpr = buildExpr(ctx->expr());
        }

        return std::make_unique<VarDeclExprAST>(name, std::move(initExpr));
    }

    std::unique_ptr<ExprAST> buildNumAssign(ImageLangParser::NumAssignContext *ctx) 
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
        if (ctx->NUM_LITERAL())
        {
            double val = std::stod(ctx->NUM_LITERAL()->getText());
            return std::make_unique<NumberExprAST>(val);
        }

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

    static std::string stripQuotes(const std::string &s) {
        if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
            return s.substr(1, s.size() - 2);
        return s;
    }
    
};