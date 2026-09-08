#ifndef PARSER_H
#define PARSER_H

#ifndef UNITY_BUILD
#include "ast_types.h"
#include "token.h"
#include "arena.h"
#include <stdint.h>
#include <stdbool.h>
#include "scope.h"
#endif

#define PREC_UNARY 7

// the env is just a dynamic array
typedef struct {
    int64_t* items;
    size_t count;
    size_t capacity;
} Environment;

typedef struct {
	const char* name;
	size_t length;
	size_t paramCount;
	size_t localCount;
	Expr* body;
} Function;

typedef struct {
	Function* items;
	size_t count;
	size_t capacity;
} FunctionList;

struct Parser {
    SymbolList symbolList;
	ScopeStack scope;
	FunctionList functionList;
	int64_t maxSlot;
    Environment env;
	TokenList tokens;
	Arena astArena;
    Arena strArena;
	char* source;
	size_t current;
	size_t line;
	int64_t functionBase;
	int64_t functionDepth;
	int64_t frameTop;
	int64_t frameBase;
};

int precedenceOf(TokenKind kind);
bool parserIsAtEnd(Parser* p);
Token tokAdvance(Parser* p);
Token tokPeek(Parser* p);
Token tokPeekNext(Parser* p);
bool tokConsume(Parser* p, TokenKind t, const char* s, bool msg);
int64_t eval(Expr* e, Parser* p);
int64_t functionResolve(Parser* p, Token t);

#endif
