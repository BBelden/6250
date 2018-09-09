/* Example of an exclusive access of write and read processes to the
   shared-memory region ensured by use of a semaphore
*/

/*  b01:  cc -o ssmmw ssmmw.c -lpthread -lrt */


/* WRITER */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>

char *shmfn = "rbutler_shm";
char *semfn = "rbutler_sem";

void main(void)
{
    void *shmptr;
    unsigned int mode;
    int shmfd, i;
    sem_t *semfd;
    int SHM_SIZE;


    /* open shmem object */
    mode = S_IRWXU | S_IRWXG;
    shmfd = shm_open(shmfn, O_CREAT|O_RDWR|O_TRUNC, mode);
    if (shmfd == -1)
    {
        perror("shm_open failed");
        exit(-1);
    }

    /* preallocate shmem area */
    SHM_SIZE = sysconf( _SC_PAGE_SIZE );
    if (ftruncate(shmfd,SHM_SIZE) == -1)
    {
        perror("ftruncate failed");
        exit(-1);
    }

    shmptr = mmap((void *)NULL, SHM_SIZE, PROT_WRITE|PROT_READ, MAP_SHARED, shmfd, 0);
    if (shmptr == (void *)-1)
    {
        perror("mmap failed");
        exit(-1);
    }

    /* create semaphore in LOCKED state */
    semfd = sem_open(semfn, O_CREAT, 0644, 0);  // 0 -> locked
    if (semfd == SEM_FAILED)
    {
        perror("sem_open failed");
        exit(-1);
    }

    /* use/access the shmem */
    for (i=0; i < 100; i++)
    {
        ((int *)shmptr)[i] = i * 2;
        printf("wrote %d into shmem shmptr[%d]  %d\n", i*2, i, ((int *)shmptr)[i]);
    }

    /* release sem lock */
    sem_post(semfd);
    munmap(shmptr,SHM_SIZE);

    /* close the shmem object */
    close(shmfd);

    /* close sem */
    sem_close(semfd);

    /* delete objects unless a reader will access them later */
    /* they can be deleted by hand from /dev/shm */
    // shm_unlink(shmfn);
    // sem_unlink(semfn);
}
