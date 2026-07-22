#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    char* data;
    size_t capacity;
    size_t offset;
} Arena;

void initArena(Arena* arena, size_t capacity) {
    arena->data = malloc(capacity);
    arena->capacity = capacity;
    arena->offset = 0;
}

void freeArena(Arena* arena) {
    free(arena->data);
    arena->data = NULL;
    arena->capacity = 0;
    arena->offset = 0;
}

void arenaReset(Arena* arena) {
    arena->offset = 0;
}

char* arena_sprintf(Arena* arena, const char* fmt, ...) {
    va_list args, args_copy;
    va_start(args, fmt);
    va_copy(args_copy, args);

    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (needed < 0 || arena->offset + needed + 1 > arena->capacity) {
        va_end(args_copy);
        printf("out of memory!\n");
        return NULL; 
    }

    char* str = &arena->data[arena->offset];
    vsnprintf(str, needed + 1, fmt, args_copy);
    va_end(args_copy);

    arena->offset += needed + 1;
    return str;
}

char* arena_substring(Arena* arena, const char* str, size_t start, size_t length) {
    if (!str || !arena) return NULL;

    size_t str_len = strlen(str);
    if (start >= str_len) {
        return arena_sprintf(arena, ""); 
    }

    if (start + length > str_len) {
        length = str_len - start;
    }

    if (arena->offset + length + 1 > arena->capacity) {
        return NULL;
    }

    char* sub = &arena->data[arena->offset];
    memcpy(sub, str + start, length);
    sub[length] = '\0';

    arena->offset += length + 1;
    return sub;
}

typedef enum {
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SEMICOLON,
    TOKEN_SLASH,
    TOKEN_STAR,
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_AND,
    TOKEN_CLASS,
    TOKEN_ELSE,
    TOKEN_FALSE,
    TOKEN_FUN,
    TOKEN_FOR,
    TOKEN_IF,
    TOKEN_NIL,
    TOKEN_OR,
    TOKEN_PRINT,
    TOKEN_RETURN,
    TOKEN_SUPER,
    TOKEN_THIS,
    TOKEN_TRUE,
    TOKEN_VAR,
    TOKEN_WHILE,
    TOKEN_EOF
} TokenType;


typedef enum {
    LITERAL_NONE,
    LITERAL_STRING,
    LITERAL_NUMBER,
} LiteralType;

typedef struct {
    LiteralType type;
    union {
        char* string;
        float number;
    } as;
} Literal;

typedef struct {
    Literal literal;
    char* lexeme;
    TokenType type;
    int line;
} Token;

typedef enum {
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_LITERAL,
    EXPR_GROUPING
} ExprType;

typedef struct Expr Expr;

struct Expr {
    ExprType type;
    union {
        struct {
            Expr* left;
            Token operator;
            Expr* right;
        } binary;

        struct {
            Expr* expr;
        } grouping;

       Literal literal;

        struct {
            Token operator;
            Expr* right;
        } unary;
    } as;
};

typedef struct {
    Token* items;
    size_t count;
    size_t capacity;
} TokenList;

#define da_append(xs, x)                                                             \
    do {                                                                             \
        if ((xs)->count >= (xs)->capacity) {                                         \
            if ((xs)->capacity == 0) (xs)->capacity = 256;                           \
            else (xs)->capacity *= 2;                                                \
            (xs)->items = realloc((xs)->items, (xs)->capacity*sizeof(*(xs)->items)); \
        }                                                                            \
        (xs)->items[(xs)->count++] = (x);                                            \
    } while (0)

typedef struct {
    char* source;
    TokenList tokens;
    int start;
    int current;
    int line;
} Scanner;

// Globals
int hadError = 0;
Scanner scanner;
Arena arena;
// End Globals

void initScanner(Scanner* scanner) {
	scanner->current = 0;
	scanner->line = 1;
	scanner->start = 0;
	scanner->tokens.count = 0;
}

bool isAtEnd() {
    return scanner.source[scanner.current] == '\0';
}

typedef struct {
	const char* name;
	TokenType token;
} Keyword;

static const Keyword KEYWORDS[] = {
    {"and",TOKEN_AND},
    {"class",TOKEN_CLASS},
    {"else",TOKEN_ELSE},
    {"false",TOKEN_FALSE},
    {"fun",TOKEN_FUN},
    {"for",TOKEN_FOR},
    {"if",TOKEN_IF},
    {"nil",TOKEN_NIL},
    {"or",TOKEN_OR},
    {"print",TOKEN_PRINT},
    {"return",TOKEN_RETURN},
    {"super",TOKEN_SUPER},
    {"this",TOKEN_THIS},
    {"true",TOKEN_TRUE},
    {"var",TOKEN_VAR},
    {"while",TOKEN_WHILE},
};

static const char* TOKEN_TYPE_NAMES[] = {
    [TOKEN_LEFT_PAREN] = "LEFT_PAREN", [TOKEN_RIGHT_PAREN] = "RIGHT_PAREN",
    [TOKEN_LEFT_BRACE] = "LEFT_BRACE", [TOKEN_RIGHT_BRACE] = "RIGHT_BRACE",
    [TOKEN_COMMA] = "COMMA",           [TOKEN_DOT] = "DOT",
    [TOKEN_MINUS] = "MINUS",           [TOKEN_PLUS] = "PLUS",
    [TOKEN_SEMICOLON] = "SEMICOLON",   [TOKEN_SLASH] = "SLASH",
    [TOKEN_STAR] = "STAR",             [TOKEN_BANG] = "BANG",
    [TOKEN_BANG_EQUAL] = "BANG_EQUAL", [TOKEN_EQUAL] = "EQUAL",
    [TOKEN_EQUAL_EQUAL] = "EQUAL_EQUAL",[TOKEN_GREATER] = "GREATER",
    [TOKEN_GREATER_EQUAL] = "GREATER_EQUAL", [TOKEN_LESS] = "LESS",
    [TOKEN_LESS_EQUAL] = "LESS_EQUAL", [TOKEN_IDENTIFIER] = "IDENTIFIER",
    [TOKEN_STRING] = "STRING",         [TOKEN_NUMBER] = "NUMBER",
    [TOKEN_AND] = "AND",               [TOKEN_CLASS] = "CLASS",
    [TOKEN_ELSE] = "ELSE",             [TOKEN_FALSE] = "FALSE",
    [TOKEN_FUN] = "FUN",               [TOKEN_FOR] = "FOR",
    [TOKEN_IF] = "IF",                 [TOKEN_NIL] = "NIL",
    [TOKEN_OR] = "OR",                 [TOKEN_PRINT] = "PRINT",
    [TOKEN_RETURN] = "RETURN",         [TOKEN_SUPER] = "SUPER",
    [TOKEN_THIS] = "THIS",             [TOKEN_TRUE] = "TRUE",
    [TOKEN_VAR] = "VAR",               [TOKEN_WHILE] = "WHILE",
    [TOKEN_EOF] = "EOF"
};

char* literalToString(Literal literal) {
    switch (literal.type) {
        case LITERAL_STRING:
            return literal.as.string ? literal.as.string : "";
            break; 
        case LITERAL_NUMBER:
            return arena_sprintf(&arena, "%g", literal.as.number);
            break;
        default: return ""; break;
    }
}

char* tokenToString(Token token) {
    return arena_sprintf(&arena, "%s %s %s", TOKEN_TYPE_NAMES[token.type], token.lexeme, literalToString(token.literal));
}

static void report(int line, char* where, char* message) {
    fprintf(stderr, "[line %d] Error %s: %s\n", line, where, message);
    hadError = 1;
}

static void error(int line, char* message) {
    report(line, "", message);
}

char advance() {
    return scanner.source[scanner.current++];
}

bool match(char expected) {
    if (isAtEnd() || scanner.source[scanner.current] != expected) return false;
    scanner.current++;
    return true;
}

char peek() {
    if (isAtEnd()) return '\0';
    return scanner.source[scanner.current];
}

char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.source[scanner.current + 1];
}

bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_';
}

void addTokenWithLiteral(TokenType type, Literal literal) {
    char* text = arena_substring(&arena,scanner.source, scanner.start, scanner.current - scanner.start);
    Token t = { .type = type, .lexeme = text, .literal = literal, .line = scanner.line };
    da_append(&scanner.tokens, t);
}

void addToken(TokenType type) {
    addTokenWithLiteral(type, (Literal){.type = LITERAL_NONE});
}

bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

void identifier() {
    while (isAlphaNumeric(peek())) advance();
    char* text = arena_substring(&arena, scanner.source, scanner.start, scanner.current - scanner.start);

    for (size_t i = 0; i < sizeof(KEYWORDS) / sizeof(KEYWORDS[0]); ++i) {
        if (strcmp(text, KEYWORDS[i].name) == 0) {
            addToken(KEYWORDS[i].token);
            return;
        }
    }
    addToken(TOKEN_IDENTIFIER);
}

void string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }

    if (isAtEnd()) {
        error(scanner.line, "Unterminated string.");
        return;
    }

    // the closing ".
    advance();

    // trim the surrounding quotes.
    size_t length = scanner.current - scanner.start - 2;
	char* value = arena_substring(&arena, scanner.source, scanner.start + 1, length);
    addTokenWithLiteral(TOKEN_STRING, (Literal){.type = LITERAL_STRING, .as.string = value});
}

void number() {
    while (isDigit(peek())) advance();

    // look for a fractional part.
    if (peek() == '.' && isDigit(peekNext())) {
        // Consume the "."
        advance();

        while (isDigit(peek())) advance();
    }

    addTokenWithLiteral(TOKEN_NUMBER, (Literal){.type = LITERAL_NUMBER, .as.number = strtof(arena_substring(&arena, scanner.source, scanner.start, scanner.current - scanner.start), NULL)});
}

void scanToken() {
    char c = advance();

    switch(c) {
        case '(': addToken(TOKEN_LEFT_PAREN); break;
        case ')': addToken(TOKEN_RIGHT_PAREN); break;
        case '{': addToken(TOKEN_LEFT_BRACE); break;
        case '}': addToken(TOKEN_RIGHT_BRACE); break;
        case ',': addToken(TOKEN_COMMA); break;
        case '.': addToken(TOKEN_DOT); break;
        case '-': addToken(TOKEN_MINUS); break;
        case '+': addToken(TOKEN_PLUS); break;
        case ';': addToken(TOKEN_SEMICOLON); break;
        case '*': addToken(TOKEN_STAR); break;
        case '!': addToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG); break;
        case '=': addToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL); break;
        case '<': addToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS); break;
        case '>': addToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER); break;
        case '/':
			if (match('/')) {
				while (peek() != '\n' && !isAtEnd()) advance();
			} else {
				addToken(TOKEN_SLASH);
			}
			break;
        case ' ':
        case '\r':
        case '\t':
			// ignore whitespace
			break;

        case '\n': scanner.line++; break;
        case '"': string(); break;
        default:
                  if (isDigit(c)) {
                      number();
                  } else if (isAlpha(c)) {
                      identifier();
                  } else {
                      error(scanner.line, "Unexpected character.");
                  }
                  break;
    }
}

void scanTokens() {
    while (!isAtEnd()) {
        scanner.start = scanner.current;
        scanToken();
    }

    Token t = {
        .type = TOKEN_EOF,
        .lexeme = "",
        .line = scanner.line,
        .literal = (Literal){.type = LITERAL_NONE}
    };
    da_append(&scanner.tokens, t);
}

void run(char* source) {
    scanner.source = source;

    size_t tokenCount = 0;
    scanTokens();

    for (size_t i = 0; i < scanner.tokens.count; ++i) {
        printf("%s\n", tokenToString(scanner.tokens.items[i]));
    }
}

int main(void) {
	initScanner(&scanner);
    initArena(&arena, 1024 * 1024);

	FILE* file = fopen("script.scc", "rb");

	if (file == NULL) {
		hadError = 1;
		fprintf(stderr, "file failed to load, aborting.\n");
		goto terminate;
	}

	fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    char *content = (char *)malloc(fileSize + 1);
    if (content == NULL) {
        fprintf(stderr, "memory allocation failed\n");
        fclose(file);
        goto terminate;
    }

    size_t bytesRead = fread(content, sizeof(char), fileSize, file);
    
    content[bytesRead] = '\0'; 

	run(content);
	
	free(scanner.tokens.items);
    free(content);
    fclose(file);

	terminate:
    	return hadError;
}
