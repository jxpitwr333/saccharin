#ifndef SCOPE_H
#define SCOPE_H

#include "parser.h"
#ifndef UNITY_BUILD
    #include "ast_types.h"
    #include "macros.h"
    #include <string.h>
#endif

static inline int64_t scopeDecl(Parser* p, Token t) {
    int64_t slot = p->symbolList.count;
    
    char* buf = arenaAlloc(&p->strArena, t.length + 1);
    memcpy(buf, p->source + t.start, t.length);
    buf[t.length] = '\0';

    Symbol s = (Symbol){ buf, t.length, slot };
    da_append(&p->symbolList, s);
    return slot;
}

static inline int64_t scopeResolve(SymbolList* symbolList, const char* name, size_t length) {
    for (int i = symbolList->count - 1; i >= 0; --i) {
        if (symbolList->items[i].length == length && strncmp(symbolList->items[i].name, name, length) == 0) {
            return symbolList->items[i].slot;
        }
    }
    return -1;
}

#endif //SCOPE_H
