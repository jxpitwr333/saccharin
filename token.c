#ifndef UNITY_BUILD
#include "token.h"
#include "macros.h"
#endif

bool isAtEnd(Lexer* l) {
	return l->source[l->current] == '\0';
}

char advance(Lexer* l) {
	return l->source[l->current++];
}

char peek(Lexer* l) {
	return l->source[l->current];
}

char peekNext(Lexer* l) {
	return l->source[l->current + 1];
}

bool match(Lexer* l, char expected) {
	if (isAtEnd(l)) return false;
	if (l->source[l->current] != expected) return false;
	l->current++;
	return true;
}

void addToken(Lexer* l, TokenKind kind) {
	Token t = {
		.kind = kind,
		.start = l->start,
		.length = l->current - l->start,
		.line = l->line
	};
	da_append(&l->tokens, t);
}
