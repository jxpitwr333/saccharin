#ifndef UNITY_BUILD
	#include "macros.h"
	#include <stdint.h>
	#include "scope.h"
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
        case TOKEN_NUMBER_LITERAL: {
            int64_t val = (int64_t)strtoll(p->source + t.start, NULL, 10);
            return makeNumber(p, val);
		}
        case TOKEN_LEFT_PAREN: {
            Expr* expr = parseExpr(p, 0);
            if (tokAdvance(p).kind != TOKEN_RIGHT_PAREN)
                fprintf(stderr, "Expected ')'\n");
            return expr;
		}
        case TOKEN_MINUS: {
            Expr* operand = parseExpr(p, PREC_UNARY);
            return makeUnary(p, operand, TOKEN_MINUS);
		}
		case TOKEN_BANG: {
            Expr* operand = parseExpr(p, PREC_UNARY);
            return makeUnary(p, operand, TOKEN_BANG);
		}
        case TOKEN_LEFT_BRACE: {
			int64_t mark = scopeBegin(p);
            Expr* block = makeBlock(p);

            ExprList exprs = {0};
            exprs.items = NULL;
            while (tokPeek(p).kind != TOKEN_RIGHT_BRACE && tokPeek(p).kind != TOKEN_EOF) {
                Expr* expr = parseExpr(p, 0);
                da_append(&exprs, expr);

                tokConsume(p, TOKEN_SEMICOLON, ";", true);
            }

            if (tokAdvance(p).kind != TOKEN_RIGHT_BRACE)
                fprintf(stderr, "Expected '}'\n");

            block->as.block.expressions = arenaAlloc(&p->astArena, exprs.count * sizeof(Expr*));
            block->as.block.count = exprs.count;
            memcpy(block->as.block.expressions, exprs.items, exprs.count * sizeof(Expr*));

            da_free(&exprs);
			scopeEnd(p, mark);
            return block;
		}
        case TOKEN_IF: {
            Expr* condition = parseExpr(p, 0);
            Expr* thenBranch = parseExpr(p, 0);

            Expr* elseBranch = NULL;
            if (tokConsume(p, TOKEN_ELSE, "else", true)) {
                elseBranch = parseExpr(p, 0);
            }

            return makeConditional(p, condition, thenBranch, elseBranch);
		}
        case TOKEN_LET: {
			//let name : type = value
            Token name = tokPeek(p);
            if (!tokConsume(p, TOKEN_IDENTIFIER, "identifier", true)) return makeNumber(p, 0);

			if (!tokConsume(p, TOKEN_COLON, ":", true)) return makeNumber(p, 0);

			Token typeTok = tokAdvance(p);
			Type* type = NULL;
			switch (typeTok.kind) {
				case TOKEN_I64:
					type = &type_i64_inst;
					break;
				case TOKEN_BOOL:
					type = &type_bool_inst;
					break;
				default:
					fprintf(stderr, "Expected a valid type, got '%.*s' at line %zu\n", (int)t.length, p->source + t.start, t.line);
					return makeNumber(p, 0);
			}
			
            tokConsume(p, TOKEN_EQUAL, "=", true);
            Expr* initializer = parseExpr(p, 0);

            return makeDecl(p, scopeDecl(p, name, type), initializer);
        }
        case TOKEN_IDENTIFIER: {
			// this is a function
			if (tokPeek(p).kind == TOKEN_LEFT_PAREN) {
				int64_t index = functionResolve(p, t);
				if (index < 0) {
					fprintf(stderr, "Undefined function '%.*s' at line %zu\n", (int)t.length, p->source + t.start, t.line);
					return makeNumber(p, 0);
				}

				tokAdvance(p);
				ExprList args = {0};
				while (tokPeek(p).kind != TOKEN_RIGHT_PAREN && tokPeek(p).kind != TOKEN_EOF) {
					da_append(&args, parseExpr(p, 0));
					if (!tokConsume(p, TOKEN_COMMA, ",", false)) break;
				}
				tokConsume(p, TOKEN_RIGHT_PAREN, ")", true);

				if (args.count != p->functionList.items[index].paramCount) {
					fprintf(stderr, "'%.*s' expects %zu arguments, got %zu\n", (int)t.length, p->source + t.start, p->functionList.items[index].paramCount, args.count);
					return makeNumber(p, 0);
				}

				Expr* call = makeCall(p, index, args.items, args.count);
				da_free(&args);
				return call;
			}

			// this is a variable
            int64_t sym = scopeResolve(p, t);

			if (sym < 0) {
				fprintf(stderr, "Undeclared identifier '%.*s' at line '%zu'\n", (int)t.length, p->source + t.start, t.line);
				return makeNumber(p, 0);
			}

			if (tokConsume(p, TOKEN_EQUAL, "=", false)) {
				Expr* newVal = parseExpr(p, 0);
				return makeAssign(p, sym, newVal);
			} else {
				return makeRead(p, sym);
			}
        }
		case TOKEN_FUN: {
			// syntax:
			// fun name(param1, param2, ...) {
			//     body;
			// }
			Token name = tokPeek(p);
			if (!tokConsume(p, TOKEN_IDENTIFIER, "identifier", true)) return makeNumber(p, 0);

			char* fname = arenaAlloc(&p->strArena, name.length + 1);
			memcpy(fname, p->source + name.start, name.length);
			fname[name.length] = '\0';

			int64_t index = p->functionList.count;
			Function fn = { fname, name.length, 0, 0, NULL };
			da_append(&p->functionList, fn);

			tokConsume(p, TOKEN_LEFT_PAREN, "(", true);
			
			int64_t saved = p->functionBase;
			int64_t savedMax = p->maxSlot;
			int64_t mark = scopeBegin(p);
			p->functionBase = p->scope.count;
			p->maxSlot = 0;
			p->functionDepth++;

			size_t paramCount = 0;
			while (tokPeek(p).kind != TOKEN_RIGHT_PAREN && tokPeek(p).kind != TOKEN_EOF) {
				Token param = tokPeek(p);
				if (!tokConsume(p, TOKEN_IDENTIFIER, "identifier", true)) break;

				scopeDecl(p, param);
				paramCount++;
				if (!tokConsume(p, TOKEN_COMMA, ",", false)) break;
			}
			tokConsume(p, TOKEN_RIGHT_PAREN, ")", true);

			p->functionList.items[index].paramCount = paramCount;

			Expr* body = parseExpr(p, 0);
			p->functionList.items[index].localCount = p->maxSlot;
			p->functionList.items[index].body = body;

			scopeEnd(p, mark);
			p->functionBase = saved;
			p->maxSlot = savedMax;
			p->functionDepth--;

			return makeFunction(p, index, body);
		}
        default:
            return makeNumber(p, 0); // don't return null
    }
}

Expr* parseExpr(Parser* p, int minPrec) {
    Expr* left = parsePrimary(p);

    while (minPrec < precedenceOf(tokPeek(p).kind)) {
        Token op = tokAdvance(p);
        Expr* right = parseExpr(p, precedenceOf(op.kind));
        left = makeInfix(p, left, right, op.kind);
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

Expr* makeConditional(Parser* p, Expr* condition, Expr* thenBranch, Expr* elseBranch) {
    Expr* e = exprAlloc(p);
    e->kind = EXPR_CONDITIONAL;
    e->as.conditional.condition = condition;
    e->as.conditional.thenBranch = thenBranch;
    e->as.conditional.elseBranch = elseBranch;
    return e;
}

Expr* makeDecl(Parser* p, int64_t sym, Expr* initializer) {
    Expr* e = exprAlloc(p);
    e->kind = EXPR_VAR_DECL;
    e->as.varDecl.sym = sym;
    e->as.varDecl.value = initializer;
    return e;
}

Expr* makeRead(Parser* p, int64_t sym) {
    Expr* e = exprAlloc(p);
    e->kind = EXPR_VAR_READ;
    e->as.varRead.sym = sym;
    return e;
}

Expr* makeAssign(Parser* p, int64_t sym, Expr* newValue) {
	Expr* e = exprAlloc(p);
    e->kind = EXPR_VAR_ASSIGN;
    e->as.varAssign.sym = sym;
	e->as.varAssign.newValue = newValue;
    return e;
}

Expr* makeFunction(Parser* p, int64_t index, Expr* body) {
	Expr* e = exprAlloc(p);
	e->kind = EXPR_FUN;
	e->as.function.body = body;
	e->as.function.index = index;
	return e;
}

Expr* makeCall(Parser* p, int64_t index, Expr** args, size_t argCount) {
	Expr* e = exprAlloc(p);
	e->kind = EXPR_CALL;
	e->as.call.index = index;
	e->as.call.count = argCount;
	e->as.call.args = arenaAlloc(&p->astArena, argCount * sizeof(Expr*));
	memcpy(e->as.call.args, args, argCount * sizeof(Expr*));
	return e;
}

Expr* makeLogical(Parser* p, Expr* left, Expr* right, TokenKind op) {
    Expr* e = exprAlloc(p);
    e->kind = EXPR_LOGICAL;
    e->as.binary.left = left;
    e->as.binary.right = right;
    e->as.binary.op = op;
    return e;
}

Expr* makeInfix(Parser* p, Expr* left, Expr* right, TokenKind op) {
    switch (op) {
        case TOKEN_AND:
        case TOKEN_OR:
            return makeLogical(p, left, right, op);
        default:
            return makeBinary(p, left, right, op);
    }
}
