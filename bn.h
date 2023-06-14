#include <stdint.h>

// this is the header file for big number operations

#define maxLen 100
#define BASE 1000000000U
#define str_ulen 9

typedef struct _bn {
    unsigned int
        *number;  // the number in array form, each digits occupies an element
    unsigned int
        size;  // the size of allocated memory, unit is sizeof(unsigned int)
    int sign;  // 0 represents positive, 1 represents negative
} bn;

bn *bn_init(int64_t num);
char *bn_to_string(bn *src);