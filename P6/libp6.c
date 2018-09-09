// Name:        Ben Belden
// Class ID#:   bpb2v
// Section:     CSCI 6250-001
// Assignment:  Lab #6
// Due:         16:20:00, November 21, 2016
// Purpose:     Practice dynamically loading libraries, you know, for fun.          
// Input:       From terminal.  
// Outut:       To terminal.
// 
// File:        libp6.c

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void p6f1(void) {
   printf("p6f1 %d\n", getpid());
}

void p6f2(void) {
   printf("p6f2 %d\n", getpid());
}
