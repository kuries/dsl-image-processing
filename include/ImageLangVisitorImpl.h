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
            else
            {
                auto stmts = buildFunctionStatement(stmtCtx);
                for(int i=0; i<stmts.size(); i++)
                    program->addStmt(std::move(stmts[i]));
            }
        }
        return program;
    }

private:
    std::unique_ptr<ExprAST> buildStatement(ImageLangParser::StatementContext *ctx) 
    {
        if (ctx->imageDecl())               return buildImageDecl(ctx->imageDecl());
        if (ctx->saveStmt())                return buildSaveStmt(ctx->saveStmt());
        if (ctx->pixelAssign())             return buildPixelAssign(ctx->pixelAssign());
        if (ctx->numDecl())                 return buildNumDecl(ctx->numDecl());
        if (ctx->numAssign())               return buildNumAssign(ctx->numAssign());
        return nullptr;
    }

    std::vector<std::unique_ptr<ExprAST>> buildFunctionStatement(ImageLangParser::StatementContext *ctx) {
        // if (ctx->applyThreshold())          return buildApplyThreshold(ctx->applyThreshold());
        // if (ctx->applyBoxBlur())            return buildApplyBoxBlur(ctx->applyBoxBlur());
        if (ctx->applyAdjustBrightness())   return buildApplyBrightnessAdjust(ctx->applyAdjustBrightness());
        // if (ctx->applyAdjustContrast())     return buildApplyContrastAdjust(ctx->applyAdjustContrast());
        return std::vector<std::unique_ptr<ExprAST>>{};
    }

    std::vector<std::unique_ptr<ExprAST>> buildApplyBrightnessAdjust(ImageLangParser::ApplyAdjustBrightnessContext *ctx)
    {
        std::vector<std::unique_ptr<ExprAST>> body;

        std::string imageName = ctx->ID()->getText();

        auto brightnessExpr = buildExpr(ctx->expr());

        auto brightnessStr = "brightness";

        auto brightnessAssign = std::make_unique<VarDeclExprAST>(
            brightnessStr,
            std::move(brightnessExpr)
        );

        auto newValAssign = std::make_unique<VarDeclExprAST>(
            newValStr,
            std::make_unique<NumberExprAST>(0)
        );

        body.push_back(std::move(brightnessAssign));
        body.push_back(std::move(newValAssign));

        auto iVar = "i";
        auto jVar = "j";
        auto kVar = "k"; //for RGB
        
        auto overwritePixel = std::make_unique<AssignExprAST>(
            std::make_unique<ArrayAccessExprAST>(
                imageName,
                std::make_unique<VariableExprAST>(iVar),
                std::make_unique<VariableExprAST>(jVar),
                std::make_unique<VariableExprAST>(kVar)
            ),
            std::make_unique<BinaryExprAST>(
                "+", 
                std::make_unique<ArrayAccessExprAST>(
                    imageName,
                    std::make_unique<VariableExprAST>(iVar),
                    std::make_unique<VariableExprAST>(jVar),
                    std::make_unique<VariableExprAST>(kVar)
                ),
                std::make_unique<VariableExprAST>(brightnessStr)
            )
        );

        std::vector<std::unique_ptr<ExprAST>> RBG_Body;
        RBG_Body.push_back(std::move(overwritePixel));

        // RGB loop (k)
        auto RGB_Loop = std::make_unique<ForExprAST>(
            kVar,
            std::make_unique<NumberExprAST>(0),
            std::make_unique<NumberExprAST>(3),
            std::make_unique<NumberExprAST>(1),
            std::move(RBG_Body)
        );

        std::vector<std::unique_ptr<ExprAST>> innerBody;
        innerBody.push_back(std::move(RGB_Loop));

        // Inner loop (j)
        auto innerLoop = std::make_unique<ForExprAST>(
            jVar,
            std::make_unique<NumberExprAST>(0),
            std::make_unique<VariableExprAST>(imageName + ".width"),
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

        
        std::vector<std::unique_ptr<ExprAST>> stmtList = std::vector<std::unique_ptr<ExprAST>>{};

        for (auto &stmt : body)
            stmtList.push_back(std::move(stmt));
        //need to do normalization
        // std::vector<std::unique_ptr<ExprAST>> ImgNormalizationVector = ImageNormalization(imageName);

        // for (auto &stmt : ImgNormalizationVector)
        //     stmtList.push_back(std::move(stmt));
        

        return stmtList;
    }

    std::unique_ptr<ProgramAST> buildApplyContrastAdjust(ImageLangParser::ApplyAdjustContrastContext *ctx)
    {
        std::vector<std::unique_ptr<ExprAST>> body;

        std::string imageName = ctx->ID()->getText();

        auto contrastExpr = buildExpr(ctx->expr());

        auto contrastStr = "contrast";

        auto contrastAssign = std::make_unique<VarDeclExprAST>(
            (contrastStr),
            std::move(contrastExpr)
        );

        body.push_back(std::move(contrastAssign));

        auto iVar = "i";
        auto jVar = "j";
        auto kVar = "k"; //for RGB
        
        auto overwritePixel = std::make_unique<AssignExprAST>(
            std::make_unique<ArrayAccessExprAST>(
                imageName,
                std::make_unique<VariableExprAST>(iVar),
                std::make_unique<VariableExprAST>(jVar),
                std::make_unique<VariableExprAST>(kVar)
            ),
            std::make_unique<BinaryExprAST>(
                "+", 
                std::make_unique<BinaryExprAST>(
                    "*", 
                    std::make_unique<BinaryExprAST>(
                        "-", 
                        std::make_unique<ArrayAccessExprAST>(
                            imageName,
                            std::make_unique<VariableExprAST>(iVar),
                            std::make_unique<VariableExprAST>(jVar),
                            std::make_unique<VariableExprAST>(kVar)
                        ),
                        std::make_unique<NumberExprAST>(128)
                    ),
                    std::make_unique<VariableExprAST>(contrastStr)
                ),
                std::make_unique<NumberExprAST>(128)
            )
        );


        std::vector<std::unique_ptr<ExprAST>> RGB_Body;

        RGB_Body.push_back(std::move(overwritePixel));

        // RGB loop (k)
        auto RGB_Loop = std::make_unique<ForExprAST>(
            kVar,
            std::make_unique<NumberExprAST>(0),
            std::make_unique<NumberExprAST>(3),
            std::make_unique<NumberExprAST>(1),
            std::move(RGB_Body)
        );


        std::vector<std::unique_ptr<ExprAST>> innerBody;

        innerBody.push_back(std::move(RGB_Loop));

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

        std::vector<std::unique_ptr<ExprAST>> ImgNormalizationVector = ImageNormalization(imageName);

        for (auto &stmt : ImgNormalizationVector)
            program->addStmt(std::move(stmt));

        return program;
    }

    std::vector<std::unique_ptr<ExprAST>> ImageNormalization(std::string imageName)
    {
        std::vector<std::unique_ptr<ExprAST>> ImgNormalizationVector;

        auto minVar = "min";
        auto maxVar = "max";
        auto iVar = "i";
        auto jVar = "j";
        auto kVar = "k";
        
        std::vector<std::unique_ptr<ExprAST>> RGB_Body;

        //happens for every channel

        auto minAssign = std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(minVar),
            std::make_unique<NumberExprAST>(INT_MAX)
        );

        auto maxAssign = std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(minVar),
            std::make_unique<NumberExprAST>(INT_MIN)
        );

        RGB_Body.push_back(std::move(minAssign));
        RGB_Body.push_back(std::move(maxAssign));

        std::vector<std::unique_ptr<ExprAST>> maxIfConditionVector;
        maxIfConditionVector.push_back(
            std::move(
                std::make_unique<AssignExprAST>(
                    std::make_unique<VariableExprAST>(maxVar),
                    std::make_unique<ArrayAccessExprAST>(
                        imageName,
                        std::make_unique<VariableExprAST>(iVar),
                        std::make_unique<VariableExprAST>(jVar),
                        std::make_unique<VariableExprAST>(kVar)
                    )
                )
            )
        );
        

        auto maxIfCondition = std::make_unique<IfExprAST>(
            std::move(
                std::make_unique<BinaryExprAST>(
                    "<",
                    std::make_unique<VariableExprAST>(maxVar),
                    std::make_unique<ArrayAccessExprAST>(
                        imageName,
                        std::make_unique<VariableExprAST>(iVar),
                        std::make_unique<VariableExprAST>(jVar),
                        std::make_unique<VariableExprAST>(kVar)
                    )
                )
            ),
            std::move(maxIfConditionVector),
            std::vector<std::unique_ptr<ExprAST>>{} //no else
        );

        std::vector<std::unique_ptr<ExprAST>> minIfConditionVector;
        minIfConditionVector.push_back(
            std::move(
                std::make_unique<AssignExprAST>(
                    std::make_unique<VariableExprAST>(minVar),
                    std::make_unique<ArrayAccessExprAST>(
                        imageName,
                        std::make_unique<VariableExprAST>(iVar),
                        std::make_unique<VariableExprAST>(jVar),
                        std::make_unique<VariableExprAST>(kVar)
                    )
                )
            )
        );

        auto minIfCondition = std::make_unique<IfExprAST>(
            std::move(
                std::make_unique<BinaryExprAST>(
                    ">",
                    std::make_unique<VariableExprAST>(minVar),
                    std::make_unique<ArrayAccessExprAST>(
                        imageName,
                        std::make_unique<VariableExprAST>(iVar),
                        std::make_unique<VariableExprAST>(jVar),
                        std::make_unique<VariableExprAST>(kVar)
                    )
                )
            ),
            std::move(minIfConditionVector),
            std::vector<std::unique_ptr<ExprAST>>{} //no else
        );


        std::vector<std::unique_ptr<ExprAST>> innerBody1;

        innerBody1.push_back(std::move(maxIfCondition));
        innerBody1.push_back(std::move(minIfCondition));

        // Inner loop (j)
        auto innerLoop1 = std::make_unique<ForExprAST>(
            jVar,
            std::make_unique<NumberExprAST>(0),
            std::make_unique<VariableExprAST>(imageName + ".mask_width"),
            std::make_unique<NumberExprAST>(1),
            std::move(innerBody1)
        );

        std::vector<std::unique_ptr<ExprAST>> outerBody1;
        outerBody1.push_back(std::move(innerLoop1));

        // Outer loop (i)
        auto outerLoop1 = std::make_unique<ForExprAST>(
            iVar,
            std::make_unique<NumberExprAST>(0),
            std::make_unique<VariableExprAST>(imageName + ".height"),
            std::make_unique<NumberExprAST>(1),
            std::move(outerBody1)
        );

        RGB_Body.push_back(std::move(outerLoop1));


        //normalization body

        //we don't want divide by 0 error, so if min == max, then do max++;

        std::vector<std::unique_ptr<ExprAST>> minMaxEqualityIfConditionVector;
        minMaxEqualityIfConditionVector.push_back(
            std::move(
                std::make_unique<AssignExprAST>(
                    std::make_unique<VariableExprAST>(maxVar),
                    std::make_unique<BinaryExprAST>(
                        "+",
                        std::make_unique<VariableExprAST>(maxVar),
                        std::make_unique<NumberExprAST>(1)
                    )
                )
            )
        );
        
        auto minMaxEqualityIfCondition = std::make_unique<IfExprAST>(
            std::move(
                std::make_unique<BinaryExprAST>(
                    "==",
                    std::make_unique<VariableExprAST>(minVar),
                    std::make_unique<VariableExprAST>(maxVar)
                )
            ),
            std::move(minMaxEqualityIfConditionVector),
            std::vector<std::unique_ptr<ExprAST>>{} //no else
        );

        RGB_Body.push_back(std::move(minMaxEqualityIfCondition));

        auto normalisedPixelAssign = std::make_unique<AssignExprAST>(
            std::make_unique<ArrayAccessExprAST>(
                imageName,
                std::make_unique<VariableExprAST>(iVar),
                std::make_unique<VariableExprAST>(jVar),
                std::make_unique<VariableExprAST>(kVar)
            ),
            std::make_unique<BinaryExprAST>
            (
                "*",
                std::make_unique<NumberExprAST>(255),
                std::make_unique<BinaryExprAST>(
                    "/",
                    std::make_unique<BinaryExprAST>(
                        "-",
                        std::make_unique<ArrayAccessExprAST>(
                            imageName,
                            std::make_unique<VariableExprAST>(iVar),
                            std::make_unique<VariableExprAST>(jVar),
                            std::make_unique<VariableExprAST>(kVar)
                        ),
                        std::make_unique<VariableExprAST>(minVar)
                    ),
                    std::make_unique<BinaryExprAST>(
                        "-",
                        std::make_unique<VariableExprAST>(maxVar),
                        std::make_unique<VariableExprAST>(minVar)
                    )
                )
            )
        );

        //loop for normalization

        std::vector<std::unique_ptr<ExprAST>> innerBody2;

        innerBody2.push_back(std::move(normalisedPixelAssign));

        // Inner loop (j)
        auto innerLoop2 = std::make_unique<ForExprAST>(
            jVar,
            std::make_unique<NumberExprAST>(0),
            std::make_unique<VariableExprAST>(imageName + ".mask_width"),
            std::make_unique<NumberExprAST>(1),
            std::move(innerBody2)
        );

        std::vector<std::unique_ptr<ExprAST>> outerBody2;
        outerBody2.push_back(std::move(innerLoop2));

        // Outer loop (i)
        auto outerLoop2 = std::make_unique<ForExprAST>(
            iVar,
            std::make_unique<NumberExprAST>(0),
            std::make_unique<VariableExprAST>(imageName + ".height"),
            std::make_unique<NumberExprAST>(1),
            std::move(outerBody2)
        );

        RGB_Body.push_back(std::move(outerLoop2));

        // RJB loop (k)
        auto RJBLoop = std::make_unique<ForExprAST>(
            kVar,
            std::make_unique<NumberExprAST>(0),
            std::make_unique<NumberExprAST>(3),
            std::make_unique<NumberExprAST>(1),
            std::move(RGB_Body)
        );

        ImgNormalizationVector.push_back(std::move(RJBLoop));

        return ImgNormalizationVector;
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
        auto sumVar_r = "sum_r";
        auto sumVar_g = "sum_g";
        auto sumVar_b = "sum_b";
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
        auto kernelWidthAssign = std::make_unique<VarDeclExprAST>(
            (kernelWidthVar),
            std::move(kernelWidthExpr)
        );

        // radius = kernelWidth / 2 + 1
        auto radiusAssign = std::make_unique<VarDeclExprAST>(
            (radiusVar),
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
        auto negRadiusAssign = std::make_unique<VarDeclExprAST>(
            (negRadiusVar),
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
        auto nxAssign = std::make_unique<VarDeclExprAST>(
            (nxVar),
            std::make_unique<BinaryExprAST>(
                "+",
                std::make_unique<VariableExprAST>(jVar),
                std::make_unique<VariableExprAST>(dxVar)
            )
        );

        // ny = i + dy
        auto nyAssign = std::make_unique<VarDeclExprAST>(
            (nyVar),
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

        // sum_r = sum_r + image[ny][nx][0]
        auto sumUpdate_r = std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(sumVar_r),
            std::make_unique<BinaryExprAST>(
                "+",
                std::make_unique<VariableExprAST>(sumVar_r),
                std::make_unique<ArrayAccessExprAST>(
                    imageName,
                    std::make_unique<VariableExprAST>(nyVar),
                    std::make_unique<VariableExprAST>(nxVar),
                    std::make_unique<NumberExprAST>(0)
                )
            )
        );

        // sum_g = sum_g + image[ny][nx][0]
        auto sumUpdate_g = std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(sumVar_g),
            std::make_unique<BinaryExprAST>(
                "+",
                std::make_unique<VariableExprAST>(sumVar_g),
                std::make_unique<ArrayAccessExprAST>(
                    imageName,
                    std::make_unique<VariableExprAST>(nyVar),
                    std::make_unique<VariableExprAST>(nxVar),
                    std::make_unique<NumberExprAST>(1)
                )
            )
        );

        // sum_b = sum_b + image[ny][nx][0]
        auto sumUpdate_b = std::make_unique<AssignExprAST>(
            std::make_unique<VariableExprAST>(sumVar_b),
            std::make_unique<BinaryExprAST>(
                "+",
                std::make_unique<VariableExprAST>(sumVar_b),
                std::make_unique<ArrayAccessExprAST>(
                    imageName,
                    std::make_unique<VariableExprAST>(nyVar),
                    std::make_unique<VariableExprAST>(nyVar),
                    std::make_unique<NumberExprAST>(2)
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
        ifBody.push_back(std::move(sumUpdate_r));
        ifBody.push_back(std::move(sumUpdate_g));
        ifBody.push_back(std::move(sumUpdate_b));
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

        // sum_r/count assign to image[i][j][0]
        auto assignBlur_r = std::make_unique<AssignExprAST>(
            std::make_unique<ArrayAccessExprAST>(
                imageName,
                std::make_unique<VariableExprAST>(iVar),
                std::make_unique<VariableExprAST>(jVar),
                std::make_unique<NumberExprAST>(0)
            ),
            std::make_unique<BinaryExprAST>(
                "/",
                std::make_unique<VariableExprAST>(sumVar_r),
                std::make_unique<VariableExprAST>(countVar)
            )
        );

        // sum_g/count assign to image[i][j][1]
        auto assignBlur_g = std::make_unique<AssignExprAST>(
            std::make_unique<ArrayAccessExprAST>(
                imageName,
                std::make_unique<VariableExprAST>(iVar),
                std::make_unique<VariableExprAST>(jVar),
                std::make_unique<NumberExprAST>(1)
            ),
            std::make_unique<BinaryExprAST>(
                "/",
                std::make_unique<VariableExprAST>(sumVar_g),
                std::make_unique<VariableExprAST>(countVar)
            )
        );

        // sum_b/count assign to image[i][j][2]
        auto assignBlur_b = std::make_unique<AssignExprAST>(
            std::make_unique<ArrayAccessExprAST>(
                imageName,
                std::make_unique<VariableExprAST>(iVar),
                std::make_unique<VariableExprAST>(jVar),
                std::make_unique<NumberExprAST>(2)
            ),
            std::make_unique<BinaryExprAST>(
                "/",
                std::make_unique<VariableExprAST>(sumVar_b),
                std::make_unique<VariableExprAST>(countVar)
            )
        );

        // inner body (for each pixel)
        std::vector<std::unique_ptr<ExprAST>> innerBody;
        innerBody.push_back(std::make_unique<VarDeclExprAST>(
            (sumVar_r),
            std::make_unique<NumberExprAST>(0)
        ));
        innerBody.push_back(std::make_unique<VarDeclExprAST>(
            (sumVar_g),
            std::make_unique<NumberExprAST>(0)
        ));
        innerBody.push_back(std::make_unique<VarDeclExprAST>(
            (sumVar_b),
            std::make_unique<NumberExprAST>(0)
        ));
        innerBody.push_back(std::make_unique<VarDeclExprAST>(
            (countVar),
            std::make_unique<NumberExprAST>(0)
        ));
        innerBody.push_back(std::move(dyLoop));
        innerBody.push_back(std::move(assignBlur_r));
        innerBody.push_back(std::move(assignBlur_g));
        innerBody.push_back(std::move(assignBlur_b));

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

        auto thresholdAssign = std::make_unique<VarDeclExprAST>(
            (thresholdStr),
            std::move(thresholdExpr)
        );

        auto maxValueAssign = std::make_unique<VarDeclExprAST>(
            (maxValueStr),
            std::move(maxValueExpr)
        );

        body.push_back(std::move(thresholdAssign));
        body.push_back(std::move(maxValueAssign));

        auto iVar = "i";
        auto jVar = "j";

        auto condition = std::make_unique<BinaryExprAST>(
            ">", 
            std::make_unique<BinaryExprAST>(
                "+",
                std::make_unique<BinaryExprAST>(
                    "*",
                    std::make_unique<NumberExprAST>(0.299), //Red
                    std::make_unique<ArrayAccessExprAST>(
                        imageName,
                        std::make_unique<VariableExprAST>(iVar),
                        std::make_unique<VariableExprAST>(jVar),
                        std::make_unique<NumberExprAST>(0)
                    )
                ),
                std::make_unique<BinaryExprAST>(
                    "+",
                    std::make_unique<BinaryExprAST>(
                        "*",
                        std::make_unique<NumberExprAST>(0.587), //Green
                        std::make_unique<ArrayAccessExprAST>(
                            imageName,
                            std::make_unique<VariableExprAST>(iVar),
                            std::make_unique<VariableExprAST>(jVar),
                            std::make_unique<NumberExprAST>(1)
                        )
                    ),
                    std::make_unique<BinaryExprAST>(
                        "*",
                        std::make_unique<NumberExprAST>(0.114), //Blue
                        std::make_unique<ArrayAccessExprAST>(
                            imageName,
                            std::make_unique<VariableExprAST>(iVar),
                            std::make_unique<VariableExprAST>(jVar),
                            std::make_unique<NumberExprAST>(2)
                        )
                    )
                )
            ),
            std::make_unique<VariableExprAST>(thresholdStr)
        );

        // auto condition = std::make_unique<BinaryExprAST>(
        //     ">", 
        //     std::make_unique<ArrayAccessExprAST>(
        //         imageName,
        //         std::make_unique<VariableExprAST>(iVar),
        //         std::make_unique<VariableExprAST>(jVar)
        //     ),
        //     std::make_unique<VariableExprAST>(thresholdStr)
        // );

        std::vector<std::unique_ptr<ExprAST>> thenBody;
        thenBody.push_back(
            std::make_unique<AssignExprAST>(
                std::make_unique<ArrayAccessExprAST>(
                    imageName,
                    std::make_unique<VariableExprAST>(iVar),
                    std::make_unique<VariableExprAST>(jVar),
                    std::make_unique<NumberExprAST>(0)
                ),
                std::make_unique<VariableExprAST>(maxValueStr)
            )
        );
        thenBody.push_back(
            std::make_unique<AssignExprAST>(
                std::make_unique<ArrayAccessExprAST>(
                    imageName,
                    std::make_unique<VariableExprAST>(iVar),
                    std::make_unique<VariableExprAST>(jVar),
                    std::make_unique<NumberExprAST>(1)
                ),
                std::make_unique<VariableExprAST>(maxValueStr)
            )
        );
        thenBody.push_back(
            std::make_unique<AssignExprAST>(
                std::make_unique<ArrayAccessExprAST>(
                    imageName,
                    std::make_unique<VariableExprAST>(iVar),
                    std::make_unique<VariableExprAST>(jVar),
                    std::make_unique<NumberExprAST>(2)
                ),
                std::make_unique<VariableExprAST>(maxValueStr)
            )
        );


        std::vector<std::unique_ptr<ExprAST>> elseBody;
        elseBody.push_back(
            std::make_unique<AssignExprAST>(
                std::make_unique<ArrayAccessExprAST>(
                    imageName,
                    std::make_unique<VariableExprAST>(iVar),
                    std::make_unique<VariableExprAST>(jVar),
                    std::make_unique<NumberExprAST>(0)
                ),
                std::make_unique<NumberExprAST>(0)
            )
        );
        elseBody.push_back(
            std::make_unique<AssignExprAST>(
                std::make_unique<ArrayAccessExprAST>(
                    imageName,
                    std::make_unique<VariableExprAST>(iVar),
                    std::make_unique<VariableExprAST>(jVar),
                    std::make_unique<NumberExprAST>(1)
                ),
                std::make_unique<NumberExprAST>(0)
            )
        );
        elseBody.push_back(
            std::make_unique<AssignExprAST>(
                std::make_unique<ArrayAccessExprAST>(
                    imageName,
                    std::make_unique<VariableExprAST>(iVar),
                    std::make_unique<VariableExprAST>(jVar),
                    std::make_unique<NumberExprAST>(2)
                ),
                std::make_unique<NumberExprAST>(0)
            )
        );

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

        std::vector<std::unique_ptr<ExprAST>> ImgNormalizationVector = ImageNormalization(imageName);

        for (auto &stmt : ImgNormalizationVector)
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

        // Case 2: Pixel access like img[x][y][z]
        if (ctx->ID() && ctx->expr().size() == 3)
        {
            std::string imageName = ctx->ID()->getText();
            auto Expr1 = buildExpr(ctx->expr(0));  // first index
            auto Expr2 = buildExpr(ctx->expr(1));  // second index
            auto Expr3 = buildExpr(ctx->expr(2));  // third index

            return std::make_unique<ArrayAccessExprAST>(imageName, std::move(Expr1), std::move(Expr2), std::move(Expr3));
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
        auto index3 = buildExpr(ctx->expr(2));

        auto valueExpr = buildExpr(ctx->expr(2));

        // LHS: ArrayAccessExprAST
        auto lhs = std::make_unique<ArrayAccessExprAST>(imageName, std::move(index1), std::move(index2), std::move(index3));

        // Assignment node
        return std::make_unique<AssignExprAST>(std::move(lhs), std::move(valueExpr));
    }

    static std::string stripQuotes(const std::string &s) {
        if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
            return s.substr(1, s.size() - 2);
        return s;
    }
    
};