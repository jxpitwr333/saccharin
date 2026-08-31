// hashmap.h, based on the implementation of FNV-1a by Robert Nystrom in Crafting Interpreters
#ifndef HASHMAP_H
#define HASHMAP_H

#ifndef UNITY_BUILD
    #include <stdint.h>
    #include <stddef.h>
    #include <stdbool.h>
#endif

#define TABLE_MAX_LOAD 0.75

typedef enum {
    VALUE_NIL,
    VALUE_INT64,
    VALUE_STRING,
} ValueKind;

typedef struct {
    ValueKind kind;
    union {
        int64_t integer;
        char* string;
    } as;
} Value;

#define NIL_VAL ((Value){VALUE_NIL, {.integer = 0}})
#define INT_VAL(val) ((Value){VALUE_INT64, {.integer = (val)}})
#define STR_VAL(val) ((Value){VALUE_STRING, {.string = (val)}})

typedef struct {
    char* key;
    Value value;
} Entry;

typedef struct {
    Entry* entries;
    size_t count;
    size_t capacity;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableSet(Table* table, const char* key, Value value);
bool tableGet(Table* table, const char* key, Value* value);
bool tableDelete(Table* table, const char* key);

#endif // HASHMAP_H

#ifdef HASH_IMPL

#ifndef UNITY_BUILD
    #include <stdlib.h>
    #include <string.h>
#endif

static uint32_t hashString(const char* key, size_t length) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

static Entry* findEntry(Entry* entries, size_t capacity, const char* key) {
    uint32_t index = hashString(key, strlen(key)) & (capacity - 1);
    Entry* tombstone = NULL;

    while (1) {
        Entry* entry = &entries[index];
        if (entry->key == NULL) {
            if (entry->value.kind == VALUE_NIL) {
                return tombstone != NULL ? tombstone : entry;
            } else {
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (strcmp(entry->key, key) == 0) {
            return entry;
        }

        index = (index + 1) & (capacity - 1);
    }
}

static void adjustCapacity(Table* table, size_t capacity) {
    Entry* entries = (Entry*)malloc(sizeof(Entry) * capacity);
    for (size_t i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    table->count = 0;
    for (size_t i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key == NULL) continue;

        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    free(table->entries);
    table->entries = entries;
    table->capacity = capacity;
}

void initTable(Table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(Table* table) {
    for (size_t i = 0; i < table->count; ++i) {
        if (table->entries[i].key) free(table->entries[i].key);
    }
    free(table->entries);
    initTable(table);
}

static char* my_strdup(const char* source) {\
    size_t len = strlen(source) + 1;
    char *dest = malloc(len);
    if (dest) strcpy(dest, source);
    return dest;
}

bool tableSet(Table* table, const char* key, Value value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        size_t capacity = table->capacity < 128 ? 128 : table->capacity * 2;
        adjustCapacity(table, capacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;

    if (isNewKey && entry->value.kind == VALUE_NIL) table->count++;

    if (isNewKey) {
        entry->key = my_strdup(key);
    }

    entry->value = value;
    return isNewKey;
}

bool tableGet(Table* table, const char* key, Value* value) {
    if (table->count == 0) return false;

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    if (value != NULL) *value = entry->value;
    return true;
}

bool tableDelete(Table* table, const char* key) {
    if (table->count == 0) return false;

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    free(entry->key);
    entry->key = NULL;
    entry->value = INT_VAL(1);

    return true;
}

#endif // HASH_IMPL
