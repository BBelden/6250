/* Example of an exclusive access of write and read processes to the
   shared-memory region ensured by use of a semaphore
*/

/*  b01:  cc -o ssmmr ssmmr.c -lpthread -lrt */


/* READER */

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
    int shmfd, i;
    sem_t *semptr;
    int SHM_SIZE;

    SHM_SIZE = sysconf( _SC_PAGE_SIZE );
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

    /* gain access to semaphore */
    semptr = sem_open(semfn, 0, 0644, 0);
    if (semptr == SEM_FAILED)
    {
        perror("sem_open failed");
        exit(-1);
    }

    /* lock the sem */
    if (sem_wait(semptr) == 0)  // got the lock
    {
        /* use/access the shmem */
        for (i=0; i < 100; i++)
        {
            printf("shmem shmptr[%d] = %d\n", i, ((int *)shmptr)[i]);
        }
        /* release sem lock */
        sem_post(semptr);
    }

    munmap(shmptr,SHM_SIZE);

    /* close the shmem object */
    close(shmfd);

    /* close sem */
    sem_close(semptr);

    /* delete shmem object unless I want to leave them there for later use */
    /* they can be deleted by hand from /dev/shm */
    shm_unlink(shmfn);
    sem_unlink(semfn);
}
