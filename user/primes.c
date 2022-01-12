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