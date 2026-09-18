#ifndef TYPES_H
#define TYPES_H

#ifndef UNITY_BUILD
    #include "ast_types.h"
    #include "arena.h"
#endif

typedef enum {
	TYPE_I64,
	TYPE_BOOL,
    TYPE_PTR,
    TYPE_ERR,
} TypeKind;

struct Type {
    TypeKind kind;
    Type* to;
};

typedef struct {
    Type** items;
    size_t count;
    size_t capacity;
} TypeList;

static Type type_bool_inst = { .kind = TYPE_BOOL };
static Type type_i64_inst = { .kind = TYPE_I64 };
static Type type_err_inst = { .kind = TYPE_ERR };

static inline Type* typeAlloc(Parser* p) {
    Type* t = arenaAlloc(&p->astArena, sizeof(Type));
    t->to = NULL;
    return t;
}

static inline bool typeEquals(Type* a, Type* b) {
    if (a->kind != b->kind) {
        return false;
    }
    if (a->kind == TYPE_PTR) return typeEquals(a->to, b->to);
    return true;
}

#endif // TYPES_H
