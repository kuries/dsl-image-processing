grammar ImageLang;

program: statement+ EOF;

statement
    : imageDecl
    | maskDecl
    | applyMask
    | saveStmt
    | ';' 
    ;

// ===== Image and Mask Declarations =====
imageDecl: 'image' ID '=' loadExpr ';';
maskDecl: 'mask' ID '=' array2D ';';

// ===== Load and Save =====
loadExpr: 'load' '(' STRING ')' ;        // e.g., load("path")
saveStmt: ID '.' 'save' '(' STRING ')' ';'; // e.g., img.save("path")

// ===== Mask application =====
applyMask: 'apply_mask' '(' ID ',' ID ',' 'x=' INT ',' 'y=' INT ')' ';';

// ===== Arrays for masks =====
array2D: '[' row (',' row)* ']';
row: '[' INT (',' INT)* ']';

// ===== Lexical rules =====
ID: [a-zA-Z_][a-zA-Z_0-9]*;
INT: [0-9]+;
STRING: '"' (~["\r\n])* '"';

WS: [ \t\r\n]+ -> skip;
