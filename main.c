/*
 * Languages don't need statements.
 * I want saccharin to be completely expression-based.
 * This is what i'm aiming to implement now.
 */
#define FILE_IMPL
#define HASH_IMPL
#define ARENA_IMPL
#include "hashmap.h"
#include "arena.h"
#include "file.h"
#include "macros.h"
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef enum {
	TOKEN_NONE,
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,
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
	TOKEN_ELSEIF,
	TOKEN_AND,
	TOKEN_OR,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_FOR,
    TOKEN_FUN,
    TOKEN_RETURN,
    TOKEN_WHILE,
	TOKEN_EOF
} TokenKind;

typedef struct {
	TokenKind kind;
	size_t start;
	size_t length;
	size_t line;
} Token;

typedef struct {
	Token* items;
	size_t capacity;
	size_t count;
} TokenList;

typedef struct {
	TokenList tokens;
	size_t current;
	size_t start;
	size_t line;
	char* source;
} Lexer;

typedef struct Expr Expr;

typedef enum {
	EXPR_BINARY,
	EXPR_UNARY,
	EXPR_NUMBER,
	EXPR_BLOCK,
	EXPR_CONDITIONAL
} ExprKind;

struct Expr {
	ExprKind kind;
	union {
		struct {
			Expr* right;
			Expr* left;
			TokenKind op;
		} binary;

		struct {
			Expr* right;
			TokenKind op;
		} unary;

		struct {
		    Expr** expressions;
			size_t count;
		} block;

		struct {
		    Expr* expression;
			Expr* ifBranch;
			Expr* elseBranch;
		} conditional;

		int64_t number;
	} as;
};

typedef struct {
	TokenList tokens;
	Arena astArena;
	char* source;
	size_t current;
	size_t line;
} Parser;

bool isAtEnd(Lexer* l);
char advance(Lexer* l);
char peek(Lexer* l);
char peekNext(Lexer* l);
bool match(Lexer* l, char expected);
void addToken(Lexer* l, TokenKind kind);
int precedenceOf(TokenKind kind);
Expr* parsePrimary(Parser* p);
Expr* parseExpr(Parser* p, int minPrec);
bool parserIsAtEnd(Parser* p);
Token tokAdvance(Parser* p);
Token tokPeek(Parser* p);
Token tokPeekNext(Parser* p);
Expr* makeNumber(Parser* p, int64_t value);
Expr* makeBinary(Parser* p, Expr* left, Expr* right, TokenKind operator);
Expr* makeUnary(Parser* p, Expr* right, TokenKind operator);
void printAst(Expr* e);
    int64_t eval(Expr* e);

Table keywords;

int main(void) {
	size_t fileSize = 0;
	char* source = readFileBinary("script.scc", &fileSize);
	if (!source) return 1;

	initTable(&keywords);
	tableSet(&keywords, "and",   INT_VAL(TOKEN_AND));
    tableSet(&keywords, "else",  INT_VAL(TOKEN_ELSE));
	tableSet(&keywords, "elseif",INT_VAL(TOKEN_ELSEIF));
    tableSet(&keywords, "false", INT_VAL(TOKEN_FALSE));
    tableSet(&keywords, "for",   INT_VAL(TOKEN_FOR));
    tableSet(&keywords, "fun",   INT_VAL(TOKEN_FUN));
    tableSet(&keywords, "if",    INT_VAL(TOKEN_IF));
    tableSet(&keywords, "or",    INT_VAL(TOKEN_OR));
    tableSet(&keywords, "return",INT_VAL(TOKEN_RETURN));
    tableSet(&keywords, "true",  INT_VAL(TOKEN_TRUE));
    tableSet(&keywords, "while", INT_VAL(TOKEN_WHILE));

	Lexer lex = {
		.current = 0,
		.start = 0,
		.source = source,
		.line = 1,
		.tokens = {0}
	};

	while (!isAtEnd(&lex)) {
		lex.start = lex.current;
		char c = advance(&lex);

		switch(c) {
			case '(': addToken(&lex, TOKEN_LEFT_PAREN); break;
			case ')': addToken(&lex, TOKEN_RIGHT_PAREN); break;
			case '+': addToken(&lex, TOKEN_PLUS); break;
			case '-': addToken(&lex, TOKEN_MINUS); break;
			case '*': addToken(&lex, TOKEN_STAR); break;
			case '/': addToken(&lex, TOKEN_SLASH); break;
			case ';': addToken(&lex, TOKEN_SEMICOLON); break;

			case '!': addToken(&lex, match(&lex, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG); break;
			case '=': addToken(&lex, match(&lex, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL); break;
			case '<': addToken(&lex, match(&lex, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS); break;
			case '>': addToken(&lex, match(&lex, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER); break;

			case ' ':
			case '\r':
			case '\t':
				break;

			case '\n':
				lex.line++;
				break;

			default:
				if (isdigit(c)) {
                    while (isdigit(peek(&lex))) advance(&lex);
                    addToken(&lex, TOKEN_NUMBER_LITERAL);
                } else if (isalpha(c)) {
					while (isalnum(peek(&lex))) advance(&lex);

					size_t length = lex.current - lex.start;
					char buf[length + 1];
					memcpy(buf, source + lex.start, length);
					buf[length] = '\0';

					TokenKind type = TOKEN_IDENTIFIER;
					Value out = NIL_VAL;
					tableGet(&keywords, buf, &out);
					type = out.as.integer;

					addToken(&lex, type);
				} else {
                    fprintf(stderr, "Unexpected character %c at line %zu\n", c, lex.line);
                }
				break;
		}
	}

    lex.start = lex.current;
    addToken(&lex, TOKEN_EOF);

	Parser parser = {
		.astArena = arenaInit(1 MB),
		.current = 0,
		.line = 1,
		.source = lex.source,
		.tokens = lex.tokens
	};

	Expr* ast = NULL;
	while (!parserIsAtEnd(&parser)) {
		ast = parseExpr(&parser, 0);

		if (tokPeek(&parser).kind == TOKEN_SEMICOLON) {
			tokAdvance(&parser);
		}

		printAst(ast);
		printf(" = %lld\n", (long long)eval(ast));
	}

	free(lex.tokens.items);
	free(lex.source);
	freeTable(&keywords);

	return 0;
}

bool isAtEnd(Lexer* l) {
	return l->source[l->current] == '\0';
}

char advance(Lexer* l) {
	return l->source[l->current++];
}

char peek(Lexer* l) {
	return l->source[l->current];
}

char peekNext(Lexer* l) {
	return l->source[l->current + 1];
}

bool match(Lexer* l, char expected) {
	if (isAtEnd(l)) return false;
	if (l->source[l->current] != expected) return false;
	l->current++;
	return true;
}

void addToken(Lexer* l, TokenKind kind) {
	Token t = {
		.kind = kind,
		.start = l->start,
		.length = l->current - l->start,
		.line = l->line
	};
	da_append(&l->tokens, t);
}

int precedenceOf(TokenKind kind) {
	switch(kind) {
		case TOKEN_EQUAL_EQUAL:
		case TOKEN_BANG_EQUAL:
			return 1;
		case TOKEN_LESS:
		case TOKEN_LESS_EQUAL:
		case TOKEN_GREATER:
		case TOKEN_GREATER_EQUAL:
		     return 2;
		case TOKEN_MINUS:
		case TOKEN_PLUS:
		    return 3;
		case TOKEN_STAR:
		case TOKEN_SLASH:
			return 4;
		default:
			return 0;
	}
}

Expr* parsePrimary(Parser* p) {
    Token t = tokAdvance(p);
    if (t.kind == TOKEN_NUMBER_LITERAL) {
        int64_t val = (int64_t)strtoll(p->source + t.start, NULL, 10);
        return makeNumber(p, val);
    }
    if (t.kind == TOKEN_LEFT_PAREN) {
        Expr* expr = parseExpr(p, 0);
        if (tokAdvance(p).kind != TOKEN_RIGHT_PAREN) {
            fprintf(stderr, "Expected ')'\n");
        }
        return expr;
    }
    if (t.kind == TOKEN_MINUS) {
        Expr* operand = parseExpr(p, 3);
        return makeUnary(p, operand, TOKEN_MINUS);
    }
    return makeNumber(p, 0); // don't return null
}

Expr* parseExpr(Parser* p, int minPrec) {
    Expr* left = parsePrimary(p);

    while (minPrec < precedenceOf(tokPeek(p).kind)) {
        Token op = tokAdvance(p);
        Expr* right = parseExpr(p, precedenceOf(op.kind));
        left = makeBinary(p, left, right, op.kind);
    }

    return left;
}

bool parserIsAtEnd(Parser* p) {
	return (p->tokens.items[p->current].kind == TOKEN_EOF);
}

Token tokAdvance(Parser* p) {
	return p->tokens.items[p->current++];
}

Token tokPeek(Parser* p) {
	return p->tokens.items[p->current];
}

Token tokPeekNext(Parser* p) {
	return p->tokens.items[p->current + 1];
}

Expr* makeNumber(Parser* p, int64_t value) {
	Expr* e = arenaAlloc(&p->astArena, sizeof(Expr));
	e->kind = EXPR_NUMBER;
	e->as.number = value;
	return e;
}

Expr* makeBinary(Parser* p, Expr* left, Expr* right, TokenKind operator) {
	Expr* e = arenaAlloc(&p->astArena, sizeof(Expr));
	e->kind = EXPR_BINARY;
	e->as.binary.left = left;
	e->as.binary.right = right;
	e->as.binary.op = operator;
	return e;
}

Expr* makeUnary(Parser* p, Expr* right, TokenKind operator) {
	Expr* e = arenaAlloc(&p->astArena, sizeof(Expr));
	e->kind = EXPR_UNARY;
	e->as.unary.right = right;
	e->as.unary.op = operator;
	return e;
}

void printAst(Expr* e) {
	if (!e) return;
	if (e->kind == EXPR_NUMBER) {
        printf("%lld", (long long)e->as.number);
    } else if (e->kind == EXPR_UNARY) {
        printf("(-");
        printAst(e->as.unary.right);
        printf(")");
    } else if (e->kind == EXPR_BINARY) {
        printf("(");
        printAst(e->as.binary.left);
        switch (e->as.binary.op) {
            case TOKEN_PLUS:  printf(" + "); break;
            case TOKEN_MINUS: printf(" - "); break;
            case TOKEN_STAR:  printf(" * "); break;
            case TOKEN_SLASH: printf(" / "); break;
            default: break;
        }
        printAst(e->as.binary.right);
        printf(")");
    }
}

int64_t eval(Expr* e) {
    if (!e) return 0;

    switch (e->kind) {
        case EXPR_NUMBER:
            return e->as.number;

		case EXPR_UNARY: {
            int64_t val = eval(e->as.unary.right);

            switch (e->as.unary.op) {
                case TOKEN_MINUS: return -val;
                default: return 0;
            }
        }

        case EXPR_BINARY: {
            int64_t left  = eval(e->as.binary.left);
            int64_t right = eval(e->as.binary.right);

            switch (e->as.binary.op) {
                case TOKEN_PLUS:  return left + right;
                case TOKEN_MINUS: return left - right;
                case TOKEN_STAR:  return left * right;
                case TOKEN_SLASH: return right != 0 ? left / right : 0;
                default: return 0;
            }
        }
    }
    return 0;
}
