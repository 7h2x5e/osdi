#include "user/ulib.h"

void delay(int period)
{
    while (period--)
        ;
}

int main()
{
    char buf[32];
    int sz, fd, ret;

    mkdir("mnt");
    ret = mount("tmpfs", "mnt", "tmpfs");
    fd = open("mnt/a.txt", O_CREAT);
    sz = write(fd, "Hello world", 11);
    printf("write %d bytes to fd %d\n", sz, fd);
    close(fd);

    memset(buf, 0, sizeof(buf));
    chdir("mnt");
    fd = open("a.txt", 0);
    sz = read(fd, buf, 11);
    printf("read %d bytes from fd %d: %s\n", sz, fd, buf);
    close(fd);

    memset(buf, 0, sizeof(buf));
    chdir("..");
    fd = open("a.txt", 0);
    sz = read(fd, buf, 11);
    printf("read %d bytes from fd %d: %s\n", sz, fd, buf);
    close(fd);

    return 0;
}