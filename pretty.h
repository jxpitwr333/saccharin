#ifndef PRETTY_H
#define PRETTY_H

#ifndef UNITY_BUILD
	#include "parser.h"
    #include "ast_types.h"
    #include "expr.h"
    #include "token.h"
    #include <stdio.h>
#endif

typedef struct {
    Expr* const* items;
    size_t count;
} Kids;

static inline Kids exprKids(Expr* e, Expr* buf[3]) {
    switch (e->kind) {
        case EXPR_NUMBER:
        case EXPR_VAR_READ:
        case EXPR_FUN:
            return (Kids){NULL, 0};
        case EXPR_UNARY:
            buf[0] = e->as.unary.right;
            return (Kids){buf, 1};
        case EXPR_VAR_DECL:
            buf[0] = e->as.varDecl.value;
            return (Kids){buf, 1};
        case EXPR_BINARY:
            buf[0] = e->as.binary.left;
            buf[1] = e->as.binary.right;
            return (Kids){buf, 2};
        case EXPR_CONDITIONAL:
            buf[0] = e->as.conditional.condition;
            buf[1] = e->as.conditional.thenBranch;
            buf[2] = e->as.conditional.elseBranch;
            return (Kids){buf, e->as.conditional.elseBranch ? 3 : 2};
        case EXPR_BLOCK:
            return (Kids){e->as.block.expressions, e->as.block.count};
    }
    return (Kids){NULL, 0};
}

static inline const char* slotName(Parser* p, int64_t slot) {
    if (slot < 0 || slot >= p->symbolList.count) return "<?>";
    return p->symbolList.items[slot].name;
}

static inline const char* exprLabel(Expr* e, Parser* p, char* buf, size_t n) {
    switch (e->kind) {
        case EXPR_NUMBER:
            snprintf(buf, n, "%lld", (long long)e->as.number);
            return buf;
        case EXPR_VAR_DECL:
            snprintf(buf, n, "let %s", slotName(p, e->as.varDecl.slot));
            return buf;
        case EXPR_VAR_READ:  return slotName(p, e->as.varRead.slot);
        case EXPR_UNARY:     return tokenLexeme(e->as.unary.op);
        case EXPR_BINARY:    return tokenLexeme(e->as.binary.op);
        case EXPR_BLOCK:     return "block";
        case EXPR_CONDITIONAL: return "if";
        case EXPR_FUN:       return "fun";
    }
    return "<?>";
}

static inline void printAstAt(Expr* e, Parser* p, int depth) {
    if (!e) {
        printf("<null>");
        return;
    }

    char label[64];
    Expr* buf[3];
    Kids kids = exprKids(e, buf);
    const char* name = exprLabel(e, p, label, sizeof label);

    if (kids.count == 0) {
        printf("%s", name);
        return;
    }

    bool wide = kids.count > 2;

    printf("(%s", name);
    for (size_t i = 0; i < kids.count; ++i) {
        if (wide) {
            printf("\n");
            for (int j = 0; j <= depth; ++j) printf("    ");
        } else {
            printf(" ");
        }
        printAstAt(kids.items[i], p, depth + 1);
    }
    printf(")");
}

static inline void printAst(Expr* e, Parser* p) {
    printAstAt(e, p, 0);
}

#endif //PRETTY_H
