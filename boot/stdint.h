#ifndef STDINT_H
#define STDINT_H

# ifdef __i386__

    typedef char                int8_t;
    typedef short               int16_t;
    typedef long                int32_t;
    typedef long long           int64_t;

    typedef unsigned char       uint8_t;
    typedef unsigned short      uint16_t;
    typedef unsigned long       uint32_t;
    typedef unsigned long long  uint64_t;

    typedef long                ssize_t;
    typedef unsigned long       size_t;

    typedef unsigned long       intptr_t;

# endif

#endif
