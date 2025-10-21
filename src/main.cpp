#include "antlr4-runtime.h"
#include "ImageLangLexer.h"
#include "ImageLangParser.h"
#include "ImageLangVisitorImpl.h"   // ✅ include the visitor header
#include "AST.h"
#include <fstream>
#include <iostream>


// Recursive pretty printer
void printParseTree(antlr4::tree::ParseTree *tree, const std::string &indent = "", bool last = true) {
    std::cout << indent;
    if (last)
        std::cout << "└─";
    else
        std::cout << "├─";

    std::string nodeText = tree->toString();
    std::cout << nodeText << std::endl;

    auto children = tree->children;
    for (size_t i = 0; i < children.size(); ++i) {
        printParseTree(children[i], indent + (last ? "  " : "│ "), i == children.size() - 1);
    }
}


int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: image-dsl <file.imgdsl>\n";
        return 1;
    }

    std::ifstream stream(argv[1]);
    antlr4::ANTLRInputStream input(stream);
    ImageLangLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    ImageLangParser parser(&tokens);

    // antlr4::tree::ParseTree *tree = parser.program();
    // printParseTree(tree);
    
    // ImageLangVisitorImpl visitor;
    // visitor.visit(tree);

    ImageLangParser::ProgramContext* programCtx = parser.program();

    ASTBuilderVisitor builder;
    auto ASTProgram = builder.build(programCtx);

    std::cout << "IR generated with " << ASTProgram->size() << " statements.\n";

    IRPrinter::print(*ASTProgram);

    return 0;
}
