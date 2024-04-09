

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
    /*
    setjump函数保存堆栈和寄存器的当前状态，
    使用setjmp和longjmp的原因是，它们可以从信号处理函数中跳出，
    回到主程序中的特定位置。这是因为信号处理函数在一个独立的执行上下文中运行，
    不能直接使用return或break来跳出主循环。而setjmp和longjmp提供了一种机制，
    可以在这两个上下文之间进行跳转。
    
    volatile 保证每次访问 i 时都会从内存中读取最新的值，而不是使用缓存的值。
    */
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
