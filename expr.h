#ifndef EXPR_H
#define EXPR_H

#ifndef UNITY_BUILD
    #include "ast_types.h"
    #include <stdint.h>
    #include <stddef.h>
#endif

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
		    Expr* condition;
			Expr* thenBranch;
			Expr* elseBranch;
		} conditional;

		int64_t number;
	} as;
};

typedef struct {
    Expr** items;
    size_t capacity;
    size_t count;
} ExprList;

Expr* parsePrimary(Parser* p);
Expr* parseExpr(Parser* p, int minPrec);
Expr* makeNumber(Parser* p, int64_t value);
Expr* makeBinary(Parser* p, Expr* left, Expr* right, TokenKind op);
Expr* makeUnary(Parser* p, Expr* right, TokenKind op);
Expr* makeBlock(Parser* p);
Expr* makeConditional(Parser* p, Expr* condition, Expr* thenBranch, Expr* elseBranch);

#endif
