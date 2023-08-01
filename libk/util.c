#include <string.h>
#include <stdint.h>
#include <util.h>

void stack_trace(void)
{
    size_t *bp;

    __asm__ volatile(
            "mov %0, ebp;"
            : "=r" (bp)
            );
    puts("stack trace:");
    puthex(*(bp + 1) );
    putchar('\n');
    while( (bp = (size_t *)*bp) ) {
        puthex(*(bp + 1) );
        putchar('\n');
    };
}
