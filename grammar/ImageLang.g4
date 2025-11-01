grammar ImageLang;

// ===== Entry Point =====
program
    : statement+ EOF
    ;

// ===== Statements =====
statement
    : imageDecl
    | saveStmt
    | pixelAssign
    | numDecl
    | numAssign
    | applyThreshold
    | applyBoxBlur
    | applyAdjustBrightness
    | applyAdjustContrast
    | convertToGreyscale
    | ';'
    ;

// ===== Image and Mask Declarations =====
imageDecl
    : 'image' ID '=' loadExpr ';'
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
    : ID '[' expr ']' '[' expr ']' '[' expr ']' '=' expr ';'
    ;

// ===== Numeric variable declarations =====
numDecl
    : 'num' ID ('=' expr)? ';'
    ;

// ===== Numeric assignments =====
numAssign
    : ID '=' expr ';'
    ;


// ===== Threshold application =====
applyThreshold
    : 'apply_threshold' '(' ID ',' expr ',' expr ')' ';'
    ;

// ===== BoxBlur application =====
applyBoxBlur
    : 'apply_boxblur' '(' ID ',' expr ')' ';'
    ;

// ===== BrightnessAdjust application =====
applyAdjustBrightness
    : 'adjust_brightness' '(' ID ',' expr  ')' ';'
    ;

 // ===== ContrastAdjust application =====
applyAdjustContrast
    : 'adjust_contrast' '(' ID ',' expr  ')' ';'
    ;

 // ===== ConvertToGreyscale application =====
convertToGreyscale
    : 'convertToGreyscale' '(' ID ')' ';'
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
    : NUM_LITERAL
    | ID
    | '(' expr ')'
    | ID '[' expr ']' '[' expr ']'
    ;

// ===== Arrays for masks =====
array2D
    : '[' row (',' row)* ']'
    ;

row
    : '[' NUM_LITERAL (',' NUM_LITERAL)* ']'
    ;

// ===== Lexical Rules =====
ID
    : [a-zA-Z_][a-zA-Z_0-9]*
    ;

// Numbers: integers and floats (like 42, 3.14, .5, 1., 1e3, 2.5E-2)
NUM_LITERAL
    : [0-9]+ ('.' [0-9]*)? ([eE] [+-]? [0-9]+)?
    | '.' [0-9]+ ([eE] [+-]? [0-9]+)?
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
