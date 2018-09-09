CFLAGS = -Wall 
LDFLAGS = -g
CC = gcc

TARGETS = p6 

all: $(TARGETS) 

libp6.o: libp6.c
	gcc -fPIC $(CFLAGS) -c $<

libp6.so:	libp6.o
	gcc $(LDFLAGS) -shared -o libp6.so libp6.o

libp6.a:  libp6.o
	ar rcs libp6.a libp6.o

p6: p6.o libp6.so
	gcc $(LDFLAGS) -o $@ $< -ldl

clean:
	rm -f $(TARGETS) *.o libp6.so* libp6.a

expand:
	@for n in *.c; do \
	    expand $$n > $$n.foo; \
	    if cmp -s $$n $$n.foo ; then \
		rm -f $$n.foo; \
	    else  \
		echo "Updating $$n"; \
		mv $$n.foo $$n; \
	    fi \
	done 
