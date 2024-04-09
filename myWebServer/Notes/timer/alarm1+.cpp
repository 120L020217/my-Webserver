

/*
 一秒计数
*/
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>

static jmp_buf jmpbuffer; // 保存某一时刻程序状态

int main()
{
    // 捕捉信号
    signal(SIGALRM, [](int){
        longjmp(jmpbuffer, 1);
    });

    alarm(1);
    /*信号处理函数和主循环之间的跳转可能导致程序在不同的时间点执行。
    volatile 保证每次访问 i 时都会从内存中读取最新的值，
    而不是使用缓存的值。*/
    volatile size_t i = 0;

    // longjmp 跳转到这里
    if (setjmp(jmpbuffer) != 0)
    {
        printf("sum = %lu\n", i);
        _exit(0);
    }

    while (true)
        ++i;
}
