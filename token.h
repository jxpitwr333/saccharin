#ifndef TOKEN_H
#define TOKEN_H

#ifndef UNITY_BUILD
    #include "ast_types.h"
    #include <stddef.h>
    #include <stdbool.h>
#endif

struct Token {
	TokenKind kind;
	size_t start;
	size_t length;
	size_t line;
};

static const char* const tokenLexemes[] = {
#define X(name, lexeme) [name] = lexeme,
	TOKEN_LIST(X)
#undef X
};

static inline const char* tokenLexeme(TokenKind kind) {
	size_t i = (size_t)kind;
	if (i >= sizeof(tokenLexemes) / sizeof(tokenLexemes[0])) return "<?>";
	return tokenLexemes[i];
}

typedef struct {
	Token* items;
	size_t capacity;
	size_t count;
} TokenList;

typedef struct {
	TokenList tokens;
	size_t current;
	size_t start;
	size_t line;
	char* source;
} Lexer;

bool isAtEnd(Lexer* l);
char advance(Lexer* l);
char peek(Lexer* l);
char peekNext(Lexer* l);
bool match(Lexer* l, char expected);
void addToken(Lexer* l, TokenKind kind);

#endif
