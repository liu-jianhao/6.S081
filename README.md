# 6.S081 Lab1-Xv6 and Unix utilities

## Boot xv6
这个应该在之前的搭建环境中完成了。

退出qemu的命令比较坑，先按ctrl+a，松开后在按x。

## sleep
这个是个热身题，比较简单。

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int duration;

    if(argc <= 1){
        fprintf(2, "usage: sleep NUMBER\n");
        exit(1);
    }

    duration = atoi(argv[0]);
    sleep(duration);
    exit(0);
}
```


- 在跑测试之前需要在Makefile加上UPROGS 里加上 sleep
```
$ ./grade-lab-util sleep
make: `kernel/kernel' is up to date.
== Test sleep, no arguments == sleep, no arguments: OK (1.5s) 
== Test sleep, returns == sleep, returns: OK (0.8s) 
== Test sleep, makes syscall == sleep, makes syscall: OK (0.8s)
```


## pingpong
这题主要是理解pipe和fork
```c
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
```

- 这里有个需要注意的点是在父进程中需要等待子进程

## primes
这题是要做一个多进程版的素数筛
```
p = get a number from left neighbor
print p
loop:
    n = get a number from left neighbor
    if (p does not divide n)
        send n to right neighbor
```
也就是把上面的伪代码理解清楚了就好实现了。

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
child_process(int p[2])
{
    int pp[2];
    int prime;
    int len;
    int num;

    // 不需要从右边读取数据
    close(p[1]);
    len = read(p[0], &prime, sizeof(prime));
    if(len == 0){
        close(p[0]);
        exit(0);
    }
    // 打印第一个素数
    printf("prime %d\n", prime);

    pipe(pp);
    if(fork() == 0){
        close(p[0]);
        child_process(pp);
    } else {
        close(pp[0]);
        while(1){
            len = read(p[0], &num, sizeof(num));
            if(len == 0){
                break;
            }

            // 过滤数字
            if(num % prime != 0){
                write(pp[1], &num, sizeof(num));
            }
        }

        close(p[0]);
        close(pp[1]);
        wait(0);
    }
}

int
main(int argc, char *argv[])
{
    int p[2];
    int i;

    if(argc > 1){
        fprintf(2, "usage: primes\n");
        exit(1);
    }

    pipe(p);
    if(fork() == 0){
        child_process(p);
    } else {
        close(p[0]);
        for(i = 2; i <= 35; i++){
            write(p[1], &i, sizeof(i));
        }
        close(p[1]);
        wait(0);
    }

    exit(0);
}
```


## find
这个大部分的实现都可以参考ls.c 
```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    return p;
}

void
find(char* path, char* filename)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
        case T_FILE:
            if(!strcmp(fmtname(path), filename))
                printf("%s\n", path);
            break;

        case T_DIR:
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("ls: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0 || !strcmp(de.name, ".") || !strcmp(de.name, ".."))
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                find(buf, filename);
            }
            break;
    }
    close(fd);
}

int
main(int argc, char *argv[])
{
    if(argc < 3){
        fprintf(2, "usage: find directory filename\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}
```

## xargs
```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXN 1024

int
main(int argc, char *argv[])
{
    char* _argv[MAXARG];
    int i;
    char buf[MAXN];
    int len;

    if(argc < 2){
        fprintf(2, "usage: xargs command\n");
        exit(1);
    }

    if(argc + 1 > MAXN){
        fprintf(2, "too many args\n");
        exit(1);
    }

    for(i = 1; i < argc; i++){
        _argv[i-1] = argv[i];
    }
    _argv[i] = 0;

    while(1){
        i = 0;
        // 读取一行
        while(1){
            len = read(0, &buf[i], 1);
            if(len == 0 || buf[i] == '\n')
                break;
            i++;
        }
        if(i == 0)
            break;

        buf[i] = 0;
        _argv[argc-1] = buf;
        if(fork() == 0){
            // child
            exec(_argv[0], _argv);
            exit(0);
        } else {
            // parent
            wait(0);
        }
    }
    exit(0);
}
```
