#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>

// #include "xs.h"
#include "bn.h"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 100

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);

// static ktime_t kt;
#define XOR_SWAP(a, b, type) \
    do {                     \
        type *__c = (a);     \
        type *__d = (b);     \
        *__c ^= *__d;        \
        *__d ^= *__c;        \
        *__c ^= *__d;        \
    } while (0)

// static void __swap(void *a, void *b, size_t size)
// {
//     if (a == b)
//         return;

//     switch (size) {
//     case 1:
//         XOR_SWAP(a, b, char);
//         break;
//     case 2:
//         XOR_SWAP(a, b, short);
//         break;
//     case 4:
//         XOR_SWAP(a, b, unsigned int);
//         break;
//     case 8:
//         XOR_SWAP(a, b, unsigned long);
//         break;
//     default:
//         /* Do nothing */
//         break;
//     }
// }

// static void reverse_str(char *str, size_t n)
// {
//     for (int i = 0; i < (n >> 1); i++)
//         __swap(&str[i], &str[n - i - 1], sizeof(char));
// }

// static void string_number_add(xs *a, xs *b, xs *out)
// {
//     char *data_a, *data_b;
//     size_t size_a, size_b;
//     int i, carry = 0;
//     int sum;

//     /*
//      * Make sure the string length of 'a' is always greater than
//      * the one of 'b'.
//      */
//     if (xs_size(a) < xs_size(b))
//         __swap((void *) &a, (void *) &b, sizeof(void *));

//     data_a = xs_data(a);
//     data_b = xs_data(b);

//     size_a = xs_size(a);
//     size_b = xs_size(b);

//     reverse_str(data_a, size_a);
//     reverse_str(data_b, size_b);

//     char *buf = kmalloc(sizeof(char) * (size_a + 2), GFP_KERNEL);

//     for (i = 0; i < size_b; i++) {
//         sum = (data_a[i] - '0') + (data_b[i] - '0') + carry;
//         buf[i] = '0' + sum % 10;
//         carry = sum / 10;
//     }

//     for (i = size_b; i < size_a; i++) {
//         sum = (data_a[i] - '0') + carry;
//         buf[i] = '0' + sum % 10;
//         carry = sum / 10;
//     }

//     if (carry)
//         buf[i++] = '0' + carry;

//     buf[i] = 0;

//     reverse_str(buf, i);

//     /* Restore the original string */
//     reverse_str(data_a, size_a);
//     reverse_str(data_b, size_b);

//     if (out)
//         *out = *xs_tmp(buf);
// }

static inline uint64_t fast_doubling_iter(long long target)
{
    if (target <= 2)
        return !!target;

    // find first 1
    uint8_t count = 63 - __builtin_clzll(target);
    uint64_t fib_n0 = 1, fib_n1 = 1;

    for (uint64_t i = count, fib_2n0, fib_2n1, mask; i-- > 0;) {
        fib_2n0 = fib_n0 * ((fib_n1 << 1) - fib_n0);
        fib_2n1 = fib_n0 * fib_n0 + fib_n1 * fib_n1;

        mask = -!!(target & (1UL << i));
        fib_n0 = (fib_2n0 & ~mask) + (fib_2n1 & mask);
        fib_n1 = (fib_2n0 & mask) + fib_2n1;
    }
    return fib_n0;
}

static inline uint64_t fast_doubling_recur(long long target)
{
    if (target <= 2)
        return !!target;

    // fib(2n) = fib(n) * (2 * fib(n+1) − fib(n))
    // fib(2n+1) = fib(n) * fib(n) + fib(n+1) * fib(n+1)
    uint64_t n = fast_doubling_recur(target >> 1);
    uint64_t n1 = fast_doubling_recur((target >> 1) + 1);

    // check 2n or 2n+1
    if (target & 1)
        return n * n + n1 * n1;
    return n * ((n1 << 1) - n);
}

static inline char *big_num_fib(long long k)
{
    if (k == 0)
        return "0";
    bn *a = bn_init(1);
    bn *b = bn_init(1);
    bn *c = bn_init(1);
    a->number[0] = 0;
    b->number[0] = 1;

    int i;
    char *tmp;
    for (i = 2; i <= k; i++) {
        bn_add(a, b, c);
        bn_free(a);
        a = b;
        b = c;
        c = bn_init(1);
    }
    tmp = bn_to_string(b);
    bn_free(a);
    bn_free(b);
    bn_free(c);
    return tmp;
}

static size_t fib_sequence(long long k, char __user *buf)
{
    char *str_k = big_num_fib(k);
    size_t size = sizeof(char) * strlen(str_k);

    if (copy_to_user(buf, str_k, size))
        return -EFAULT;
    kfree(str_k);
    return size;
}

static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}

static ssize_t fib_time_proxy(loff_t *offset, char __user *buf)
{
    // kt = ktime_get();
    ssize_t result = (ssize_t) fib_sequence(*offset, buf);
    // kt = ktime_sub(ktime_get(), kt);

    return result;
}


/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char __user *buf,
                        size_t size,
                        loff_t *offset)
{
    return fib_time_proxy(offset, buf);
}

/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    return 1;
    // return ktime_to_ns(kt);
}

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    mutex_init(&fib_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    fib_cdev->ops = &fib_fops;
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);
