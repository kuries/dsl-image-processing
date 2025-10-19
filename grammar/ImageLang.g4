grammar ImageLang;

// --- Parser Rules ---

program
    : (statement SEMI)* EOF
    ;

statement
    : imageDecl
    | maskDecl
    | assignStmt
    | applyMaskStmt
    | printStmt
    ;

imageDecl
    : 'image' ID '=' array2D
    ;

maskDecl
    : 'mask' ID '=' array2D
    ;

assignStmt
    : ID '=' array2D
    ;

applyMaskStmt
    : 'apply_mask' '(' ID ',' ID ')'
    ;

printStmt
    : 'print' '(' ID ')'
    ;

array2D
    : '[' rowList ']'
    ;

rowList
    : row (',' row)*
    ;

row
    : '[' numberList ']'
    ;

numberList
    : NUMBER (',' NUMBER)*
    ;

// --- Lexer Rules ---

ID          : [a-zA-Z_][a-zA-Z0-9_]* ;
NUMBER      : [0-9]+ ;
SEMI        : ';' ;
WS          : [ \t\r\n]+ -> skip ;
LBRACK      : '[' ;
RBRACK      : ']' ;
COMMA       : ',' ;
LPAREN      : '(' ;
RPAREN      : ')' ;
