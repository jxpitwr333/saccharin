#ifndef SCOPE_H
#define SCOPE_H

#ifndef UNITY_BUILD
	#include "parser.h"
    #include "ast_types.h"
    #include "macros.h"
    #include <string.h>
#endif

static inline bool symbolMatches(Parser* p, int64_t sym, Token t) {
    return p->symbolList.items[sym].length == t.length &&
		strncmp(p->symbolList.items[sym].name, p->source + t.start, t.length) == 0;
}

static inline int64_t scopeDecl(Parser* p, Token t, Type* type) {
    int64_t slot = p->scope.count - p->functionBase;
    if (slot + 1 > p->maxSlot) p->maxSlot = slot + 1;

    char* buf = arenaAlloc(&p->strArena, t.length + 1);
    memcpy(buf, p->source + t.start, t.length);
    buf[t.length] = '\0';

    int64_t sym = p->symbolList.count;
    Symbol s = (Symbol){ buf, t.length, slot, p->functionDepth == 0, type };
    da_append(&p->symbolList, s);
    da_append(&p->scope, sym);
    return sym;
}

static inline int64_t scopeResolve(Parser* p, Token t) {
    for (int64_t i = p->scope.count - 1; i >= p->functionBase; --i) {
        if (symbolMatches(p, p->scope.items[i], t)) return p->scope.items[i];
    }

    for (int64_t i = p->functionBase - 1; i >= 0; --i) {
        int64_t sym = p->scope.items[i];
        if (p->symbolList.items[sym].isGlobal && symbolMatches(p, sym, t)) return sym;
    }

    return -1;
}

static inline int64_t scopeBegin(Parser* p) {
    return p->scope.count;
}

static inline void scopeEnd(Parser* p, int64_t mark) {
    p->scope.count = mark;
}

#endif //SCOPE_H
