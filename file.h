#ifndef FILE_H
#define FILE_H

#ifndef UNITY_BUILD
    #include <stddef.h>
#endif

char* readFileBinary(char* fileName, size_t* fileSizeOut);

#endif

#ifdef FILE_IMPL

#ifndef UNITY_BUILD
    #include <stdio.h>
    #include <stdlib.h>
#endif

char* readFileBinary(char* fileName, size_t* fileSizeOut) {
    if (fileSizeOut) *fileSizeOut = 0;

    FILE* in = fopen(fileName, "rb");
    if (!in) return NULL;

    char* source = NULL;

    if (fseek(in, 0, SEEK_END) != 0) goto cleanup;

    long size = ftell(in);
    if (size < 0) goto cleanup;

    if (fseek(in, 0, SEEK_SET) != 0) goto cleanup;

    source = (char*)malloc((size_t)size + 1);
    if (!source) goto cleanup;

    if (fread(source, 1, (size_t)size, in) != (size_t)size) {
        free(source);
        source = NULL;
        goto cleanup;
    }

    source[size] = '\0';
    if (fileSizeOut) *fileSizeOut = (size_t)size;

cleanup:
    fclose(in);
    return source;
}

#endif
