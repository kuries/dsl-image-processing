grammar ImageLang;

program: statement+ EOF;

statement
    : imageDecl
    | maskDecl
    | applyMask
    | returnStmt
    | ';' 
    ;

imageDecl: 'image' ID '=' array2D ';';
maskDecl: 'mask' ID '=' array2D ';';

applyMask: 'apply_mask' '(' ID ',' ID ',' 'x=' INT ',' 'y=' INT ')' ';';

returnStmt: 'return' ID ';';

array2D: '[' row (',' row)* ']';
row: '[' INT (',' INT)* ']';

ID: [a-zA-Z_][a-zA-Z_0-9]*;
INT: [0-9]+;

WS: [ \t\r\n]+ -> skip;
