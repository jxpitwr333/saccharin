#ifndef AST_TYPES_H
#define AST_TYPES_H

#ifndef UNITY_BUILD
    #include <stddef.h>
    #include <stdint.h>
    #include <stdbool.h>
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
	EXPR_CALL,
	EXPR_VAR_DECL,
	EXPR_VAR_READ,
	EXPR_VAR_ASSIGN,
	EXPR_LOGICAL
} ExprKind;

#define TOKEN_LIST(X)\
	X(TOKEN_NONE, "<none>")\
	X(TOKEN_LEFT_PAREN, "(")\
	X(TOKEN_RIGHT_PAREN, ")")\
	X(TOKEN_LEFT_BRACE, "{")\
	X(TOKEN_RIGHT_BRACE, "}")\
	X(TOKEN_PLUS, "+")\
	X(TOKEN_MINUS, "-")\
	X(TOKEN_STAR, "*")\
	X(TOKEN_SLASH, "/")\
	X(TOKEN_SEMICOLON, ";")\
	X(TOKEN_NUMBER_LITERAL, "<number>")\
	X(TOKEN_EQUAL, "=")\
	X(TOKEN_EQUAL_EQUAL, "==")\
	X(TOKEN_BANG, "!")\
	X(TOKEN_BANG_EQUAL, "!=")\
	X(TOKEN_GREATER, ">")\
	X(TOKEN_LESS, "<")\
	X(TOKEN_GREATER_EQUAL, ">=")\
	X(TOKEN_LESS_EQUAL, "<=")\
	X(TOKEN_IDENTIFIER, "<identifier>")\
	X(TOKEN_IF, "if")\
	X(TOKEN_ELSE, "else")\
	X(TOKEN_AND, "and")\
	X(TOKEN_OR, "or")\
	X(TOKEN_TRUE, "true")\
	X(TOKEN_FALSE, "false")\
	X(TOKEN_FOR, "for")\
	X(TOKEN_FUN, "fun")\
	X(TOKEN_RETURN, "return")\
	X(TOKEN_WHILE, "while")\
	X(TOKEN_LET, "let")\
	X(TOKEN_COMMA, ",")\
	X(TOKEN_COLON, ":")\
	X(TOKEN_I64, "i64")\
	X(TOKEN_BOOL, "bool")\
	X(TOKEN_EOF, "<eof>")

typedef enum {
#define X(name, lexeme) name,
	TOKEN_LIST(X)
#undef X
} TokenKind;

typedef enum {
	TYPE_I64,
	TYPE_BOOL,
    TYPE_ERR
} TypeKind;

typedef struct {
	TypeKind kind;
} Type;

typedef struct {
    Type** items;
    size_t count;
    size_t capacity;
} TypeList;

static Type type_bool_inst = { .kind = TYPE_BOOL };
static Type type_i64_inst = { .kind = TYPE_I64 };
static Type type_err_inst = { .kind = TYPE_ERR };

typedef struct {
    const char* name;
    size_t length;
    int64_t slot;
    bool isGlobal;
	Type* type;
} Symbol;

typedef struct {
    Symbol* items;
    int64_t capacity;
    int64_t count;
} SymbolList;

typedef struct {
    int64_t* items;
    int64_t capacity;
    int64_t count;
} ScopeStack;

#endif
