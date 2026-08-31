#ifndef ARENA_H
#define ARENA_H

#ifndef UNITY_BUILD
    #include <stddef.h>
#endif

typedef struct {
	unsigned char* data;
	size_t offset;
	size_t capacity;
} Arena;

Arena arenaInit(size_t capacity);
void* arenaAlloc(Arena* a, size_t size);
void arenaFree(Arena* a);

#endif // ARENA_H

#ifdef ARENA_IMPL

#ifndef UNITY_BUILD
    #include <stdlib.h>
    #include <stdio.h>
#endif

Arena arenaInit(size_t capacity) {
	Arena a ={
		.capacity = capacity,
		.offset = 0
	};
	a.data = malloc(capacity);
	if (!a.data) {
		fprintf(stderr, "Malloc failed.\n");
		a.capacity = 0;
	}
	return a;
}

void* arenaAlloc(Arena* a, size_t size) {
    size_t alignedOffset = (a->offset + 7) & ~7;

    if (alignedOffset + size > a->capacity) {
        fprintf(stderr, "OOM.\n");
        return NULL;
    }

    unsigned char* ptr = &a->data[alignedOffset];
    a->offset = alignedOffset + size;
    return ptr;
}

void arenaFree(Arena* a) {
	a->capacity = 0;
	a->offset = 0;
	free(a->data);
	a->data = NULL;
}

#endif // ARENA_IMPL
