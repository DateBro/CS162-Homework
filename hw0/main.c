#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>

int main() {
    struct rlimit lim;
    int temp = getrlimit(RLIMIT_STACK,&lim);
    printf("stack size: %ld\n", (int)lim.rlim_cur);
    temp = getrlimit(RLIMIT_NPROC,&lim);
    printf("process limit: %ld\n", (int)lim.rlim_cur);
    temp = getrlimit(RLIMIT_NOFILE,&lim);
    printf("max file descriptors: %ld\n", (int)lim.rlim_cur);
    return 0;
}
