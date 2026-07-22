#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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

int isAtEnd() {
    return (scanner.current >= strlen(scanner.source)) ? 0 : 1;
}

const char* TOKENS[] = {
    [TOKEN_LEFT_PAREN] 	= "(",
    [TOKEN_RIGHT_PAREN] = ")",
    [TOKEN_LEFT_BRACE] 	= "{",
    [TOKEN_RIGHT_BRACE] = "}",
    [TOKEN_COMMA] 		= ",",
    [TOKEN_DOT] 		= ".",
    [TOKEN_MINUS] 		= "-",
    [TOKEN_PLUS] 		= "+",
    [TOKEN_SEMICOLON] 	= ";",
    [TOKEN_SLASH] 		= "/",
    [TOKEN_STAR] 		= "*",
    [TOKEN_BANG] 		= "!",
    [TOKEN_BANG_EQUAL] 	= "!=",
    [TOKEN_EQUAL] 		= "=",
    [TOKEN_EQUAL_EQUAL] = "==",
    [TOKEN_GREATER] 	= ">",
    [TOKEN_GREATER_EQUAL] = ">=",
    [TOKEN_LESS] 		= "<",
    [TOKEN_LESS_EQUAL] 	= "<=",
    [TOKEN_IDENTIFIER] 	= "identifier",
    [TOKEN_STRING] 		= "string",
    [TOKEN_NUMBER] 		= "number",
    [TOKEN_AND] 		= "and",
    [TOKEN_CLASS] 		= "class",
    [TOKEN_ELSE] 		= "else",
    [TOKEN_FALSE] 		= "false",
    [TOKEN_FUN] 		= "fun",
    [TOKEN_FOR] 		= "for",
    [TOKEN_IF] 			= "if",
    [TOKEN_NIL] 		= "nil",
    [TOKEN_OR] 			= "or",
    [TOKEN_PRINT] 		= "print",
    [TOKEN_RETURN] 		= "return",
    [TOKEN_SUPER] 		= "super",
    [TOKEN_THIS] 		= "this",
    [TOKEN_TRUE] 		= "true",
    [TOKEN_VAR] 		= "var",
    [TOKEN_WHILE] 		= "while",
    [TOKEN_EOF] 		= "EOF",
};

char* literalToString(Literal literal) {
    switch (literal.type) {
        case LITERAL_STRING:
            return arena_sprintf(&arena, "%f", literal.as.number);
            break; 
        case LITERAL_NUMBER:
            return arena_sprintf(&arena, "%f", literal.as.number);
            break;
        default: return ""; break;
    }
}

char* tokenToString(Token token) {
    return arena_sprintf(&arena, "%s %s %s", TOKENS[token.type], token.lexeme, literalToString(token.literal));
}

static void report(int line, char* where, char* message) {
    fprintf(stderr, "[line %d] Error %s: %s/n", line, where, message);
    hadError = 1;
}

static void error(int line, char* message) {
    report(line, "", message);
}

char advance() {
    return scanner.source[scanner.current++];
}

int match(char expected) {
    if (isAtEnd() || scanner.source[scanner.current] != expected) return 1;

    scanner.current++;
    return 0;
}

char peek() {
    if (isAtEnd()) return '\0';
    return scanner.source[scanner.current];
}

char peekNext() {
    if (scanner.current + 1 >= strlen(scanner.source)) return '\0';
    return scanner.source[scanner.current + 1];
}

int isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_';
}

void addTokenWithLiteral(TokenType type, Literal literal) {
    char* text = arena_substring(&arena,scanner.source, scanner.start, scanner.current);
    Token t = { .type = type, .lexeme = text, .literal = literal, .line = scanner.line };
    da_append(&scanner.tokens, t);
}

void addToken(TokenType type) {
    addTokenWithLiteral(type, (Literal){.type = LITERAL_NONE});
}

int isDigit(char c) {
    return c >= '0' && c <= '9';
}

int isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

int string_equals(const char* a, const char* b) {
    size_t alen, blen;
    alen = strlen(a);
    blen = strlen(b);

    if (alen != blen) return 1;
    for (size_t i = 0; i < alen; ++i) {
        if (a[i] != b[i]) return 1;
    }
    return 0;
}

void identifier() {
    while (isAlphaNumeric(peek())) advance();

    const char* text = arena_substring(&arena, scanner.source, scanner.start, scanner.current);
    for (size_t i = 0; i < sizeof(TOKENS) / sizeof(char *); ++i) {
        if (string_equals(text, TOKENS[i])) {
            TokenType type = (TokenType)i;
            addToken(type);
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
    char* value = arena_substring(&arena, scanner.source, scanner.start + 1, scanner.current - 1);
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

    addTokenWithLiteral(TOKEN_NUMBER, (Literal){.type = LITERAL_NUMBER, .as.number = strtof(arena_substring(&arena, scanner.source, scanner.start, scanner.current), NULL)});
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

    }
}

int main(void) {
    initArena(&arena, 1024 * 1024);

    return hadError;
}
