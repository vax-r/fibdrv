#include "bn.h"

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>

bn *bn_init(size_t size)
{
    bn *new_num = kmalloc(sizeof(bn), GFP_KERNEL);
    if (!new_num) {
        printk(KERN_ERR "failed to create memory for big number\n");
        // exit(1);
        // return new_num;
    }
    new_num->number = kmalloc(sizeof(n_type) * size, GFP_KERNEL);
    if (!new_num->number) {
        printk(KERN_ERR "failed to create memory for numbers\n");
        // exit(1);
    }
    memset(new_num->number, 0, sizeof(n_type) * size);
    new_num->size = size;
    new_num->sign = 0;
    return new_num;
}

void bn_free(bn *num)
{
    if (!num)
        return;
    kfree(num->number);
    kfree(num);
}

unsigned int max_size(bn *num1, bn *num2)
{
    unsigned int ans = num1->size >= num2->size ? num1->size : num2->size;
    return ans;
}

void bn_resize(bn *num, int new_size)
{
    if (num->size == new_size)
        return;
    num->number = krealloc(num->number, new_size * sizeof(n_type), GFP_KERNEL);
    if (!num->number) {
        printk("failed to realloc\n");
        // exit(1);
    }
    if (new_size > num->size)
        memset(num->number + num->size, 0,
               sizeof(n_type) * (new_size - num->size));
    num->size = new_size;
}

void bn_add(bn *a, bn *b, bn *c)
{
    // c = a + b
    unsigned int new_size = max_size(a, b) + 1;
    bn_resize(c, new_size);

    n_type carry = 0;
    int i;
    for (i = 0; i < new_size; i++) {
        n_type tmp1 = i < a->size ? a->number[i] : 0;
        n_type tmp2 = i < b->size ? b->number[i] : 0;
        carry += (tmp1 + tmp2);
        if (carry >= BASE) {
            c->number[i] = (carry - BASE);
            carry = 1;
        } else {
            c->number[i] = carry;
            carry = 0;
        }
    }
    if (!c->number[c->size - 1] && c->size > 1)
        bn_resize(c, c->size - 1);
}


char *string_resize(char *str, int len)
{
    // remove the prefix zeroes in str
    if (str[0] != '0')
        return str;
    int count = 0;
    for (count = 0; str[count] == '0'; count++)
        ;
    int new_len = len - count;
    char *new_str = kmalloc(sizeof(char) * new_len, GFP_KERNEL);
    strncpy(new_str, &str[count], new_len);
    new_str[new_len - 1] = '\0';
    kfree(str);
    return new_str;
}

char *bn_to_string(bn *num)
{
    int str_len = num->size * BLOCK_LEN + 1;
    char *num_str = kmalloc(sizeof(char) * str_len, GFP_KERNEL);
    memset(num_str, '\0', str_len - 1);
    int i, j, k;
    n_type block_num;
    j = 0, k = 0;
    block_num = num->number[j];
    for (i = str_len - 2; i >= 0; i--) {
        num_str[i] = (block_num % 10) + '0';
        block_num /= 10;
        k++;
        if (k % BLOCK_LEN == 0) {
            j++;
            block_num = num->number[j];
        }
    }
    num_str = string_resize(num_str, str_len);
    return num_str;
}