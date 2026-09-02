#ifndef UNITY_BUILD
#include "ast_types.h"
#include "expr.h"
#include "token.h"
#include <stdio.h>
#include "arena.h"
#include <stdlib.h>
#include "parser.h"
#endif

Expr* parsePrimary(Parser* p) {
    Token t = tokAdvance(p);
    switch (t.kind) {
        case TOKEN_NUMBER_LITERAL:
            int64_t val = (int64_t)strtoll(p->source + t.start, NULL, 10);
            return makeNumber(p, val);
        case TOKEN_LEFT_PAREN:
            Expr* expr = parseExpr(p, 0);
            if (tokAdvance(p).kind != TOKEN_RIGHT_PAREN)
                fprintf(stderr, "Expected ')'\n");
            return expr;
        case TOKEN_MINUS:
            Expr* operand = parseExpr(p, 3);
            return makeUnary(p, operand, TOKEN_MINUS);
        case TOKEN_LEFT_BRACE:
            Expr* block = makeBlock(p);
            ExprList exprs = {0};
            exprs.items = NULL;
            while (tokPeek(p).kind != TOKEN_RIGHT_BRACE) {
                Expr* expr = parseExpr(p, 0);
                da_append(&exprs, expr);

                tokConsume(p, TOKEN_SEMICOLON, ';');
            }

            if (tokAdvance(p).kind != TOKEN_RIGHT_BRACE)
                fprintf(stderr, "Expected '}'\n");

            block->as.block.expressions = arenaAlloc(&p->astArena, exprs.count * sizeof(Expr*));
            block->as.block.count = exprs.count;
            memcpy(block->as.block.expressions, exprs.items, exprs.count * sizeof(Expr*));                

            da_free(&exprs);
            return block;
        default:
            return makeNumber(p, 0); // don't return null
    }
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

static inline Expr* exprAlloc(Parser* p) {
    return (Expr*)(arenaAlloc(&p->astArena, sizeof(Expr)));
}

Expr* makeNumber(Parser* p, int64_t value) {
    Expr* e = exprAlloc(p);
    e->kind = EXPR_NUMBER;
    e->as.number = value;
    return e;
}

Expr* makeBinary(Parser* p, Expr* left, Expr* right, TokenKind operator) {
    Expr* e = exprAlloc(p);
    e->kind = EXPR_BINARY;
    e->as.binary.left = left;
    e->as.binary.right = right;
    e->as.binary.op = operator;
    return e;
}

Expr* makeUnary(Parser* p, Expr* right, TokenKind operator) {
    Expr* e = exprAlloc(p);
    e->kind = EXPR_UNARY;
    e->as.unary.right = right;
    e->as.unary.op = operator;
    return e;
}

Expr* makeBlock(Parser* p) {
    Expr* e = exprAlloc(p);
    e->kind = EXPR_BLOCK;
    e->as.block.expressions = NULL;
    e->as.block.count = 0;
    return e;
}
