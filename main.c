/*
 * Languages don't need statements.
 * I want saccharin to be completely expression-based.
 * This is what i'm aiming to implement now.
 */
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ast_types.h"
#define FILE_IMPL
#define HASH_IMPL
#define ARENA_IMPL
#include "hashmap.h"
#include "arena.h"
#include "file.h"
#include "macros.h"
#include "token.h"
#include "expr.h"
#include "parser.h"

#include "token.c"
#include "expr.c"
#include "parser.c"

Table keywords;

int main(void) {
	size_t fileSize = 0;
	char* source = readFileBinary("script.scc", &fileSize);
	if (!source) return 1;

	initTable(&keywords);
	tableSet(&keywords, "and",   INT_VAL(TOKEN_AND));
    tableSet(&keywords, "else",  INT_VAL(TOKEN_ELSE));
	tableSet(&keywords, "elseif",INT_VAL(TOKEN_ELSEIF));
    tableSet(&keywords, "false", INT_VAL(TOKEN_FALSE));
    tableSet(&keywords, "for",   INT_VAL(TOKEN_FOR));
    tableSet(&keywords, "fun",   INT_VAL(TOKEN_FUN));
    tableSet(&keywords, "if",    INT_VAL(TOKEN_IF));
    tableSet(&keywords, "or",    INT_VAL(TOKEN_OR));
    tableSet(&keywords, "return",INT_VAL(TOKEN_RETURN));
    tableSet(&keywords, "true",  INT_VAL(TOKEN_TRUE));
    tableSet(&keywords, "while", INT_VAL(TOKEN_WHILE));

	Lexer lex = {
		.current = 0,
		.start = 0,
		.source = source,
		.line = 1,
		.tokens = {0}
	};

	while (!isAtEnd(&lex)) {
		lex.start = lex.current;
		char c = advance(&lex);

		switch(c) {
			case '(': addToken(&lex, TOKEN_LEFT_PAREN); break;
			case ')': addToken(&lex, TOKEN_RIGHT_PAREN); break;
			case '+': addToken(&lex, TOKEN_PLUS); break;
			case '-': addToken(&lex, TOKEN_MINUS); break;
			case '*': addToken(&lex, TOKEN_STAR); break;
			case '/': addToken(&lex, TOKEN_SLASH); break;
			case ';': addToken(&lex, TOKEN_SEMICOLON); break;

			case '!': addToken(&lex, match(&lex, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG); break;
			case '=': addToken(&lex, match(&lex, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL); break;
			case '<': addToken(&lex, match(&lex, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS); break;
			case '>': addToken(&lex, match(&lex, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER); break;

			case ' ':
			case '\r':
			case '\t':
				break;

			case '\n':
				lex.line++;
				break;

			default:
				if (isdigit(c)) {
                    while (isdigit(peek(&lex))) advance(&lex);
                    addToken(&lex, TOKEN_NUMBER_LITERAL);
                } else if (isalpha(c)) {
					while (isalnum(peek(&lex))) advance(&lex);

					size_t length = lex.current - lex.start;
					char buf[length + 1];
					memcpy(buf, source + lex.start, length);
					buf[length] = '\0';

					TokenKind type = TOKEN_IDENTIFIER;
					Value out = NIL_VAL;
					tableGet(&keywords, buf, &out);
					type = out.as.integer;

					addToken(&lex, type);
				} else {
                    fprintf(stderr, "Unexpected character %c at line %zu\n", c, lex.line);
                }
				break;
		}
	}

    lex.start = lex.current;
    addToken(&lex, TOKEN_EOF);

	Parser parser = {
		.astArena = arenaInit(1 MB),
		.current = 0,
		.line = 1,
		.source = lex.source,
		.tokens = lex.tokens
	};

	Expr* ast = NULL;
	while (!parserIsAtEnd(&parser)) {
		ast = parseExpr(&parser, 0);

		if (tokPeek(&parser).kind == TOKEN_SEMICOLON) {
			tokAdvance(&parser);
		}

		printAst(ast);
		printf(" = %lld\n", (long long)eval(ast));
	}

	free(lex.tokens.items);
	free(lex.source);
	freeTable(&keywords);

	return 0;
}
