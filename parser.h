#ifndef PARSER_H
#define PARSER_H

#ifndef UNITY_BUILD
#include "ast_types.h"
#include "token.h"
#include "arena.h"
#include <stdint.h>
#include <stdbool.h>
#endif

struct Parser {
	TokenList tokens;
	Arena astArena;
	char* source;
	size_t current;
	size_t line;
};

int precedenceOf(TokenKind kind);
bool parserIsAtEnd(Parser* p);
Token tokAdvance(Parser* p);
Token tokPeek(Parser* p);
Token tokPeekNext(Parser* p);
void printAst(Expr* e);
int64_t eval(Expr* e);

#endif
