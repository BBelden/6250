CFLAGS = -Wall 
LDFLAGS = -g
CC = gcc

TARGETS = statichello dlinkhello dloadhello 

all: $(TARGETS) 
	@echo "*  You may run 'ldd -v fn' to see the libraries used by an executable."

libhello.o: libhello.c
	@echo "*  Use -fPIC to build an object file for a shared library."
	gcc -fPIC $(CFLAGS) -c $<

libhello.so.0.0:	libhello.o
	@echo "*  Link libhello against libc for best results on all systems."
	# gcc $(LDFLAGS) -shared -Wl,-soname,libhello.so.0 -o $@ $< -lc
	gcc $(LDFLAGS) -shared -o libhello.so libhello.o
	@echo "*  Create a pointer from the soname to the library."
	ln -sf libhello.so.0.0 libhello.so.0
	@echo "*  Create a pointer for the linker."
	ln -sf libhello.so.0 libhello.so
	@echo "*  You would run ldconfig to make this a 'permanent' library."

libhello.a:  libhello.o
	ar rcs libhello.a libhello.o

statichello: statichello.o libhello.a
	gcc $(LDFLAGS) -o $@ $< -static -L. -lhello
	@echo "*  To run statichello: ./$@"
	@echo " "

dlinkhello: dlinkhello.o libhello.so.0.0
	@echo "*  -L. -lhello, not libhello.so.0"
	gcc $(LDFLAGS) -o $@ $< -L. -lhello
	@echo "*  To run dlinkhello: LD_LIBRARY_PATH=`pwd` ./$@"
	@echo " "

dloadhello: dloadhello.o libhello.so.0.0
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
