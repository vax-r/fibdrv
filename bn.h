#ifndef _BIGNUM_H_
#define _BIGNUM_H_

#define BASE 10000000000000000
#define BLOCK_LEN 16  // the number of digits per block

#include <stddef.h>

typedef unsigned long long n_type;

typedef struct {
    n_type *number;
    unsigned int size;
    int sign;
} bn;

bn *bn_init(size_t size);
void bn_free(bn *num);
void bn_add(bn *a, bn *b, bn *c);
void bn_resize(bn *num, int new_size);
char *bn_to_string(bn *num);



#endif