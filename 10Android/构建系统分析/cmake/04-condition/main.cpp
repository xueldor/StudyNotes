#include <stdio.h>
#include <stdbool.h>

int printLinux() ;
int main() {
    printf("main\n");
#ifdef I_AM_LINUX
    printLinux();
#endif
    return 0;
}