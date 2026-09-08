#ifndef SCOPE_H
#define SCOPE_H

#ifndef UNITY_BUILD
	#include "parser.h"
    #include "ast_types.h"
    #include "macros.h"
    #include <string.h>
#endif

static inline int64_t scopeDecl(Parser* p, Token t) {
    int64_t slot = p->symbolList.count - p->functionBase;
    if (slot + 1 > p->maxSlot) p->maxSlot = slot + 1;

    char* buf = arenaAlloc(&p->strArena, t.length + 1);
    memcpy(buf, p->source + t.start, t.length);
    buf[t.length] = '\0';

    Symbol s = (Symbol){ buf, t.length, slot };
    da_append(&p->symbolList, s);
    return slot;
}

static inline int64_t scopeResolve(Parser* p, Token t) {
    for (int64_t i = p->symbolList.count - 1; i >= 0; --i) {
        if (p->symbolList.items[i].length == t.length &&
			strncmp(p->symbolList.items[i].name, p->source + t.start, t.length) == 0) {
            return p->symbolList.items[i].slot;
        }
    }
    return -1;
}

static inline int64_t scopeBegin(Parser* p) {
    return p->symbolList.count;
}

static inline void scopeEnd(Parser* p, int64_t mark) {
    p->symbolList.count = mark;
}

#endif //SCOPE_H
