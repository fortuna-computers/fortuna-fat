#include "test.h"

#include <stdio.h>

int main()
{
    size_t i = 0;
    do {
        printf("[%c] %s\n", (char) i + 'A', test_list[i].name);
        ++i;
    } while (test_list[i].name);
    
    printf("%40c", ' ');
    i = 0; do { printf("%c", (char) i + 'A'); ++i; } while (test_list[i].name);
    printf("\n");
    for (size_t j = 0; j < 40 + i; ++j) putchar('=');
    putchar('\n');
    
    return 0;
}