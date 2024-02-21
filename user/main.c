#include "user/ulib.h"

#define filetype(flag) (flag == DIRECTORY ? 'D' : 'F')
#define BUFFER_MAX_SIZE 256

int search_command(char *str)
{
    struct TimeStamp ts;
    char buf[256];
    int fd, count;
    if (!strcmp(str, "exit")) {
        printf("entering infinite loop...\n");
        return -1;
    } else if (!strcmp(str, "hello")) {
        printf("Hello world\n");
    } else if (!strcmp(str, "help")) {
        printf(
            "help: print all available commands\n"
            "hello: print Hello world!\n"
            "timestamp: print current timestamp\n"
            "ls: list all files\n"
            "pwd: show working directory\n"
            "cd: change working directory\n"
            "cat: dump file content\n"
            "reboot: reboot rpi3 (not work on QEMU)\n"
            "exit: exit shell\n");
    } else if (!strcmp(str, "timestamp")) {
        get_timestamp(&ts);
        printf("[%f]\n", (float) ts.counts / ts.freq);
    } else if (!strcmp(str, "reboot")) {
        printf("Rebooting...\n");
        reset(50);
        while (1)
            ;
    } else if (!strcmp(str, "ls")) {
        dir_t *dir;
        char name[256];
        enum node_attr_flag flag;
        size_t size;
        if (opendir(".", &dir) != -1) {
            printf("Flag\tSize\tFilename\n", filetype(flag), size, name);
            while (readdir(dir, name, &flag, &size) != -1) {
                printf("%c\t%d\t%s\n", filetype(flag), size, name);
            }
            closedir(dir);
        }
    } else if (!strcmp(str, "pwd")) {
        getcwd(buf, 256);
        printf("%s\n", buf);
    } else if (!strncmp(str, "cd ", 3)) {
        chdir(&str[3]);
    } else if (!strncmp(str, "cat ", 4)) {
        if ((fd = open(&str[4], 0)) == -1) {
            printf("file not found\n");
        } else {
            while ((count = read(fd, buf, sizeof(buf) - 1)) > 0) {
                buf[count] = 0;
                printf("%s", buf);
            }
            close(fd);
        }
    } else if (!strcmp(str, "test")) {
        fd = open("/a.txt", O_CREAT);
        int sz = write(fd, "Hello world", 11);
        printf("write %d bytes to fd %d\n", sz, fd);
        close(fd);
    } else {
        printf("Unknown command\n");
    }
    return 0;
}

int main()
{
    char str[BUFFER_MAX_SIZE];
    printf("\n---Raspberry PI 3 B+---\n");
    printf("~$ ");
    while (fgets(str, BUFFER_MAX_SIZE)) {
        str[strlen(str) - 1] = 0;  // Replace newline with null byte
        if (search_command(str) == -1)
            goto _exit;
        printf("~$ ");
    }
_exit:
    return 0;
}