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

		struct {
		    int64_t sym;
			Expr* value;
		} varDecl;

		struct {
		    int64_t sym;
		} varRead;

		struct {
		    int64_t sym;
			Expr* newValue;
		} varAssign;

		struct {
			int64_t index;
			Expr* body;
		} function;

		struct {
			int64_t index;
			Expr** args;
			size_t count;
		} call;
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
Expr* makeDecl(Parser* p, int64_t sym, Expr* initializer);
Expr* makeRead(Parser* p, int64_t sym);
Expr* makeAssign(Parser* p, int64_t sym, Expr* newValue);
Expr* makeFunction(Parser* p, int64_t index, Expr* body);
Expr* makeCall(Parser* p, int64_t index, Expr** args, size_t argCount);
Expr* makeLogical(Parser* p, Expr* left, Expr* right, TokenKind op);
Expr* makeInfix(Parser* p, Expr* left, Expr* right, TokenKind op);


#endif
