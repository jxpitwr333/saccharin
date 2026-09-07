#ifndef AST_TYPES_H
#define AST_TYPES_H

#ifndef UNITY_BUILD
    #include <stddef.h>
    #include <stdint.h>
#endif

typedef struct Parser Parser;
typedef struct Expr Expr;
typedef struct Token Token;

typedef enum {
	EXPR_BINARY,
	EXPR_UNARY,
	EXPR_NUMBER,
	EXPR_BLOCK,
	EXPR_CONDITIONAL,
	EXPR_FUN,
	EXPR_VAR_DECL,
	EXPR_VAR_READ
} ExprKind;

typedef enum {
	TOKEN_NONE,
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_STAR,
	TOKEN_SLASH,
	TOKEN_SEMICOLON,
	TOKEN_NUMBER_LITERAL,
	TOKEN_EQUAL, // =
	TOKEN_EQUAL_EQUAL, // ==
	TOKEN_BANG,
	TOKEN_BANG_EQUAL, // !=
	TOKEN_GREATER, // >
	TOKEN_LESS, // <
	TOKEN_GREATER_EQUAL, // >=
	TOKEN_LESS_EQUAL, // <=
	TOKEN_IDENTIFIER,
	TOKEN_IF,
	TOKEN_ELSE,
	TOKEN_AND,
	TOKEN_OR,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_FOR,
    TOKEN_FUN,
    TOKEN_RETURN,
    TOKEN_WHILE,
    TOKEN_LET,
	TOKEN_EOF
} TokenKind;

typedef struct {
    const char* name;
    size_t length;
    int64_t slot;
} Symbol;

typedef struct {
    Symbol* items;
    int64_t capacity;
    int64_t count;
} SymbolList;

#endif
