#include <stdio.h>

typedef void (*hello_function)();


int f(int a,int b,int c)
{
    printf("%d %d %d\n",a,b,c);
}

main()
{
     hello_function hello = f;
     (*hello)(33,44,55);
}
