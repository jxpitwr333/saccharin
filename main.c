#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct Scanner Scanner;
typedef struct Parser Parser;
typedef struct Arena Arena;
typedef struct Program Program;
typedef struct Hashtable Hashtable;
typedef struct Environment Environment;

// GLOBALS
int hadError = 0;
int panicMode = 0;
Program program;
Scanner scanner;
Parser parser;
Arena stringArena;
Arena exprArena;
Arena stmtArena;
Environment environment;
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
        double number;
        bool boolean;
    } as;
} Literal;

typedef struct {
    char* key;
    Literal value;
} Entry;

struct Hashtable {
    Entry* entries;
    size_t count;
    size_t capacity;
};

struct Environment {
    Hashtable values;
    Environment* enclosing;
};

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
    EXPR_GROUPING,
    EXPR_VARIABLE,
    EXPR_ASSIGN
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

        struct {
            Token name;
        } variable;

        struct {
            Token name;
            Expr* value;
        } assign;
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

typedef enum {
    STMT_EXPR,
    STMT_PRINT,
    STMT_VAR
} StmtType;

typedef struct {
    StmtType type;
    union {
        Expr* expr;
        Expr* print;
        struct {
            Token name;
            Expr* initializer;
        } var;
    } as;
} Stmt;

typedef struct {
    Stmt** items;
    size_t count;
    size_t capacity;
} StmtList;

struct Program {
    StmtList statements;
};

// END STRUCTS

// FORWARD DECLARATIONS
void initArena(Arena* arena, size_t capacity);
void freeArena(Arena* arena);
void arenaReset(Arena* arena);
void* arena_alloc(Arena* arena, size_t size);
char* arena_sprintf(Arena* arena, const char* fmt, ...);
char* arena_substring(Arena* arena, const char* str, size_t start, size_t length);
void initTable(Hashtable* table);
uint32_t hashString(char* key);
Entry* findEntry(Entry* entries, int capacity, char* key);
void adjustCapacity(Hashtable* table, size_t capacity);
Literal* tableGet(Hashtable* table, char* key);
bool tableAdd(Hashtable* table, char* key, Literal value);
void freeTable(Hashtable* table);
void envDefine(Token name, Literal value);
Literal getVariable(Token name);
void initScanner(Scanner* scanner);
void initParser(Parser* parser);
bool isAtEnd(void);
char* literalToString(Literal literal);
char* tokenToString(Token token);
void report(int line, char* where, char* message);
void error(int line, char* message);
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
Expr* newExprVar(Token token);
Expr* newExprAssign(Token name, Expr* value);
Token tokenPeek(void);
bool parserIsAtEnd();
bool tokenCheck(TokenType type);
Token tokenPrevious(void);
Token tokenAdvance(void);
bool tokenMatch(TokenType type);
bool va_tokenMatch(size_t count, ...);
Expr* expression(void);
Expr* assignment(void);
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
bool isTruthy(Literal lit);
bool isEqual(Literal litA, Literal litB);
Literal evaluate(Expr* expr);
void parseStatements(StmtList* statements);
Stmt* declaration();
Stmt* varDeclaration();
Stmt* newStmt(StmtType type, Expr* expr, Token name);
Stmt* statement(void);
Stmt* printStatement(void);
Stmt* expressionStatement(void);
void execute(Stmt* stmt);
void interpret(StmtList* statements);

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
    initArena(&stmtArena, MEBIBYTE);
    initTable(&environment.values);

    content[bytesRead] = '\0';

	run(content);

	free(scanner.tokens.items);
	freeTable(&environment.values);
	freeArena(&stringArena);
	freeArena(&stmtArena);
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

void initTable(Hashtable* table) {
    table->entries = NULL;
    table->count = 0;
    table->capacity = 0;
}

uint32_t hashString(char* key) {
    size_t length = strlen(key);
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

Entry* findEntry(Entry* entries, int capacity, char* key) {
  uint32_t index = hashString(key) % capacity;
  for (;;) {
    Entry* entry = &entries[index];
    if (entry->key == NULL || strcmp(entry->key, key) == 0) {
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

Literal* tableGet(Hashtable* table, char* key) {
  if (table->count == 0) return NULL;

  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) return NULL;

  return &entry->value;
}

void adjustCapacity(Hashtable* table, size_t capacity) {
    Entry* entries = (Entry*)calloc(capacity, sizeof(Entry));

    table->count = 0;
    for (size_t i = 0; i < table->capacity; i++) {
        Entry* source = &table->entries[i];
        if (source->key == NULL) continue;

        Entry* dest = findEntry(entries, capacity, source->key);
        dest->key = source->key;
        dest->value = source->value;
        table->count++;
    }

    free(table->entries);
    table->entries = entries;
    table->capacity = capacity;
}

bool tableAdd(Hashtable* table, char* key, Literal value) {
    if (table->count + 1 > table->capacity * 0.75f) {
        size_t newCapacity = table->capacity < 256 ? 256 : table->capacity * 2;
        adjustCapacity(table, newCapacity);
      }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;
    if (isNewKey) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

void freeTable(Hashtable* table) {
    free(table->entries);
}

void envDefine(Token name, Literal value) {
    tableAdd(&environment.values, name.lexeme, value);
}

Literal getVariable(Token name) {
    Literal* val = tableGet(&environment.values, name.lexeme);
    if (val) return *val;
    else return (Literal){.type = LITERAL_NONE};
}

void initEnvironment(Environment* env) {
    env->enclosing = NULL;
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

void report(int line, char* where, char* message) {
    fprintf(stderr, "[line %d] Error %s: %s\n", line, where, message);
    hadError = 1;
}

void error(int line, char* message) {
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

    addTokenWithLiteral(TOKEN_NUMBER, (Literal){.type = LITERAL_NUMBER, .as.number = strtod(arena_substring(&stringArena, scanner.source, scanner.start, scanner.current - scanner.start), NULL)});
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

    hadError = 0;
    panicMode = 0;
    program.statements.count = 0;
    program.statements.capacity = 0;
    program.statements.items = NULL;
    parser.tokens = scanner.tokens;

    scanTokens();

    // Stop if there was a syntax error.
    if (hadError) return;

    parser.tokens = scanner.tokens;
    parseStatements(&program.statements);

    if (hadError) return;

    interpret(&program.statements);
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

Expr* newExprVar(Token name) {
    Expr* expr = arena_alloc(&exprArena, sizeof(Expr));
    expr->type = EXPR_VARIABLE;
    expr->as.variable.name = name;
    return expr;
}

Expr* newExprAssign(Token name, Expr* value) {
    Expr* expr = arena_alloc(&exprArena, sizeof(Expr));
    expr->type = EXPR_ASSIGN;
    expr->as.assign.name = name;
    expr->as.assign.value = value;
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
        case EXPR_VARIABLE:
            printf("%s = %s", expr->as.variable.name.lexeme ,literalToString(expr->as.variable.name.literal));
            break;
        case EXPR_ASSIGN:
            printf("%s", expr->as.assign.name.lexeme);
            printAst(expr->as.assign.value);
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
    return assignment();
}

Expr* assignment(void) {
    Expr* expr = equality();

    if (tokenMatch(TOKEN_EQUAL)) {
        Token equals = tokenPrevious();
        Expr* value = assignment();

        if (expr->type == EXPR_VARIABLE) {
            Token name = expr->as.variable.name;
            return newExprAssign(name, value);
        }

        error(equals.line, "Invalid assignment target.");
    }

    return expr;
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

    if (tokenMatch(TOKEN_IDENTIFIER)) {
        return newExprVar(tokenPrevious());
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

bool isTruthy(Literal lit) {
    if (lit.type == LITERAL_NONE) return false;
    if (lit.type == LITERAL_BOOLEAN) return lit.as.boolean;
    return true;
}

bool isEqual(Literal litA, Literal litB) {
    if (litA.type != litB.type) return false;
    switch(litA.type) {
        case LITERAL_NONE: return true; /* this is horrible! */ break;
        case LITERAL_BOOLEAN: return (litA.as.boolean == litB.as.boolean); break;
        case LITERAL_NUMBER: return (litA.as.number == litB.as.number); break;
        case LITERAL_STRING: return (strcmp(litA.as.string, litB.as.string) == 0); break;
        default: return false; break;
    }
    // unreachable
    return false;
}

Literal evaluate(Expr* expr) {
    if (!expr) return (Literal){ .type = LITERAL_NONE };

    switch (expr->type) {
        case EXPR_ASSIGN: {
            Literal value = evaluate(expr->as.assign.value);
            envDefine(expr->as.assign.name, value); /* assignment uses the same function as definition because tableAdd handles the "if key exists, replace value" case */
            return value;
        }
        case EXPR_VARIABLE:
            return getVariable(expr->as.variable.name);

        case EXPR_LITERAL:
            return expr->as.literal;

        case EXPR_GROUPING:
            return evaluate(expr->as.grouping.expr);

        case EXPR_UNARY: {
            Literal right = evaluate(expr->as.unary.right);
            TokenType op = expr->as.unary.operator.type;

            if (op == TOKEN_MINUS) {
                if (right.type != LITERAL_NUMBER) {
                    error(expr->as.unary.operator.line, "Operand must be a number.");
                    return (Literal){ .type = LITERAL_NONE };
                }
                return (Literal){ .type = LITERAL_NUMBER, .as.number = -right.as.number };
            }
            else if (op == TOKEN_BANG) {
                return (Literal){ .type = LITERAL_BOOLEAN, .as.boolean = !isTruthy(right) };
            }
            break;
        }

        case EXPR_BINARY: {
            Literal left = evaluate(expr->as.binary.left);
            Literal right = evaluate(expr->as.binary.right);
            TokenType op = expr->as.binary.operator.type;

            switch (op) {
                // Arithmetic
                case TOKEN_MINUS:
                    if (left.type != LITERAL_NUMBER || right.type != LITERAL_NUMBER) {
                        error(expr->as.binary.operator.line, "Operands must be numbers.");
                        return (Literal){ .type = LITERAL_NONE };
                    }
                    return (Literal){ .type = LITERAL_NUMBER, .as.number = left.as.number - right.as.number };

                case TOKEN_STAR:
                    if (left.type != LITERAL_NUMBER || right.type != LITERAL_NUMBER) {
                        error(expr->as.binary.operator.line, "Operands must be numbers.");
                        return (Literal){ .type = LITERAL_NONE };
                    }
                    return (Literal){ .type = LITERAL_NUMBER, .as.number = left.as.number * right.as.number };

                case TOKEN_SLASH:
                    if (left.type != LITERAL_NUMBER || right.type != LITERAL_NUMBER) {
                        error(expr->as.binary.operator.line, "Operands must be numbers.");
                        return (Literal){ .type = LITERAL_NONE };
                    }
                    return (Literal){ .type = LITERAL_NUMBER, .as.number = left.as.number / right.as.number };

                case TOKEN_PLUS:
                    // Number addition
                    if (left.type == LITERAL_NUMBER && right.type == LITERAL_NUMBER) {
                        return (Literal){ .type = LITERAL_NUMBER, .as.number = left.as.number + right.as.number };
                    }
                    // String concatenation (and concatenation of mismatch types)
                    if (left.type == LITERAL_STRING || right.type == LITERAL_STRING) {
                        char* strLeft = "";
                        char* strRight = "";
                        if (left.type != LITERAL_STRING) {
                            strLeft = literalToString(left);
                        } else strLeft = left.as.string;
                        if (right.type != LITERAL_STRING) {
                            strRight = literalToString(right);
                        } else strRight = right.as.string;
                        char* concat = arena_sprintf(&stringArena, "%s%s", strLeft, strRight);
                        return (Literal){ .type = LITERAL_STRING, .as.string = concat };
                    }
                    error(expr->as.binary.operator.line, "Operands must be two numbers or two strings.");
                    return (Literal){ .type = LITERAL_NONE };

                // Comparison operators
                case TOKEN_GREATER:
                    return (Literal){ .type = LITERAL_BOOLEAN, .as.boolean = left.as.number > right.as.number };
                case TOKEN_GREATER_EQUAL:
                    return (Literal){ .type = LITERAL_BOOLEAN, .as.boolean = left.as.number >= right.as.number };
                case TOKEN_LESS:
                    return (Literal){ .type = LITERAL_BOOLEAN, .as.boolean = left.as.number < right.as.number };
                case TOKEN_LESS_EQUAL:
                    return (Literal){ .type = LITERAL_BOOLEAN, .as.boolean = left.as.number <= right.as.number };

                // Equality operators
                case TOKEN_EQUAL_EQUAL:
                    return (Literal){ .type = LITERAL_BOOLEAN, .as.boolean = isEqual(left, right) };
                case TOKEN_BANG_EQUAL:
                    return (Literal){ .type = LITERAL_BOOLEAN, .as.boolean = !isEqual(left, right) };

                default: break;
            }
            break;
        }
    }

    return (Literal){ .type = LITERAL_NONE };
}

void parseStatements(StmtList* statements) {
    while (!parserIsAtEnd()) {
        da_append(statements, declaration());
    }
}

Stmt* declaration() {
    if (tokenMatch(TOKEN_VAR)) return varDeclaration();
    return statement();

    if (panicMode) { /* BUT WHO SETS PANIC MODE? */
        synchronize();
        return NULL;
    }
}

Stmt* varDeclaration() {
    Token name = expect(TOKEN_IDENTIFIER, "Expect variable name.");

    Expr* initializer = NULL;
    if (tokenMatch(TOKEN_EQUAL)) initializer = expression();

    expect(TOKEN_SEMICOLON, "Expect ';' after variable declaration");
    return newStmt(STMT_VAR, initializer, name);
}

Stmt* newStmt(StmtType type, Expr* expr, Token name) {
    Stmt* stmt = arena_alloc(&stmtArena, sizeof(Stmt));
    stmt->type = type;
    switch (type) {
        case STMT_EXPR: stmt->as.expr = expr; break;
        case STMT_PRINT: stmt->as.print = expr; break;
        case STMT_VAR:
            stmt->as.var.initializer = expr;
            stmt->as.var.name = name;
        break;
    }
    return stmt;
}

Stmt* statement(void) {
    if (tokenMatch(TOKEN_PRINT)) return printStatement();
    return expressionStatement();
}

Stmt* printStatement(void) {
    Expr* value = expression();
    expect(TOKEN_SEMICOLON, "Expect ';' after value.");
    return newStmt(STMT_PRINT, value, (Token){});
}

Stmt* expressionStatement(void) {
    Expr* value = expression();
    expect(TOKEN_SEMICOLON, "Expect ';' after value.");
    return newStmt(STMT_EXPR, value, (Token){});
}

void execute(Stmt* stmt) {
    switch (stmt->type) {
        case STMT_EXPR:
            evaluate(stmt->as.expr);
            break;
        case STMT_PRINT:
            { /* Label followed by a declaration is a C23 extension. it took them 50+ years to fix this? lol */
                Literal val = evaluate(stmt->as.expr);
                printf("%s\n", literalToString(val));
            }
            break;
        case STMT_VAR:
            {
                Literal value;
                if (stmt->as.var.initializer != NULL) {
                    value = evaluate(stmt->as.var.initializer);
                }

                envDefine(stmt->as.var.name, value);
            }
            break;
    }
}

void interpret(StmtList* statements) {
    if (!statements) return;

    hadError = 0;

    for (size_t i = 0; i < statements->count; ++i) {
        execute(statements->items[i]);
        if (hadError) return;
    }
}

// END DEFINITIONS
