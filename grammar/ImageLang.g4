grammar ImageLang;

// ===== Entry Point =====
program
    : statement+ EOF
    ;

// ===== Statements =====
statement
    : imageDecl
    | maskDecl
    | saveStmt
    | pixelAssign
    | intDecl
    | intAssign
    | applyMask
    | applyThreshold
    | applyBoxBlur
    | ';'
    ;

// ===== Image and Mask Declarations =====
imageDecl
    : 'image' ID '=' loadExpr ';'
    ;

maskDecl
    : 'mask' ID '=' array2D ';'
    ;

// ===== Load and Save =====
loadExpr
    : 'load' '(' STRING ')'
    ;

saveStmt
    : ID '.' 'save' '(' STRING ')' ';'
    ;

// ===== Pixel assignment =====
pixelAssign
    : ID '[' expr ']' '[' expr ']' '=' expr ';'
    ;

// ===== Integer variable declarations =====
intDecl
    : 'int' ID ('=' expr)? ';'
    ;

// ===== Integer assignments =====
intAssign
    : ID '=' expr ';'
    ;

// ===== Mask application =====
applyMask
    : 'apply_mask' '(' ID ',' ID ',' 'x=' INT ',' 'y=' INT ')' ';'
    ;

// ===== Threshold application =====
applyThreshold
    : 'apply_threshold' '(' ID ',' expr ',' expr ')' ';'
    ;

// ===== BoxBlur application =====
applyBoxBlur
    : 'apply_boxblur' '(' ID ',' expr ')' ';'
    ;

// ===== Expressions (with precedence) =====
expr
    : addExpr
    ;

addExpr
    : mulExpr (('+' | '-') mulExpr)*
    ;

mulExpr
    : primary (('*' | '/') primary)*
    ;

primary
    : INT
    | ID
    | '(' expr ')'
    | ID '[' expr ']' '[' expr ']'
    ;

// ===== Arrays for masks =====
array2D
    : '[' row (',' row)* ']'
    ;

row
    : '[' INT (',' INT)* ']'
    ;

// ===== Lexical Rules =====
ID
    : [a-zA-Z_][a-zA-Z_0-9]*
    ;

INT
    : [0-9]+
    ;

STRING
    : '"' (~["\r\n])* '"'
    ;

WS
    : [ \t\r\n]+ -> skip
    ;

COMMENT
    : '//' ~[\r\n]* -> skip
    ;

MULTILINE_COMMENT
    : '/*' .*? '*/' -> skip
    ;
