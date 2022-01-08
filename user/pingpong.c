#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int p[2];
    int pid;
    char content[16];

    if(argc > 1){
        fprintf(2, "usage: pingpong\n");
        exit(1);
    }

    pipe(p);
    if(fork() == 0) {
        // child
        pid = getpid();
        // 读取父进程写入管道的数据
        read(p[0], content, sizeof(content));
        close(p[0]);
        printf("%d: received %s\n", pid, content);
        write(p[1], "pong", 4);
        close(p[1]);
    } else {
        // parent
        pid = getpid();
        write(p[1], "ping", 4);
        close(p[1]);
        // 注意：需要等待子进程
        wait(0);
        // 读取子进程写入管道的数据
        read(p[0], content, sizeof(content));
        printf("%d: received %s\n", pid, content);
        close(p[1]);
    }

    exit(0);
}