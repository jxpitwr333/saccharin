#ifndef MACROS_H
#define MACROS_H

#ifndef UNITY_BUILD
    #include <stdlib.h>
#endif

#define MB * 1048576ULL

#define da_append(xs, x) do {\
	if ((xs)->count >= (xs)->capacity) {\
		(xs)->capacity = (xs)->capacity == 0 ? 256 : (xs)->capacity * 2;\
		(xs)->items = realloc((xs)->items, (xs)->capacity * sizeof(*(xs)->items));\
	}\
	(xs)->items[(xs)->count++] = (x);\
} while(0)

#define da_free(xs) do {\
    if ((xs)-> items) {\
        free((xs)->items);\
        (xs)->items = NULL;\
        (xs)->capacity = 0;\
        (xs)->count = 0;\
    }\
} while(0)

#endif
