CFLAGS = -Wall 
LDFLAGS = -g
CC = gcc

TARGETS = statichello dlinkhello dloadhello 

all: $(TARGETS) 
	@echo "*  You may run 'ldd -v fn' to see the libraries used by an executable."

libhello.o: libhello.c
	gcc -fPIC $(CFLAGS) -c $<

libhello.so:	libhello.o
	gcc $(LDFLAGS) -shared -o libhello.so libhello.o
	@echo "*  You would run ldconfig to make this a 'permanent' library."

libhello.a:  libhello.o
	ar rcs libhello.a libhello.o

statichello: statichello.o libhello.a
	gcc $(LDFLAGS) -o $@ $< -static -L. -lhello

dlinkhello: dlinkhello.o libhello.so
	gcc $(LDFLAGS) -o $@ $< -L. -lhello
	@echo "*  To run dlinkhello: LD_LIBRARY_PATH=`pwd` ./$@"
	@echo " "

dloadhello: dloadhello.o libhello.so
	gcc $(LDFLAGS) -o $@ $< -ldl
	@echo "*  To run dloadhello: LD_LIBRARY_PATH=`pwd` ./$@"
	@echo " "

clean:
	rm -f $(TARGETS) *.o libhello.so* libhello.a

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
