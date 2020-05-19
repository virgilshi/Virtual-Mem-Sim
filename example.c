#include <stdio.h>
#include <sys/time.h>

#include "vmsim.h"

#define NR_REQUEST 1000000
#define NR_OVERWRITE 1000

int main()
{
    int ret = 0;
    ret = vmsim_init();
    if (ret == FALSE)
    {
        printf("init error\n");
        return -1;
    }

    struct timeval start, end;
    gettimeofday(&start, NULL);

    vm_map_t mp;
    w_size_t num_pages = 4096; // virtual memory
    w_size_t num_frames = 2048; // physical memory

    ret = vm_alloc(num_pages, num_frames, &mp);
    if (ret == FALSE)
    {
        printf("alloc error\n");
        return -1;
    }

    // printf("alloc ok\n");

    char* p = (char*)mp.start;
    for (int i = 0; i < NR_REQUEST; ++i) {
        *p = 'a';
        ++p;
    }

    // printf("will free\n");

    ret = vm_free(mp.start);
    if (ret == FALSE)
    {
        printf("free error\n");
        return -1;
    }

    gettimeofday(&end, NULL);

    printf("execution time: %ld\n", (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec));
    return 0;
}
