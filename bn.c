#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bn.h"

// #define maxlen 100

static inline size_t num_len(int32_t num, int32_t base)
{
    if (!num)
        return 1;
    size_t len = 0;
    while (num) {
        len++;
        num /= base;
    }
    return len;
}

bn *bn_init(int64_t num)
{
    // msb is new_bn->number[len]
    bn *new_bn = malloc(sizeof(bn));
    size_t len = num_len(num, BASE);

    if (len > maxLen)
        len = maxLen;
    new_bn->number = malloc(len * sizeof(unsigned int));
    new_bn->size = (unsigned int) len;
    new_bn->sign = 0;
    if (!num) {
        new_bn->number[0] = 0;
        return new_bn;
    }
    if (num < 0) {
        new_bn->sign = 1;
        num *= -1;
    }
    size_t i;
    for (i = 0; i <= len && num; i++, num /= BASE) {
        new_bn->number[i] = num % BASE;
    }
    return new_bn;
}

char *bn_to_string(bn *src)
{
    char *ret = malloc(src->size * str_ulen * sizeof(char));
    char *q = ret;
    int buf_size = src->size * str_ulen;
    snprintf(q, buf_size, "%u", src->number[src->size - 1]);
    while (*q)
        q++;
    for (int i = src->size - 2; i >= 0; i--) {
        snprintf(q, buf_size, "%09u", src->number[i]);
        q += str_ulen;
    }
    *q = '\0';
    return ret;
}

void bn_free(bn *src)
{
    free(src->number);
}

int main()
{
    int64_t a = 2000000001;
    bn *big_a = bn_init(a);

    char *bn_str = bn_to_string(big_a);
    printf("%s\n", bn_str);

    bn_free(big_a);
    return 0;
}