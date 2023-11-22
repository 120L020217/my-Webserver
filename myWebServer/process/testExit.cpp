#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(){
    printf("Hello\n"); // 带\n,内部会刷缓存
    printf("world");
    
    // exit(0);
    _exit(0);
    return 0;
}