#ifndef UNITY_BUILD
    #include "ast_types.h"
    #include "parser.h"
    #include "expr.h"
    #include <stdio.h>
#endif

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

void tokConsume(Parser* p, TokenKind t, char c) {
    if (p->tokens.items[p->current].kind == t) {
        tokAdvance(p);
        return;
    }
    printf("Expected '%c'", c);
}

void printAst(Expr* e) {
	if (!e) return;
	switch(e->kind) {
        case EXPR_NUMBER:
           printf("%lld", (long long)e->as.number);
        break;
        case EXPR_UNARY:
            printf("(-");
            printAst(e->as.unary.right);
            printf(")");
        break;
        case EXPR_BINARY:
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
        break;
        case EXPR_BLOCK:
            printf("{\n");
            for (size_t i = 0; i < e->as.block.count; ++i) {
                printf("    ");
                printAst(e->as.block.expressions[i]);
                printf("\n");
            }
            printf("}\n");
        break;
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

        case EXPR_BLOCK: {
            for (size_t i = 0; i < e->as.block.count; ++i) {
                printf("[expr %zu] = %lld\n", i, (long long)eval(e->as.block.expressions[i]));
            }
            return 0;
        }

		case EXPR_CONDITIONAL: {
			int64_t res = eval(e->as.conditional.condition);
			if (res) {
				eval(e->as.conditional.thenBranch);
			} else {
				if (e->as.conditional.elseBranch) eval(e->as.conditional.elseBranch);
			}
			return 0;
		}
    }
    return 0;
}
