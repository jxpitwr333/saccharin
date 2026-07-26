#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

typedef struct Scanner Scanner;
typedef struct Parser Parser;
typedef struct Arena Arena;

// GLOBALS
int hadError = 0;
int panicMode = 0;
Scanner scanner;
Parser parser;
Arena stringArena;
Arena exprArena;
// END GLOBALS

// MACROS
#define da_append(xs, x)                                                             \
    do {                                                                             \
        if ((xs)->count >= (xs)->capacity) {                                         \
            if ((xs)->capacity == 0) (xs)->capacity = 256;                           \
            else (xs)->capacity *= 2;                                                \
            (xs)->items = realloc((xs)->items, (xs)->capacity*sizeof(*(xs)->items)); \
        }                                                                            \
        (xs)->items[(xs)->count++] = (x);                                            \
    } while (0)

#define MEBIBYTE 1048576
// END MACROS

// STRUCTS
struct Arena {
    char* data;
    size_t capacity;
    size_t offset;
};

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
    LITERAL_BOOLEAN
} LiteralType;

typedef struct {
    LiteralType type;
    union {
        char* string;
        float number;
        bool boolean;
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

struct Scanner {
    char* source;
    TokenList tokens;
    int start;
    int current;
    int line;
};

typedef struct {
	const char* name;
	TokenType token;
} Keyword;

struct Parser {
    TokenList tokens;
    int current;
};

// END STRUCTS

// FORWARD DECLARATIONS
void initArena(Arena* arena, size_t capacity);
void freeArena(Arena* arena);
void arenaReset(Arena* arena);
void* arena_alloc(Arena* arena, size_t size);
char* arena_sprintf(Arena* arena, const char* fmt, ...);
char* arena_substring(Arena* arena, const char* str, size_t start, size_t length);
void initScanner(Scanner* scanner);
void initParser(Parser* parser);
bool isAtEnd(void);
char* literalToString(Literal literal);
char* tokenToString(Token token);
static void report(int line, char* where, char* message);
static void error(int line, char* message);
char advance(void);
bool match(char expected);
char peek(void);
char peekNext(void);
bool isAlpha(char c);
void addTokenWithLiteral(TokenType type, Literal literal);
void addToken(TokenType type);
bool isDigit(char c);
bool isAlphaNumeric(char c);
void identifier(void);
void string(void);
void number(void);
void scanToken(void);
void scanTokens(void);
void run(char* source);
void printAst(Expr* expr);
Expr* newUnaryExpr(Token operator, Expr* right);
Expr* newBinaryExpr(Expr* left, Token operator, Expr* right);
Expr* newLiteralExpr(Literal literal);
Expr* newGroupingExpr(Expr* group);
Token tokenPeek(void);
bool parserIsAtEnd();
bool tokenCheck(TokenType type);
Token tokenPrevious(void);
Token tokenAdvance(void);
bool tokenMatch(TokenType type);
bool va_tokenMatch(size_t count, ...);
Expr* expression(void);
Expr* equality(void);
Expr* comparison(void);
Expr* term(void);
Expr* factor(void);
Expr* unary(void);
Expr* primary(void);
Token expect(TokenType type, const char* error_msg);
void parserError(Token token, const char* message);
void synchronize(void);
Expr* parse(void);

// END FORWARD DECLARATIONS

int main(void) {
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
        hadError = 1;
        fprintf(stderr, "memory allocation failed\n");
        fclose(file);
        goto terminate;
    }

    size_t bytesRead = fread(content, sizeof(char), fileSize, file);
    initScanner(&scanner);
    initParser(&parser);

    initArena(&stringArena, MEBIBYTE);
    initArena(&exprArena, MEBIBYTE);

    content[bytesRead] = '\0';

	run(content);

	free(scanner.tokens.items);
	freeArena(&stringArena);
	freeArena(&exprArena);
    free(content);
    fclose(file);

	terminate:
    	return hadError;
}

// DEFINITIONS
const Keyword KEYWORDS[] = {
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

const char* TOKEN_TYPE_NAMES[] = {
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

void* arena_alloc(Arena* arena, size_t size) {
    size_t alignedOffset = (arena->offset + 7) & ~7;

    if (alignedOffset + size > arena->capacity) {
        fprintf(stderr, "out of memory!\n");
        return NULL;
    }

    void* ptr = &arena->data[alignedOffset];
    arena->offset = alignedOffset + size;
    return ptr;
}

char* arena_sprintf(Arena* arena, const char* fmt, ...) {
    va_list args, args_copy;
    va_start(args, fmt);
    va_copy(args_copy, args);

    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (needed < 0) {
        va_end(args_copy);
        return NULL;
    }

    char* str = (char*)arena_alloc(arena, needed + 1);
    if (!str) {
        va_end(args_copy);
        return NULL;
    }

    vsnprintf(str, needed + 1, fmt, args_copy);
    va_end(args_copy);

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

    char* sub = (char*)arena_alloc(arena, length + 1);
    if (!sub) return NULL;

    memcpy(sub, str + start, length);
    sub[length] = '\0';

    return sub;
}

void initScanner(Scanner* scanner) {
	scanner->current = 0;
	scanner->line = 1;
	scanner->start = 0;
	scanner->tokens.count = 0;
}

void initParser(Parser* parser) {
	parser->current = 0;
	parser->tokens.count = 0;
}

bool isAtEnd(void) {
    return scanner.source[scanner.current] == '\0';
}

char* literalToString(Literal literal) {
    switch (literal.type) {
        case LITERAL_STRING:
            return literal.as.string ? literal.as.string : "";
            break;
        case LITERAL_NUMBER:
            return arena_sprintf(&stringArena, "%g", literal.as.number);
            break;
        case LITERAL_BOOLEAN:
            return arena_sprintf(&stringArena, "%s", literal.as.boolean ? "TRUE" : "FALSE");
            break;
        default: return ""; break;
    }
}

char* tokenToString(Token token) {
    return arena_sprintf(&stringArena, "%s %s %s", TOKEN_TYPE_NAMES[token.type], token.lexeme, literalToString(token.literal));
}

static void report(int line, char* where, char* message) {
    fprintf(stderr, "[line %d] Error %s: %s\n", line, where, message);
    hadError = 1;
}

static void error(int line, char* message) {
    report(line, "", message);
}

char advance(void) {
    return scanner.source[scanner.current++];
}

bool match(char expected) {
    if (isAtEnd() || scanner.source[scanner.current] != expected) return false;
    scanner.current++;
    return true;
}

char peek(void) {
    if (isAtEnd()) return '\0';
    return scanner.source[scanner.current];
}

char peekNext(void) {
    if (isAtEnd()) return '\0';
    return scanner.source[scanner.current + 1];
}

bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_';
}

void addTokenWithLiteral(TokenType type, Literal literal) {
    char* text = arena_substring(&stringArena,scanner.source, scanner.start, scanner.current - scanner.start);
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

void identifier(void) {
    while (isAlphaNumeric(peek())) advance();
    char* text = arena_substring(&stringArena, scanner.source, scanner.start, scanner.current - scanner.start);

    for (size_t i = 0; i < sizeof(KEYWORDS) / sizeof(KEYWORDS[0]); ++i) {
        if (strcmp(text, KEYWORDS[i].name) == 0) {
            addToken(KEYWORDS[i].token);
            return;
        }
    }
    addToken(TOKEN_IDENTIFIER);
}

void string(void) {
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
	char* value = arena_substring(&stringArena, scanner.source, scanner.start + 1, length);
    addTokenWithLiteral(TOKEN_STRING, (Literal){.type = LITERAL_STRING, .as.string = value});
}

void number(void) {
    while (isDigit(peek())) advance();

    // look for a fractional part.
    if (peek() == '.' && isDigit(peekNext())) {
        // Consume the "."
        advance();

        while (isDigit(peek())) advance();
    }

    addTokenWithLiteral(TOKEN_NUMBER, (Literal){.type = LITERAL_NUMBER, .as.number = strtof(arena_substring(&stringArena, scanner.source, scanner.start, scanner.current - scanner.start), NULL)});
}

void scanToken(void) {
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

void scanTokens(void) {
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

    parser.tokens = scanner.tokens;
    Expr* expression = parse();

    // Stop if there was a syntax error.
    if (hadError) return;

    printAst(expression);
}

Expr* newUnaryExpr(Token operator, Expr* right) {
    Expr* expr = arena_alloc(&exprArena, sizeof(Expr));
    expr->type = EXPR_UNARY;
    expr->as.unary.operator = operator;
    expr->as.unary.right = right;
    return expr;
}

Expr* newBinaryExpr(Expr* left, Token operator, Expr* right) {
    Expr* expr = arena_alloc(&exprArena, sizeof(Expr));
    expr->type = EXPR_BINARY;
    expr->as.binary.left = left;
    expr->as.binary.operator = operator;
    expr->as.binary.right = right;
    return expr;
}

Expr* newLiteralExpr(Literal literal) {
    Expr* expr = arena_alloc(&exprArena, sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->as.literal = literal;
    return expr;
}

Expr* newGroupingExpr(Expr* group) {
    Expr* expr = arena_alloc(&exprArena, sizeof(Expr));
    expr->type = EXPR_GROUPING;
    expr->as.grouping.expr = group;
    return expr;
}

void printAst(Expr* expr) {
    if (!expr) return;

    switch (expr->type) {
        case EXPR_LITERAL:
            printf("%s", literalToString(expr->as.literal));
            break;
        case EXPR_UNARY:
            printf("(%s ", expr->as.unary.operator.lexeme);
            printAst(expr->as.unary.right);
            printf(")");
            break;
        case EXPR_BINARY:
            printf("(");
            printAst(expr->as.binary.left);
            printf(" %s ", expr->as.binary.operator.lexeme);
            printAst(expr->as.binary.right);
            printf(")");
            break;
        case EXPR_GROUPING:
            printf("(group ");
            printAst(expr->as.grouping.expr);
            printf(")");
            break;
    }
}

Token tokenPeek(void) {
    return parser.tokens.items[parser.current];
}

bool parserIsAtEnd() {
  return tokenPeek().type == TOKEN_EOF;
}

bool tokenCheck(TokenType type) {
    if (parserIsAtEnd()) return false;
    return tokenPeek().type == type;
}

Token tokenPrevious(void) {
    return parser.tokens.items[parser.current - 1];
}

Token tokenAdvance(void) {
    if (!parserIsAtEnd()) parser.current++;
    return tokenPrevious();
}

bool tokenMatch(TokenType type) {
    if (tokenCheck(type)) {
        tokenAdvance();
        return true;
    }
    return false;
}

bool va_tokenMatch(size_t count, ...) {
    va_list args;
    va_start(args, count);

    for (size_t i = 0; i < count; ++i) {
        TokenType type = va_arg(args, TokenType);

        if (tokenCheck(type)) {
            tokenAdvance();
            va_end(args);
            return true;
        }
    }

    va_end(args);
    return false;
}

Expr* expression(void) {
    return equality();
}

Expr* equality(void) {
    Expr* expr = comparison();
    while (va_tokenMatch(2, TOKEN_BANG_EQUAL, TOKEN_EQUAL_EQUAL)) {
        Token operator = tokenPrevious();
        Expr* right = comparison();
        expr = newBinaryExpr(expr, operator, right);
    }
    return expr;
}

Expr* comparison(void) {
  Expr* expr = term();
  while (va_tokenMatch(4, TOKEN_GREATER, TOKEN_GREATER_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL)) {
    Token operator = tokenPrevious();
    Expr* right = term();
    expr = newBinaryExpr(expr, operator, right);
  }

  return expr;
}

Expr* term(void) {
  Expr* expr = factor();

  while (va_tokenMatch(2, TOKEN_MINUS, TOKEN_PLUS)) {
    Token operator = tokenPrevious();
    Expr* right = factor();
    expr = newBinaryExpr(expr, operator, right);
  }

  return expr;
}

Expr* factor(void) {
    Expr* expr = unary();

    while (va_tokenMatch(2, TOKEN_SLASH, TOKEN_STAR)) {
      Token operator = tokenPrevious();
      Expr* right = unary();
      expr = newBinaryExpr(expr, operator, right);
    }

    return expr;
}

Expr* unary(void) {
    if (va_tokenMatch(2, TOKEN_BANG, TOKEN_MINUS)) {
      Token operator = tokenPrevious();
      Expr* right = unary();
      return newUnaryExpr(operator, right);
    }

    return primary();
}

Expr* primary(void) {
    if (tokenMatch(TOKEN_FALSE)) return newLiteralExpr((Literal){.type = LITERAL_BOOLEAN, .as.boolean = false});
    if (tokenMatch(TOKEN_TRUE)) return newLiteralExpr((Literal){.type = LITERAL_BOOLEAN, .as.boolean = true});
    if (tokenMatch(TOKEN_NIL)) return newLiteralExpr((Literal){.type = LITERAL_NONE});

    if (va_tokenMatch(2, TOKEN_NUMBER, TOKEN_STRING)) {
      return newLiteralExpr(tokenPrevious().literal);
    }

    if (tokenMatch(TOKEN_LEFT_PAREN)) {
      Expr* expr = expression();
      expect(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
      return newGroupingExpr(expr);
    }

    // unreachable
    parserError(tokenPeek(), "Expect expression.");
    return NULL;
}

// consume
Token expect(TokenType type, const char* error_msg) {
    if (tokenCheck(type)) {
        return tokenAdvance();
    }

    parserError(tokenPeek(), error_msg);

    return tokenPeek();
}

void parserError(Token token, const char* message) {
    if (panicMode) return;

    panicMode = 1;
    hadError = 1;

    if (token.type == TOKEN_EOF) {
        fprintf(stderr, "[line %d] Error at end: %s\n", token.line, message);
    } else {
        fprintf(stderr, "[line %d] Error at '%s': %s\n", token.line, token.lexeme, message);
    }
}

void synchronize(void) {
    tokenAdvance();

    while (!parserIsAtEnd()) {
      if (tokenPrevious().type == TOKEN_SEMICOLON) return;

      switch (tokenPeek().type) {
        case TOKEN_CLASS:
        case TOKEN_FUN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
          return;
        default:
            break;
      }

      tokenAdvance();
    }
}

Expr* parse(void) {
    hadError = 0;
    panicMode = 0;

    Expr* expr = expression();

    if (hadError) {
        return NULL;
    }

    return expr;
}
// END DEFINITIONS
