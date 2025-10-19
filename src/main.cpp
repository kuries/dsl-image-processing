#include "antlr4-runtime.h"
#include "ImageLangLexer.h"
#include "ImageLangParser.h"
#include "ImageLangVisitorImpl.h"   // ✅ include the visitor header
#include <fstream>
#include <iostream>

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

    antlr4::tree::ParseTree *tree = parser.program();

    ImageLangVisitorImpl visitor;
    visitor.visit(tree);

    return 0;
}
