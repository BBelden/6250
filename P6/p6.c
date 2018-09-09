// Name:		Ben Belden
// Class ID#:	bpb2v
// Section:		CSCI 6250-001
// Assignment:	Lab #6
// Due:			16:20:00, November 21, 2016
// Purpose:	    Practice dynamically loading libraries, you know, for fun.			
// Input:	    From terminal.	
// Outut:		To terminal.
// 
// File:		p6.c

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

typedef void (*hello_function)(void);

int main (int argc, char *argv[])
{
	void *lib;
	hello_function hello;
	const char *err;

	if (argc < 2) fprintf(stderr,"Not enough args\n");

	lib = dlopen("libp6.so",RTLD_LAZY);
	if (lib == NULL)
	{
		fprintf(stderr,"Could not open libp6.so: %s\n",dlerror());
		exit(1);
	}

	dlerror();
	hello = dlsym(lib,argv[1]);
	err = dlerror();
	if (err)
	{
		fprintf(stderr,"Could not find %s: %s\n",argv[1],err);
		exit(1);
	}
	(*hello)();
	dlclose(lib);
	return 0;
}

/*

	*.a --> static linking
	*.so --> dynamic linking/loading

	use dloadhello.c as an example
	/usr/lib/x86_64-linux-gnu
	/lib/x86_64-linux-gnu

	make statichello
	make dlinkhello
	make dloadhello

	-L. // library files are in cwd
	-lhello // library file is named libhello

	export LD_LIBRARY_PATH=.

	use libhello.c, dloadhello.c, and Makefile as templates
*/
