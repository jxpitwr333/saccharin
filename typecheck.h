#ifndef TYPECHECK_H
#define TYPECHECK_H

#ifndef UNITY_BUILD
    #include "ast_types.h"
    #include "expr.h"
    #include "scope.h"
    #include "parser.h"
    #include <stdio.h>
#endif

static inline Type* typecheck(Expr* e, Parser* p) {
    switch (e->kind) {
        case EXPR_NUMBER:
            e->type = &type_i64_inst;
            return e->type;

        case EXPR_VAR_READ: {
            Symbol* s = &p->symbolList.items[e->as.varRead.sym];
            e->type = s->type;
            return e->type;
        }

        case EXPR_BINARY: {
            Type* left_t = typecheck(e->as.binary.left, p);
            Type* right_t = typecheck(e->as.binary.right, p);

            if (left_t->kind == TYPE_ERR || right_t->kind == TYPE_ERR) {
                e->type = &type_err_inst;
                return &type_err_inst;
            }

            switch (e->as.binary.op) {
                case TOKEN_PLUS:
                case TOKEN_MINUS:
                case TOKEN_STAR:
                case TOKEN_SLASH: {
                    if (left_t->kind != TYPE_I64 || right_t->kind != TYPE_I64) {
                        // not a very useful error
                        fprintf(stderr, "TypeError: arithmetic operators require i64 operands.\n");
                        e->type = &type_err_inst;
                        return &type_err_inst;
                    }
                    e->type = &type_i64_inst;
                    return &type_i64_inst;
                }

                case TOKEN_EQUAL_EQUAL:
                case TOKEN_BANG_EQUAL:
                case TOKEN_GREATER:
                case TOKEN_GREATER_EQUAL:
                case TOKEN_LESS:
                case TOKEN_LESS_EQUAL: {
                    if (left_t->kind != right_t->kind) {
                        fprintf(stderr, "TypeError: cannot compare mismatched types.\n");
                        e->type = &type_err_inst;
                        return &type_err_inst;
                    }
                    e->type = &type_bool_inst;
                    return &type_bool_inst;
                }

                default:
                    e->type = &type_err_inst;
                    return &type_err_inst;
            }
        }

        case EXPR_CONDITIONAL: {
            Type* cond_t = typecheck(e->as.conditional.condition, p);
            if (cond_t->kind != TYPE_BOOL) {
                fprintf(stderr, "TypeError: condition in if statement must evaluate to bool.\n");
            }

            Type* then_t = typecheck(e->as.conditional.thenBranch, p);
            if (e->as.conditional.elseBranch) {
                Type* else_t = typecheck(e->as.conditional.elseBranch, p);
                if (then_t->kind != else_t->kind) {
                    fprintf(stderr, "TypeError: both branches of a conditional expression should have matching types.\n");
                    e->type = &type_err_inst;
                    return &type_err_inst;
                }
            }

            e->type = then_t;
            return then_t;
        }

        case EXPR_VAR_DECL: {
            Type* init_t = typecheck(e->as.varDecl.value, p);
            Symbol* s = &p->symbolList.items[e->as.varDecl.sym];

            if (s->type != init_t) {
                fprintf(stderr, "TypeError: initializer type does not match explicit declaration.\n");
                e->type = &type_err_inst;
                return &type_err_inst;
            }

            e->type = s->type;
            return e->type;
        }

        case EXPR_BLOCK: {
            Type* last_type = TYPE_I64;
            for (size_t i = 0; i < e->as.block.count; ++i) {
                last_type = typecheck(e->as.block.expressions[i], p);
            }
            e->type = last_type;
            return e->type;
        }

        case EXPR_FUN: {
            Function* fn = &p->functionList.items[e->as.function.index];
            Type* body_t = typecheck(e->as.function.body, p);

            if (body_t->kind != fn->retType->kind) {
                fprintf(stderr, "TypeError: function type mismatch.\n");
                return &type_err_inst;
            }

            e->type = fn->retType;
            return e->type;
        }

        case EXPR_CALL: {
            Function* fn = &p->functionList.items[e->as.call.index];

            if (e->as.call.count != fn->paramCount) {
                fprintf(stderr, "TypeError: expected %zu arguments, got %zu.\n", fn->paramCount, e->as.call.count);
                return &type_err_inst;
            }

            for (size_t i = 0; i < e->as.call.count; ++i) {
                Type* arg_t = typecheck(e->as.call.args[i], p);
                if (arg_t->kind != fn->paramTypes[i]->kind) {
                    fprintf(stderr, "TypeError: argument %zu type mismatch.\n", i);
                    return &type_err_inst;
                }
            }

            e->type = fn->retType;
            return e->type;
        }

        case EXPR_UNARY: {
            Type* op_t = typecheck(e->as.unary.right, p);
            if (op_t->kind == TYPE_ERR) {
                e->type = &type_err_inst;
                return &type_err_inst;
            }

            if (e->as.unary.op == TOKEN_MINUS) {
                if (op_t->kind != TYPE_I64) {
                    fprintf(stderr, "TypeError: unary minus requires i64.\n");
                    e->type = &type_err_inst;
                    return &type_err_inst;
                }
                e->type = &type_i64_inst;
            } else if (e->as.unary.op == TOKEN_BANG) {
                if (op_t->kind != TYPE_BOOL) {
                    fprintf(stderr, "TypeError: unary NOT requires bool.\n");
                    e->type = &type_err_inst;
                    return &type_err_inst;
                }
                e->type = &type_bool_inst;
            }
            return e->type;
        }

        case EXPR_VAR_ASSIGN: {
            Symbol* s = &p->symbolList.items[e->as.varAssign.sym];
            Type* val_t = typecheck(e->as.varAssign.newValue, p);

            if (s->type->kind != val_t->kind) {
                fprintf(stderr, "TypeError: variable type mismatch on assignment.\n");
                e->type = &type_err_inst;
                return &type_err_inst;
            }

            e->type = s->type;
            return e->type;
        }

        case EXPR_LOGICAL: {
            Type* left_t = typecheck(e->as.binary.left, p);
            Type* right_t = typecheck(e->as.binary.right, p);

            if (left_t->kind != TYPE_BOOL || right_t->kind != TYPE_BOOL) {
                fprintf(stderr, "TypeError: logical operators require boolean operands.\n");
                e->type = &type_err_inst;
                return &type_err_inst;
            }

            e->type = &type_bool_inst;
            return &type_bool_inst;
        }

        default:
            e->type = &type_err_inst;
            return &type_err_inst;
    }
}

static inline Type* getTypeFromToken(Token t, Parser* p) {
	switch (t.kind) {
		case TOKEN_I64:
			return &type_i64_inst;
		case TOKEN_BOOL:
			return &type_bool_inst;
		default:
			fprintf(stderr, "Expected a valid type, got '%.*s' at line %zu\n", (int)t.length, p->source + t.start, t.line);
			return &type_err_inst;
	}
}

#endif // TYPECHECK_H
