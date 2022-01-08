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