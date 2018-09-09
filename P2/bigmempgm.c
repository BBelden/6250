#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
    
void main(int argc,char *argv[])
{
    int tomalloc;  // for ben
    int malloced = 0;
    void *p;

    tomalloc = atoi(argv[1]);
    printf("mallocing at least %d MB\n",tomalloc);
    tomalloc *= 1000000;
    while (malloced < tomalloc)
    {
        p = malloc(1000000);
        malloced += 1000000;
    }
    printf("DONE mallocing\n");
}
