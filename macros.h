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

// this is da_append but it doesn't autoincrement the index, instead takes an at parameter, and checks to grow
#define da_at(xs, x, at) do {\
    size_t _at = (at);\
    if (_at >= (xs)->capacity) {\
        size_t _cap = (xs)->capacity == 0 ? 256 : (xs)->capacity;\
        while (_at >= _cap) _cap *= 2;\
        (xs)->items = realloc((xs)->items, _cap * sizeof(*(xs)->items));\
        (xs)->capacity = _cap;\
    }\
    (xs)->items[_at] = (x);\
    if (_at >= (xs)->count) (xs)->count = _at + 1;\
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
