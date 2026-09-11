#ifndef UNITY_BUILD
	#include "macros.h"
	#include <stdint.h>
    #include "ast_types.h"
    #include "parser.h"
    #include "expr.h"
    #include <stdio.h>
#endif

int precedenceOf(TokenKind kind) {
	switch(kind) {
		case TOKEN_OR:
			return 1;
		case TOKEN_AND:
			return 2;
		case TOKEN_EQUAL_EQUAL:
		case TOKEN_BANG_EQUAL:
			return 3;
		case TOKEN_LESS:
		case TOKEN_LESS_EQUAL:
		case TOKEN_GREATER:
		case TOKEN_GREATER_EQUAL:
		     return 4;
		case TOKEN_MINUS:
		case TOKEN_PLUS:
		    return 5;
		case TOKEN_STAR:
		case TOKEN_SLASH:
			return 6;
		default:
			return 0;
	}
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

bool tokConsume(Parser* p, TokenKind t, const char* s, bool msg) {
    if (p->tokens.items[p->current].kind == t) {
        tokAdvance(p);
        return true;
    }
    if (msg) fprintf(stderr, "Expected '%s' at line '%zu'\n", s, tokPeek(p).line);
    return false;
}

static inline int64_t varAddr(Parser* p, int64_t sym) {
	Symbol* s = &p->symbolList.items[sym];
	return s->isGlobal ? s->slot : p->frameBase + s->slot;
}

int64_t eval(Expr* e, Parser* p) {
    if (!e) return 0;

    switch (e->kind) {
		case EXPR_BOOL:
            return e->as.boolean;

        case EXPR_NUMBER:
            return e->as.number;

        case EXPR_WHILE: {
            while (eval(e->as.whileExpr.condition, p)) {
                eval(e->as.whileExpr.body, p);
            }
            return 0;
        }

        case EXPR_PRINT: {
            int64_t val = eval(e->as.print.value, p);
            switch (e->as.print.value->type->kind) {
                case TYPE_BOOL:
                    fprintf(stderr, "%s\n", val ? "true" : "false");
                    break;
                case TYPE_I64:
                    fprintf(stderr, "%lld\n", (long long)val);
                    break;
                default:
                    fprintf(stderr, "print error: Unhandled type.\n");
            }
            return val;
        }

		case EXPR_UNARY: {
            int64_t val = eval(e->as.unary.right, p);

            switch (e->as.unary.op) {
                case TOKEN_MINUS: return -val;
				case TOKEN_BANG: return !val;
                default: return 0;
            }
        }

        case EXPR_BINARY: {
            int64_t left  = eval(e->as.binary.left, p);
            int64_t right = eval(e->as.binary.right, p);

            switch (e->as.binary.op) {
                case TOKEN_PLUS:  return left + right;
                case TOKEN_MINUS: return left - right;
                case TOKEN_STAR:  return left * right;
                case TOKEN_SLASH: return right != 0 ? left / right : 0;
                case TOKEN_EQUAL_EQUAL:  return left == right;
                case TOKEN_BANG_EQUAL: return left != right;
                case TOKEN_GREATER_EQUAL:  return left >= right;
                case TOKEN_LESS_EQUAL: return left <= right;
                case TOKEN_GREATER:  return left > right;
                case TOKEN_LESS: return left < right;
                default: return 0;
            }
        }

		case EXPR_LOGICAL: {
			int64_t left = eval(e->as.binary.left, p);

			switch (e->as.binary.op) {
				case TOKEN_AND: return left ? (eval(e->as.binary.right, p) != 0) : 0;
				case TOKEN_OR: return left ? 1 : (eval(e->as.binary.right, p) != 0);
				default: return 0;
			}
		}

        case EXPR_BLOCK: {
			int64_t lastRes = 0;
            for (size_t i = 0; i < e->as.block.count; ++i) {
				lastRes = (long long)eval(e->as.block.expressions[i], p);
			}
            return lastRes;
        }

		case EXPR_CONDITIONAL: {
			int64_t res = eval(e->as.conditional.condition, p);
			int64_t lastRes = 0;
			if (res) {
				lastRes = eval(e->as.conditional.thenBranch, p);
			} else {
				if (e->as.conditional.elseBranch) {
					lastRes = eval(e->as.conditional.elseBranch, p);
				}
			}
			return lastRes;
		}

        case EXPR_VAR_DECL: {
            int64_t val = eval(e->as.varDecl.value, p);
            da_at(&p->env, val, varAddr(p, e->as.varDecl.sym));
            return val;
        }

        case EXPR_VAR_READ:
            return p->env.items[varAddr(p, e->as.varRead.sym)];

		case EXPR_VAR_ASSIGN: {
			int64_t val = eval(e->as.varAssign.newValue, p);
            da_at(&p->env, val, varAddr(p, e->as.varAssign.sym));
            return val;
		}

		case EXPR_FUN: return 0;

		case EXPR_CALL: {
			Function* fn = &p->functionList.items[e->as.call.index];
			int64_t newBase = p->frameTop;
			for (size_t i = 0; i < e->as.call.count; ++i) {
				int64_t v = eval(e->as.call.args[i], p);
				da_at(&p->env, v, newBase + i);
			}

			int64_t savedBase = p->frameBase;
			int64_t savedTop = p->frameTop;
			p->frameBase = newBase;
			p->frameTop = newBase + fn->localCount;

			int64_t result = eval(fn->body, p);
			p->frameBase = savedBase;
			p->frameTop = savedTop;
			return result;
		}
    }
    return 0;
}

int64_t functionResolve(Parser* p, Token t) {
    for (int64_t i = p->functionList.count - 1; i >= 0; --i) {
        if (p->functionList.items[i].length == t.length &&
			strncmp(p->functionList.items[i].name, p->source + t.start, t.length) == 0) {
            return i;
        }
    }
    return -1;
}
