#ifndef TYPECHECK_H
#define TYPECHECK_H

#ifndef UNITY_BUILD
    #include "ast_types.h"
    #include "expr.h"
    #include "scope.h"
    #include "parser.h"
    #include <stdio.h>
	#include "types.h"
#endif

static inline Type* typecheck(Expr* e, Parser* p) {
    switch (e->kind) {
        case EXPR_NUMBER: {
            e->type = &type_i64_inst;
            return e->type;
        }

        case EXPR_WHILE: {
            Type* cond_t = typecheck(e->as.whileExpr.condition, p);
            if (cond_t->kind != TYPE_BOOL) {
                fprintf(stderr, "TypeError: while loop condition must evaluate to bool.\n");
                e->type = &type_err_inst;
                return &type_err_inst;
            }

            Type* body_t = typecheck(e->as.whileExpr.body, p);
            if (body_t->kind == TYPE_ERR) {
                e->type = &type_err_inst;
                return &type_err_inst;
            }
            
            e->type = &type_i64_inst;
            return e->type;
        }
        
        case EXPR_RETURN: {
            if (p->currentFunction < 0) {
                fprintf(stderr, "TypeError: return outside of a function.\n");
                e->type = &type_err_inst;
                return e->type;
            }

            Function* fn = &p->functionList.items[p->currentFunction];
            Type* val_t = typecheck(e->as.ret.value, p);
            if (val_t->kind == TYPE_ERR) {
                e->type = &type_err_inst;
                return e->type;
            }

            if (!typeEquals(val_t, fn->retType)) {
                fprintf(stderr, "TypeError: return value does not match function type.\n");
                e->type = &type_err_inst;
                return e->type;
            }

            e->type = fn->retType;
            return e->type;
        }

        case EXPR_PRINT: {
            Type* val_t = typecheck(e->as.print.value, p);
            if (val_t->kind == TYPE_ERR) {
                e->type = &type_err_inst;
                return &type_err_inst;
            }

            e->type = val_t;
            return e->type;
        }

		case EXPR_BOOL: {
            e->type = &type_bool_inst;
            return e->type;
        }

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
                    if (!typeEquals(left_t, right_t)) {
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
				e->type = &type_err_inst;
				return &type_err_inst;
            }

            Type* then_t = typecheck(e->as.conditional.thenBranch, p);
            if (e->as.conditional.elseBranch) {
                Type* else_t = typecheck(e->as.conditional.elseBranch, p);
                if (!typeEquals(then_t, else_t)) {
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

            if (!typeEquals(s->type, init_t)) {
                fprintf(stderr, "TypeError: initializer type does not match explicit declaration.\n");
                e->type = &type_err_inst;
                return &type_err_inst;
            }

            e->type = s->type;
            return e->type;
        }

        case EXPR_BLOCK: {
            Type* last_type = &type_i64_inst;
            for (size_t i = 0; i < e->as.block.count; ++i) {
                last_type = typecheck(e->as.block.expressions[i], p);
            }
            e->type = last_type;
            return e->type;
        }

        case EXPR_FUN: {
            Function* fn = &p->functionList.items[e->as.function.index];
            int64_t saved = p->currentFunction;
            p->currentFunction = e->as.function.index;
            Type* body_t = typecheck(e->as.function.body, p);
            p->currentFunction = saved;

            if (!typeEquals(body_t, fn->retType)) {
                fprintf(stderr, "TypeError: function type mismatch.\n");
				e->type = &type_err_inst;
                return &type_err_inst;
            }

            e->type = fn->retType;
            return e->type;
        }

        case EXPR_CALL: {
            Function* fn = &p->functionList.items[e->as.call.index];

            if (e->as.call.count != fn->paramCount) {
                fprintf(stderr, "TypeError: expected %zu arguments, got %zu.\n", fn->paramCount, e->as.call.count);
				e->type = &type_err_inst;
                return &type_err_inst;
            }

            for (size_t i = 0; i < e->as.call.count; ++i) {
                Type* arg_t = typecheck(e->as.call.args[i], p);
                if (!typeEquals(arg_t, fn->paramTypes[i])) {
                    fprintf(stderr, "TypeError: argument %zu type mismatch.\n", i);
					e->type = &type_err_inst;
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

            if (!typeEquals(s->type, val_t)) {
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

		case EXPR_ADDR_OF: {
			Symbol* s = &p->symbolList.items[e->as.addrOf.sym];
			Type* t = typeAlloc(p);
			t->kind = TYPE_PTR;
			t->to = s->type;
			e->type = t;
			return e->type;
		}

		case EXPR_DEREF: {
			Type* ptr_t = typecheck(e->as.deref.ptr, p);

			if (ptr_t->kind == TYPE_ERR)  {
				e->type = &type_err_inst;
				return e->type;
			}

			if (ptr_t->kind != TYPE_PTR)  {
				fprintf(stderr, "TypeError: cannot dereference a non-pointer.\n");
				e->type = &type_err_inst;
				return e->type;
			}

			e->type = ptr_t->to;
			return e->type;
		}

		case EXPR_STORE: {
			Type* ptr_t = typecheck(e->as.store.ptr, p);
			Type* val_t = typecheck(e->as.store.value, p);
			if (ptr_t->kind == TYPE_ERR || val_t->kind == TYPE_ERR) {
				e->type = &type_err_inst;
				return e->type;
			}
			if (ptr_t->kind != TYPE_PTR) {
				fprintf(stderr, "TypeError: cannot store through a non-pointer.\n");
				e->type = &type_err_inst;
				return e->type;
			}
			if (!typeEquals(ptr_t->to, val_t)) {
				fprintf(stderr, "TypeError: stored value does not match pointee type.\n");
				e->type = &type_err_inst;
				return e->type;
			}
			e->type = val_t;
			return e->type;
		}
    }
	// default, but lets -Wswitch notify me instead of failing silently
	e->type = &type_err_inst;
	return &type_err_inst;
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

static inline Type* parseType(Parser* p) {
	if (tokConsume(p, TOKEN_STAR, "*", false)) {
		Type* t = typeAlloc(p);
		t->kind = TYPE_PTR;
		t->to = parseType(p);
		return t;
	}
	return getTypeFromToken(tokAdvance(p), p);
}

#endif // TYPECHECK_H
