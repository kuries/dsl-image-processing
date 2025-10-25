grammar ImageLang;

program: statement+ EOF;

statement
    : imageDecl
    | maskDecl
    | applyMask
    | saveStmt
    | pixelAssign
    | ';'
    ;

// ===== Image and Mask Declarations =====
imageDecl: 'image' ID '=' loadExpr ';';
maskDecl: 'mask' ID '=' array2D ';';

// ===== Load and Save =====
loadExpr: 'load' '(' STRING ')' ;
saveStmt: ID '.' 'save' '(' STRING ')' ';';

// ===== Pixel assignment =====
// grayscale → only 2D indexing
pixelAssign
    : ID '[' INT ']' '[' INT ']' '=' INT ';'   // a[x][y] = 128;
    ;

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
