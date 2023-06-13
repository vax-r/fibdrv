#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"

// static inline long long get_nanotime()
// {
//     struct timespec ts;
//     clock_gettime(CLOCK_REALTIME, &ts);
//     return ts.tv_sec * 1e9 + ts.tv_nsec;
// }

int main()
{
    long long sz;

    char read_buf[] = "";
    char write_buf[] = "testing writing";
    int offset = 92; /* TODO: try test something bigger than the limit */

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }
    // FILE *data = fopen("data.csv", "w");

    // for (int i = 0; i <= offset; i++) {
    //     sz = write(fd, write_buf, strlen(write_buf));
    //     printf("Writing to " FIB_DEV ", returned the sequence %lld\n", sz);
    // }
    // fprintf(data, "ktime,utime,systemcalltime\n");
    FILE *data = fopen("data_iter.csv", "w");
    fprintf(data, "ktime_iter\n");

    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        // long long start_time = get_nanotime();
        sz = read(fd, read_buf, 1);
        read_buf[sz] = '\0';
        // long long utime = get_nanotime() - start_time;
        ssize_t ktime = write(fd, write_buf, strlen(write_buf));
        fprintf(data, "%ld\n", ktime);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%s.\n",
               i, read_buf);
        printf("Writing to " FIB_DEV ", returned the sequence %ld.\n", ktime);
    }

    for (int i = offset; i >= 0; i--) {
        lseek(fd, i, SEEK_SET);
        sz = read(fd, read_buf, 1);
        read_buf[sz] = '\0';
        // ssize_t ktime = write(fd, write_buf, strlen(write_buf));
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%s.\n",
               i, read_buf);
        // printf("Writing to " FIB_DEV ", returned the sequence %ld.\n",
        // ktime);
    }

    fclose(data);
    close(fd);
    return 0;
}
