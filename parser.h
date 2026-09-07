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

struct Parser {
    SymbolList symbolList;
	TokenList tokens;
	Arena astArena;
    Arena strArena;
	char* source;
	size_t current;
	size_t line;
};

int precedenceOf(TokenKind kind);
bool parserIsAtEnd(Parser* p);
Token tokAdvance(Parser* p);
Token tokPeek(Parser* p);
Token tokPeekNext(Parser* p);
bool tokConsume(Parser* p, TokenKind t, const char* s, bool msg);
void printAst(Expr* e);
int64_t eval(Expr* e);

#endif
