#ifndef STDDEF_H
#define STDDEF_H

#if defined(__has_builtin)
    #if __has_builtin(__builtin_offsetof)
        #define offsetof(type, member) (__builtin_offsetof(type, member))
    #else
        #define offsetof(type, member) ((size_t)&(((type *)(0))->member))
    #endif
#else
    #define offsetof(type, member) ((size_t)&(((type *)(0))->member))
#endif

#define NULL ((void *)0)

#ifdef __i386__

typedef unsigned long size_t;
typedef signed long   ptrdiff_t;

typedef int wchar_t;

#endif /* __i386__ */

#endif /* STDDEF_H */
