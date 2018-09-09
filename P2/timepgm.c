#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
    
double timestamp()
{
    struct timeval tv;

    gettimeofday( &tv, ( struct timezone * ) 0 );
    return ( (double) (tv.tv_sec + (tv.tv_usec / 1000000.0)) );
}

void main(int argc,char *argv[])
{
    double starttime, timetorun;

    timetorun = atof(argv[1]);
    printf("running for %lf secs\n",timetorun);
    starttime = timestamp();
    while ((timestamp() - starttime) < timetorun)
        ;
    printf("ran for %lf secs\n",timetorun);
}
