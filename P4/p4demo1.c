#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>


void main(int argc, char *argv[])
{
    char *shmfn, *p;
    void *shmptr;
    int shmfd, i;
    int SHM_SIZE;

    if (argc < 3)
    {
        printf("usage: %s shmname shmsize\n",argv[0]);
        exit(-1);
    }

    shmfn = argv[1];
    SHM_SIZE = (1024*1024) * atoi(argv[2]);
    /* open shmem object */
    shmfd = shm_open(shmfn, O_RDWR, 0);  // 0: mode
    if (shmfd == -1)
    {
        perror("shm_open failed");
        exit(-1);
    }

    shmptr = mmap((void *)NULL, SHM_SIZE, PROT_WRITE|PROT_READ, MAP_SHARED, shmfd, 0);
    if (shmptr == (void *)-1)
    {
        perror("mmap failed");
        exit(-1);
    }

    p = (char *)shmptr;
    for (i=0; i < SHM_SIZE; i++)
    {
		if (i%((1024*1024))!=0)
			*(p+i) = 'X';
		else *(p+i) = 'B';
    }

    for (i=0; i < SHM_SIZE; i++)
    {
        if (*(p+i) != 'X')
        {
            printf("bad char %c at %d\n",*(p+i),i);
            //break;
        }
    }

    munmap(shmptr,SHM_SIZE);

    /* close the shmem object */
    close(shmfd);

    /* delete shmem object unless I want to leave them there for later use */
    /* they can be deleted by hand from /dev/shm */
    // shm_unlink(shmfn);
}
